#!/usr/bin/env python3
"""Validate Wolf3D Revive distribution policy and staged release trees.

This is deliberately separate from the future runtime content manifest.  It
answers whether a file may be distributed, not how the engine loads it.
"""
from __future__ import annotations

import argparse
import hashlib
import os
import re
import stat
import subprocess
import sys
import tomllib
from dataclasses import dataclass
from pathlib import Path, PurePosixPath


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "assets" / "distribution.toml"
MAX_MANIFEST_BYTES = 1024 * 1024
READ_CHUNK_BYTES = 1024 * 1024
STATES = {"approved", "prohibited", "pending-review", "development-only"}
REVIEWED_LICENSES = {
    "CC-BY-4.0",
    "CC0-1.0",
    "GPL-2.0-only",
    "MIT",
    "OFL-1.1",
    "Zlib",
}
NON_APPROVABLE_LICENSES = {
    "NOASSERTION",
    "LicenseRef-Commercial-Proprietary",
    "LicenseRef-Historical-Unverified",
    "LicenseRef-Pending-Review",
}
LICENSES = REVIEWED_LICENSES | NON_APPROVABLE_LICENSES
ID_RE = re.compile(r"^[a-z0-9][a-z0-9-]*$")
PROFILE_RE = re.compile(r"^[a-z0-9][a-z0-9-]*$")
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
DRIVE_RE = re.compile(r"^[A-Za-z]:")


class DistributionError(Exception):
    """A policy or input failure suitable for a command-line diagnostic."""


@dataclass(frozen=True)
class Profile:
    name: str
    description: str
    required_assets: tuple[str, ...]


@dataclass(frozen=True)
class Asset:
    id: str
    paths: tuple[str, ...]
    path_kind: str
    category: str
    origin: str
    creator: str
    license: str
    redistribution: str
    evidence: str
    sha256: str
    attribution_required: bool
    notice: str
    notice_path: str
    profiles: tuple[str, ...]
    notes: str


@dataclass(frozen=True)
class Manifest:
    path: Path
    repository_root: Path
    name: str
    purpose: str
    max_files: int
    max_file_size: int
    max_total_size: int
    profiles: dict[str, Profile]
    assets: tuple[Asset, ...]


def _fail(message: str) -> None:
    raise DistributionError(message)


def _required(record: dict, key: str, owner: str, expected: type):
    if key not in record:
        _fail(f"{owner}: missing mandatory field {key!r}")
    value = record[key]
    if not isinstance(value, expected):
        _fail(f"{owner}: field {key!r} must be {expected.__name__}")
    return value


def _safe_relative(raw: str, owner: str, *, prefix: bool = False) -> str:
    if not raw:
        _fail(f"{owner}: path must not be empty")
    if "\\" in raw:
        _fail(f"{owner}: mixed or Windows path separators are not supported: {raw!r}")
    if DRIVE_RE.match(raw):
        _fail(f"{owner}: drive-qualified path is not allowed: {raw!r}")
    if raw.startswith("/"):
        _fail(f"{owner}: absolute path is not allowed: {raw!r}")
    if prefix and not raw.endswith("/"):
        _fail(f"{owner}: prefix must end with '/': {raw!r}")
    check = raw[:-1] if prefix else raw
    parts = PurePosixPath(check).parts
    if not parts or any(part in ("", ".", "..") for part in parts):
        _fail(f"{owner}: unsafe relative path: {raw!r}")
    normalized = "/".join(parts) + ("/" if prefix else "")
    if normalized != raw:
        _fail(f"{owner}: path is not normalized: {raw!r}")
    return raw


def _bounded_toml(path: Path) -> dict:
    try:
        size = path.stat().st_size
    except OSError as exc:
        _fail(f"cannot stat manifest {path}: {exc}")
    if size > MAX_MANIFEST_BYTES:
        _fail(f"manifest exceeds {MAX_MANIFEST_BYTES} byte limit: {path}")
    try:
        with path.open("rb") as stream:
            return tomllib.load(stream)
    except (OSError, tomllib.TOMLDecodeError) as exc:
        _fail(f"cannot parse manifest {path}: {exc}")


def load_manifest(path: Path = DEFAULT_MANIFEST, repository_root: Path = ROOT) -> Manifest:
    """Load and fully validate a distribution manifest."""
    data = _bounded_toml(path)
    if data.get("schema_version") != 1:
        _fail(f"unsupported schema_version {data.get('schema_version')!r}; expected 1")

    distribution = _required(data, "distribution", "manifest", dict)
    name = _required(distribution, "name", "distribution", str)
    purpose = _required(distribution, "purpose", "distribution", str)
    if distribution.get("default_policy") != "deny":
        _fail("distribution.default_policy must be 'deny'")
    limits = _required(data, "limits", "manifest", dict)
    max_files = _required(limits, "max_files", "limits", int)
    max_file_size = _required(limits, "max_file_size", "limits", int)
    max_total_size = _required(limits, "max_total_size", "limits", int)
    if min(max_files, max_file_size, max_total_size) <= 0:
        _fail("all distribution limits must be positive")

    raw_profiles = _required(data, "profiles", "manifest", dict)
    profiles: dict[str, Profile] = {}
    for profile_name, raw in sorted(raw_profiles.items()):
        owner = f"profile {profile_name!r}"
        if not PROFILE_RE.fullmatch(profile_name):
            _fail(f"{owner}: invalid profile name")
        if not isinstance(raw, dict):
            _fail(f"{owner}: profile definition must be a table")
        description = _required(raw, "description", owner, str)
        required_assets = _required(raw, "required_assets", owner, list)
        if not description or not all(isinstance(item, str) for item in required_assets):
            _fail(f"{owner}: invalid description or required_assets")
        if len(required_assets) != len(set(required_assets)):
            _fail(f"{owner}: duplicate required asset ID")
        profiles[profile_name] = Profile(profile_name, description, tuple(required_assets))
    if "original-standalone" not in profiles:
        _fail("manifest must define the original-standalone profile")

    raw_assets = _required(data, "asset", "manifest", list)
    assets: list[Asset] = []
    ids: set[str] = set()
    exact_paths: dict[str, str] = {}
    prefixes: dict[str, str] = {}
    case_paths: dict[str, str] = {}
    for index, raw in enumerate(raw_assets):
        owner = f"asset[{index}]"
        if not isinstance(raw, dict):
            _fail(f"{owner}: entry must be a table")
        asset_id = _required(raw, "id", owner, str)
        owner = f"asset {asset_id!r}"
        if not ID_RE.fullmatch(asset_id):
            _fail(f"{owner}: invalid stable identifier")
        if asset_id in ids:
            _fail(f"duplicate asset ID: {asset_id}")
        ids.add(asset_id)

        selectors = [key for key in ("path", "paths", "prefix") if key in raw]
        if len(selectors) != 1:
            _fail(f"{owner}: exactly one of path, paths or prefix is required")
        selector = selectors[0]
        if selector == "path":
            paths = (_safe_relative(_required(raw, "path", owner, str), owner),)
            path_kind = "file"
        elif selector == "paths":
            values = _required(raw, "paths", owner, list)
            if not values or not all(isinstance(item, str) for item in values):
                _fail(f"{owner}: paths must be a non-empty string array")
            paths = tuple(_safe_relative(item, owner) for item in values)
            path_kind = "file-set"
        else:
            paths = (_safe_relative(_required(raw, "prefix", owner, str), owner, prefix=True),)
            path_kind = "prefix"

        category = _required(raw, "type", owner, str)
        origin = _required(raw, "origin", owner, str)
        creator = _required(raw, "creator", owner, str)
        license_id = _required(raw, "license", owner, str)
        redistribution = _required(raw, "redistribution", owner, str)
        evidence = _required(raw, "evidence", owner, str)
        attribution = _required(raw, "attribution_required", owner, bool)
        notes = _required(raw, "notes", owner, str)
        raw_asset_profiles = _required(raw, "profiles", owner, list)
        if not all((category, origin, creator, notes)):
            _fail(f"{owner}: type, origin, creator and notes must not be empty")
        if redistribution not in STATES:
            _fail(f"{owner}: unknown redistribution status {redistribution!r}")
        if license_id not in LICENSES:
            _fail(f"{owner}: unknown licence state {license_id!r}")
        if evidence:
            _safe_relative(evidence, owner)
        if not raw_asset_profiles or not all(isinstance(item, str) for item in raw_asset_profiles):
            _fail(f"{owner}: profiles must be a non-empty string array")
        if len(raw_asset_profiles) != len(set(raw_asset_profiles)):
            _fail(f"{owner}: duplicate profile reference")
        unknown_profiles = set(raw_asset_profiles) - set(profiles)
        if unknown_profiles:
            _fail(f"{owner}: unknown profile reference(s): {sorted(unknown_profiles)}")

        digest = raw.get("sha256", "")
        if not isinstance(digest, str) or (digest and not SHA256_RE.fullmatch(digest)):
            _fail(f"{owner}: sha256 must be 64 lowercase hexadecimal characters")
        if redistribution == "approved":
            if path_kind != "file":
                _fail(f"{owner}: approved assets must use one exact path")
            if not digest:
                _fail(f"{owner}: approved file requires sha256")
            if not evidence:
                _fail(f"{owner}: approved file requires provenance evidence")
            if license_id in NON_APPROVABLE_LICENSES:
                _fail(f"{owner}: approved file cannot use non-approvable licence state {license_id}")
            evidence_path = repository_root / _safe_relative(evidence, owner)
            if not evidence_path.is_file():
                _fail(f"{owner}: provenance evidence does not exist: {evidence}")
        notice = raw.get("notice", "")
        notice_path = raw.get("notice_path", "")
        if not isinstance(notice, str) or not isinstance(notice_path, str):
            _fail(f"{owner}: notice fields must be strings")
        if attribution:
            if redistribution == "approved" and (not notice or not notice_path):
                _fail(f"{owner}: approved attributed asset requires notice and notice_path")
            if notice_path:
                _safe_relative(notice_path, owner)
        elif notice or notice_path:
            _fail(f"{owner}: notice metadata conflicts with attribution_required=false")

        for selected_path in paths:
            folded = selected_path.casefold()
            previous_case = case_paths.get(folded)
            if previous_case is not None:
                _fail(f"case-insensitive path collision: {previous_case!r} and {selected_path!r}")
            case_paths[folded] = selected_path
            if path_kind == "prefix":
                for exact, exact_owner in exact_paths.items():
                    if exact.startswith(selected_path):
                        _fail(f"overlapping rules: {selected_path!r} ({asset_id}) and {exact!r} ({exact_owner})")
                for prefix, prefix_owner in prefixes.items():
                    if selected_path.startswith(prefix) or prefix.startswith(selected_path):
                        _fail(f"overlapping prefixes: {selected_path!r} ({asset_id}) and {prefix!r} ({prefix_owner})")
                prefixes[selected_path] = asset_id
            else:
                if selected_path in exact_paths:
                    _fail(f"duplicate asset path: {selected_path}")
                for prefix, prefix_owner in prefixes.items():
                    if selected_path.startswith(prefix):
                        _fail(f"overlapping rules: {selected_path!r} ({asset_id}) and {prefix!r} ({prefix_owner})")
                exact_paths[selected_path] = asset_id

        assets.append(Asset(
            asset_id, paths, path_kind, category, origin, creator, license_id,
            redistribution, evidence, digest, attribution, notice, notice_path,
            tuple(raw_asset_profiles), notes,
        ))

    by_id = {asset.id: asset for asset in assets}
    exact_assets = {
        selected_path: asset
        for asset in assets if asset.path_kind != "prefix"
        for selected_path in asset.paths
    }
    for asset in assets:
        if asset.redistribution != "approved" or not asset.attribution_required:
            continue
        notice_asset = exact_assets.get(asset.notice_path)
        if notice_asset is None or notice_asset.redistribution != "approved":
            _fail(f"asset {asset.id!r}: notice_path is not an approved exact asset: {asset.notice_path}")
        if not set(asset.profiles).issubset(notice_asset.profiles):
            _fail(f"asset {asset.id!r}: notice_path is not approved for every selected profile")
    for profile in profiles.values():
        for asset_id in profile.required_assets:
            asset = by_id.get(asset_id)
            if asset is None:
                _fail(f"profile {profile.name!r}: required asset does not exist: {asset_id}")
            if profile.name not in asset.profiles or asset.redistribution != "approved":
                _fail(f"profile {profile.name!r}: required asset is not approved for profile: {asset_id}")

    return Manifest(path, repository_root, name, purpose, max_files,
                    max_file_size, max_total_size, profiles, tuple(assets))


def _tracked_paths(repository_root: Path) -> set[str] | None:
    try:
        proc = subprocess.run(
            ["git", "-C", str(repository_root), "ls-files", "-z"],
            check=False, capture_output=True, timeout=10,
        )
    except (OSError, subprocess.TimeoutExpired):
        return None
    if proc.returncode != 0:
        return None
    return {item.decode("utf-8", "surrogateescape") for item in proc.stdout.split(b"\0") if item}


def inventory_text(manifest: Manifest) -> str:
    tracked = _tracked_paths(manifest.repository_root)
    counts = {state: 0 for state in sorted(STATES)}
    lines = [f"Distribution inventory: {manifest.name}", "default_policy=deny"]
    for asset in sorted(manifest.assets, key=lambda item: item.id):
        counts[asset.redistribution] += 1
        if tracked is None:
            tracked_label = "unknown"
        elif asset.path_kind == "prefix":
            tracked_label = str(sum(path.startswith(asset.paths[0]) for path in tracked))
        else:
            tracked_label = str(sum(path in tracked for path in asset.paths))
        selector = asset.paths[0] if len(asset.paths) == 1 else f"{len(asset.paths)} exact paths"
        lines.append(
            f"{asset.id}: {asset.redistribution} type={asset.category} "
            f"selector={selector} tracked={tracked_label}"
        )
    lines.append("Summary: " + " ".join(f"{state}={counts[state]}" for state in sorted(counts)))
    lines.append("Tracked counts describe Git paths only; untracked local files are not inventoried or approved.")
    return "\n".join(lines) + "\n"


def notices_text(manifest: Manifest, profile_name: str) -> str:
    profile = manifest.profiles.get(profile_name)
    if profile is None:
        _fail(f"unsupported release profile: {profile_name}")
    selected = [
        asset for asset in manifest.assets
        if profile_name in asset.profiles and asset.redistribution == "approved" and asset.attribution_required
    ]
    lines = [f"Required notices for profile: {profile_name}"]
    for asset in sorted(selected, key=lambda item: item.id):
        lines.extend([
            f"- {asset.id}",
            f"  creator: {asset.creator}",
            f"  license: {asset.license}",
            f"  notice_path: {asset.notice_path}",
            f"  requirement: {asset.notice}",
        ])
    if not selected:
        lines.append("(none)")
    return "\n".join(lines) + "\n"


def _asset_for_path(manifest: Manifest, relative: str) -> Asset | None:
    for asset in manifest.assets:
        if asset.path_kind == "prefix":
            if relative.startswith(asset.paths[0]):
                return asset
        elif relative in asset.paths:
            return asset
    return None


def _hash_open_file(path: Path, expected_stat: os.stat_result, max_size: int) -> str:
    if expected_stat.st_size > max_size:
        _fail(f"file exceeds {max_size} byte limit: {path}")
    if expected_stat.st_mode & 0o444 == 0:
        _fail(f"file has no readable permission bits: {path}")
    flags = os.O_RDONLY
    if hasattr(os, "O_NOFOLLOW"):
        flags |= os.O_NOFOLLOW
    try:
        fd = os.open(path, flags)
    except OSError as exc:
        _fail(f"cannot open staged file {path}: {exc}")
    digest = hashlib.sha256()
    try:
        before = os.fstat(fd)
        identity = (expected_stat.st_dev, expected_stat.st_ino, expected_stat.st_size, expected_stat.st_mtime_ns)
        opened = (before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns)
        if identity != opened or not stat.S_ISREG(before.st_mode):
            _fail(f"staged file changed before verification: {path}")
        with os.fdopen(fd, "rb", closefd=False) as stream:
            while True:
                chunk = stream.read(READ_CHUNK_BYTES)
                if not chunk:
                    break
                digest.update(chunk)
        after = os.fstat(fd)
        final = (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns)
        if opened != final:
            _fail(f"staged file changed during verification: {path}")
    finally:
        os.close(fd)
    return digest.hexdigest()


def verify_tree(manifest: Manifest, profile_name: str, root: Path) -> tuple[int, int]:
    profile = manifest.profiles.get(profile_name)
    if profile is None:
        _fail(f"unsupported release profile: {profile_name}")
    try:
        root_lstat = root.lstat()
    except OSError as exc:
        _fail(f"cannot inspect staging root {root}: {exc}")
    if stat.S_ISLNK(root_lstat.st_mode) or not stat.S_ISDIR(root_lstat.st_mode):
        _fail(f"staging root must be a real directory, not a symlink or special file: {root}")

    seen_case: dict[str, str] = {}
    seen_assets: set[str] = set()
    verified_paths: set[str] = set()
    staged_notice_dependencies: dict[str, str] = {}
    file_count = 0
    total_size = 0
    staged_files: list[tuple[str, Path, os.stat_result]] = []

    def walk(directory: Path, relative_dir: str) -> None:
        nonlocal file_count, total_size
        try:
            entries = sorted(os.scandir(directory), key=lambda item: (item.name.casefold(), item.name))
        except OSError as exc:
            _fail(f"cannot read staging directory {directory}: {exc}")
        if relative_dir and not entries:
            _fail(f"unexpected empty directory in staging tree: {relative_dir}")
        for entry in entries:
            relative = f"{relative_dir}/{entry.name}" if relative_dir else entry.name
            relative = relative.replace(os.sep, "/")
            folded = relative.casefold()
            previous = seen_case.get(folded)
            if previous is not None:
                _fail(f"case-insensitive staging collision: {previous!r} and {relative!r}")
            seen_case[folded] = relative
            try:
                item_stat = entry.stat(follow_symlinks=False)
            except OSError as exc:
                _fail(f"cannot stat staged path {relative}: {exc}")
            if stat.S_ISLNK(item_stat.st_mode):
                _fail(f"symlinks are prohibited in staging trees: {relative}")
            if stat.S_ISDIR(item_stat.st_mode):
                walk(Path(entry.path), relative)
                continue
            if not stat.S_ISREG(item_stat.st_mode):
                _fail(f"unsupported special file in staging tree: {relative}")

            file_count += 1
            if file_count > manifest.max_files:
                _fail(f"staging tree exceeds {manifest.max_files} file limit")
            total_size += item_stat.st_size
            if total_size > manifest.max_total_size:
                _fail(f"staging tree exceeds {manifest.max_total_size} byte total limit")
            staged_files.append((relative, Path(entry.path), item_stat))

    walk(root, "")
    # Inspect the complete namespace before applying per-file policy.  This
    # ensures a case collision cannot be hidden by an earlier unknown file.
    for relative, staged_path, item_stat in staged_files:
        asset = _asset_for_path(manifest, relative)
        if asset is None:
            _fail(f"unlisted staged file rejected by default-deny policy: {relative}")
        if profile_name not in asset.profiles:
            _fail(f"asset is not eligible for profile {profile_name}: {relative}")
        if asset.redistribution != "approved":
            _fail(f"{asset.redistribution} asset rejected for {profile_name}: {relative} ({asset.id})")
        actual_digest = _hash_open_file(staged_path, item_stat, manifest.max_file_size)
        if actual_digest != asset.sha256:
            _fail(f"sha256 mismatch for {relative}: expected {asset.sha256}, got {actual_digest}")
        seen_assets.add(asset.id)
        verified_paths.add(relative)
        if asset.attribution_required:
            staged_notice_dependencies[asset.id] = asset.notice_path
    missing_notices = sorted(
        f"{asset_id} requires {notice_path}"
        for asset_id, notice_path in staged_notice_dependencies.items()
        if notice_path not in verified_paths
    )
    if missing_notices:
        _fail("staging tree is missing verified notice dependency: " + "; ".join(missing_notices))
    missing = sorted(set(profile.required_assets) - seen_assets)
    if missing:
        _fail(f"staging tree is missing required asset(s) for {profile_name}: {', '.join(missing)}")
    return file_count, total_size


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate distributable assets and staged release trees.")
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--repository-root", type=Path, default=ROOT)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("check", help="validate manifest structure and policy")
    sub.add_parser("inventory", help="print deterministic manifest inventory")
    verify = sub.add_parser("verify", help="validate an actual release staging tree")
    verify.add_argument("--profile", required=True)
    verify.add_argument("--root", required=True, type=Path)
    notices = sub.add_parser("notices", help="print required notices for a release profile")
    notices.add_argument("--profile", required=True)
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        manifest = load_manifest(args.manifest, args.repository_root)
        if args.command == "check":
            print(f"PASS manifest schema=1 assets={len(manifest.assets)} profiles={len(manifest.profiles)} default_policy=deny")
        elif args.command == "inventory":
            sys.stdout.write(inventory_text(manifest))
        elif args.command == "notices":
            sys.stdout.write(notices_text(manifest, args.profile))
        elif args.command == "verify":
            count, size = verify_tree(manifest, args.profile, args.root)
            print(f"PASS profile={args.profile} files={count} bytes={size} root={args.root}")
        return 0
    except DistributionError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
