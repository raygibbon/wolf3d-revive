import hashlib
import importlib.util
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "validate_distribution.py"


def load_tool():
    spec = importlib.util.spec_from_file_location("w3r_distribution", TOOL)
    module = importlib.util.module_from_spec(spec)
    assert spec and spec.loader
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


DIST = load_tool()


def asset_block(asset_id, path, state, *, digest="", license_id="MIT",
                attribution="false", notice_path="legal/NOTICE",
                evidence="evidence.txt", extra=""):
    digest_line = f'sha256 = "{digest}"\n' if digest else ""
    notice_lines = ""
    if attribution == "true":
        notice_lines = f'notice = "Retain the notice."\nnotice_path = "{notice_path}"\n'
    return f'''\n[[asset]]
id = "{asset_id}"
path = "{path}"
type = "test-asset"
origin = "synthetic test"
creator = "test suite"
license = "{license_id}"
redistribution = "{state}"
evidence = "{evidence}"
{digest_line}attribution_required = {attribution}
{notice_lines}profiles = ["original-standalone"]
notes = "Synthetic policy fixture."
{extra}'''


class DistributionFixture(unittest.TestCase):
    content = b"approved synthetic notice\n"

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.stage = self.root / "stage"
        self.stage.mkdir()
        (self.root / "evidence.txt").write_text("synthetic provenance\n", encoding="utf-8")
        self.digest = hashlib.sha256(self.content).hexdigest()
        self.manifest_path = self.root / "distribution.toml"
        self.write_manifest()

    def tearDown(self):
        self.temp.cleanup()

    def base_manifest(self):
        return f'''schema_version = 1

[distribution]
name = "synthetic"
purpose = "test"
default_policy = "deny"

[limits]
max_files = 32
max_file_size = 1048576
max_total_size = 2097152

[profiles.original-standalone]
description = "synthetic test profile"
required_assets = ["approved-notice"]
{asset_block("approved-notice", "legal/NOTICE", "approved", digest=self.digest, attribution="true")}
'''

    def write_manifest(self, text=None):
        self.manifest_path.write_text(text if text is not None else self.base_manifest(), encoding="utf-8")

    def separate_notice_manifest(self):
        payload = b"approved synthetic artwork\n"
        notice = b"reviewed synthetic licence notice\n"
        payload_digest = hashlib.sha256(payload).hexdigest()
        notice_digest = hashlib.sha256(notice).hexdigest()
        text = self.base_manifest().replace(
            'required_assets = ["approved-notice"]',
            'required_assets = ["approved-payload"]',
        )
        start = text.index("\n[[asset]]")
        text = text[:start]
        text += asset_block(
            "approved-payload", "art/payload.bin", "approved",
            digest=payload_digest, attribution="true", notice_path="legal/LICENSE",
        )
        text += asset_block(
            "payload-license", "legal/LICENSE", "approved", digest=notice_digest,
        )
        return text, payload, notice

    def manifest(self):
        return DIST.load_manifest(self.manifest_path, self.root)

    def stage_approved(self, content=None):
        target = self.stage / "legal" / "NOTICE"
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(self.content if content is None else content)
        return target

    def cli(self, *arguments):
        return subprocess.run(
            [sys.executable, str(TOOL), "--manifest", str(self.manifest_path),
             "--repository-root", str(self.root), *arguments],
            text=True, capture_output=True, timeout=20,
        )

    def assert_manifest_error(self, text, fragment):
        self.write_manifest(text)
        with self.assertRaisesRegex(DIST.DistributionError, fragment):
            self.manifest()


class ManifestStructureTests(DistributionFixture):
    def test_valid_manifest_and_supported_schema_parse(self):
        manifest = self.manifest()
        self.assertEqual(manifest.name, "synthetic")
        self.assertEqual(manifest.profiles["original-standalone"].required_assets,
                         ("approved-notice",))

    def test_malformed_toml_fails_with_useful_cli_error(self):
        self.write_manifest("schema_version = [")
        result = self.cli("check")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("cannot parse manifest", result.stderr)
        self.assertNotIn("Traceback", result.stderr)

    def test_unsupported_schema_version_rejected(self):
        self.assert_manifest_error(self.base_manifest().replace("schema_version = 1", "schema_version = 9"),
                                   "unsupported schema_version")

    def test_duplicate_asset_id_rejected(self):
        text = self.base_manifest() + asset_block("approved-notice", "other.dat", "prohibited")
        self.assert_manifest_error(text, "duplicate asset ID")

    def test_duplicate_path_rejected(self):
        text = self.base_manifest() + asset_block("other", "legal/NOTICE", "prohibited")
        self.assert_manifest_error(text, "collision|duplicate asset path")

    def test_overlapping_prefix_rule_rejected(self):
        block = '''
[[asset]]
id = "overlap"
prefix = "legal/"
type = "test"
origin = "synthetic"
creator = "test"
license = "NOASSERTION"
redistribution = "prohibited"
evidence = ""
attribution_required = false
profiles = ["original-standalone"]
notes = "overlap"
'''
        self.assert_manifest_error(self.base_manifest() + block, "overlapping rules")

    def test_malformed_prefix_rejected(self):
        text = self.base_manifest().replace('path = "legal/NOTICE"', 'prefix = "legal"')
        self.assert_manifest_error(text, "prefix must end")

    def test_missing_mandatory_field_rejected(self):
        text = self.base_manifest().replace('origin = "synthetic test"\n', "", 1)
        self.assert_manifest_error(text, "missing mandatory field 'origin'")

    def test_unknown_redistribution_status_rejected(self):
        text = self.base_manifest().replace('redistribution = "approved"', 'redistribution = "maybe"')
        self.assert_manifest_error(text, "unknown redistribution status")

    def test_unknown_licence_state_rejected(self):
        text = self.base_manifest().replace('license = "MIT"', 'license = "looks-free"')
        self.assert_manifest_error(text, "unknown licence state")

    def test_reviewed_cc0_spdx_identifier_can_be_approved(self):
        self.manifest_path.write_text(
            self.base_manifest().replace('license = "MIT"', 'license = "CC0-1.0"'),
            encoding="utf-8",
        )
        self.assertEqual(self.manifest().assets[0].license, "CC0-1.0")

    def test_reviewed_cc_by_spdx_identifier_can_be_approved(self):
        self.manifest_path.write_text(
            self.base_manifest().replace('license = "MIT"', 'license = "CC-BY-4.0"'),
            encoding="utf-8",
        )
        self.assertEqual(self.manifest().assets[0].license, "CC-BY-4.0")

    def test_reviewed_ofl_spdx_identifier_can_be_approved(self):
        self.manifest_path.write_text(
            self.base_manifest().replace('license = "MIT"', 'license = "OFL-1.1"'),
            encoding="utf-8",
        )
        self.assertEqual(self.manifest().assets[0].license, "OFL-1.1")

    def test_unresolved_or_restricted_licence_cannot_grant_approval(self):
        for licence in (
            "NOASSERTION",
            "LicenseRef-Commercial-Proprietary",
            "LicenseRef-Historical-Unverified",
            "LicenseRef-Pending-Review",
        ):
            with self.subTest(licence=licence):
                text = self.base_manifest().replace('license = "MIT"', f'license = "{licence}"')
                self.assert_manifest_error(text, "non-approvable licence state")

    def test_invalid_sha256_rejected(self):
        text = self.base_manifest().replace(self.digest, "1234")
        self.assert_manifest_error(text, "sha256")

    def test_missing_provenance_rejected(self):
        text = self.base_manifest().replace('evidence = "evidence.txt"', 'evidence = ""')
        self.assert_manifest_error(text, "requires provenance evidence")

    def test_missing_evidence_file_rejected(self):
        text = self.base_manifest().replace('evidence = "evidence.txt"', 'evidence = "missing.txt"')
        self.assert_manifest_error(text, "provenance evidence does not exist")

    def test_unsafe_evidence_path_rejected(self):
        text = self.base_manifest().replace('evidence = "evidence.txt"',
                                            'evidence = "../evidence.txt"')
        self.assert_manifest_error(text, "unsafe relative path")

    def test_missing_required_attribution_rejected(self):
        text = self.base_manifest().replace('notice = "Retain the notice."\n', "")
        self.assert_manifest_error(text, "requires notice and notice_path")

    def test_unregistered_notice_path_rejected(self):
        text = self.base_manifest().replace('notice_path = "legal/NOTICE"',
                                            'notice_path = "missing/NOTICE"')
        self.assert_manifest_error(text, "notice_path is not an approved exact asset")

    def test_notice_metadata_conflict_rejected(self):
        text = self.base_manifest().replace("attribution_required = true",
                                            "attribution_required = false")
        self.assert_manifest_error(text, "conflicts with attribution_required=false")

    def test_unknown_profile_reference_rejected(self):
        text = self.base_manifest().replace('profiles = ["original-standalone"]', 'profiles = ["missing"]')
        self.assert_manifest_error(text, "unknown profile reference")

    def test_path_traversal_rejected(self):
        text = self.base_manifest().replace('path = "legal/NOTICE"', 'path = "../NOTICE"')
        self.assert_manifest_error(text, "unsafe relative path")

    def test_absolute_path_rejected(self):
        text = self.base_manifest().replace('path = "legal/NOTICE"', 'path = "/tmp/NOTICE"')
        self.assert_manifest_error(text, "absolute path")

    def test_windows_drive_path_rejected(self):
        text = self.base_manifest().replace('path = "legal/NOTICE"', 'path = "C:/NOTICE"')
        self.assert_manifest_error(text, "drive-qualified")

    def test_mixed_separator_rejected(self):
        text = self.base_manifest().replace('path = "legal/NOTICE"', 'path = "legal\\\\NOTICE"')
        self.assert_manifest_error(text, "separators")


class ReleaseVerificationTests(DistributionFixture):
    def test_valid_approved_asset_and_correct_sha_pass(self):
        self.stage_approved()
        self.assertEqual(DIST.verify_tree(self.manifest(), "original-standalone", self.stage),
                         (1, len(self.content)))

    def test_cli_verifies_actual_staging_path(self):
        self.stage_approved()
        result = self.cli("verify", "--profile", "original-standalone", "--root", str(self.stage))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("files=1", result.stdout)

    def test_staged_attributed_asset_requires_separate_notice(self):
        text, payload, _ = self.separate_notice_manifest()
        self.write_manifest(text)
        target = self.stage / "art" / "payload.bin"
        target.parent.mkdir(parents=True)
        target.write_bytes(payload)
        with self.assertRaisesRegex(DIST.DistributionError, "missing verified notice dependency"):
            DIST.verify_tree(self.manifest(), "original-standalone", self.stage)

    def test_staged_attributed_asset_with_verified_notice_passes(self):
        text, payload, notice = self.separate_notice_manifest()
        self.write_manifest(text)
        payload_path = self.stage / "art" / "payload.bin"
        notice_path = self.stage / "legal" / "LICENSE"
        payload_path.parent.mkdir(parents=True)
        notice_path.parent.mkdir(parents=True)
        payload_path.write_bytes(payload)
        notice_path.write_bytes(notice)
        count, total = DIST.verify_tree(self.manifest(), "original-standalone", self.stage)
        self.assertEqual((count, total), (2, len(payload) + len(notice)))

    def test_staged_attributed_asset_with_tampered_notice_fails(self):
        text, payload, _ = self.separate_notice_manifest()
        self.write_manifest(text)
        payload_path = self.stage / "art" / "payload.bin"
        notice_path = self.stage / "legal" / "LICENSE"
        payload_path.parent.mkdir(parents=True)
        notice_path.parent.mkdir(parents=True)
        payload_path.write_bytes(payload)
        notice_path.write_bytes(b"tampered notice\n")
        with self.assertRaisesRegex(DIST.DistributionError, "sha256 mismatch for legal/LICENSE"):
            DIST.verify_tree(self.manifest(), "original-standalone", self.stage)

    def test_empty_staging_fails_missing_required_assets(self):
        with self.assertRaisesRegex(DIST.DistributionError, "missing required asset"):
            DIST.verify_tree(self.manifest(), "original-standalone", self.stage)

    def test_hash_mismatch_fails(self):
        self.stage_approved(b"modified")
        with self.assertRaisesRegex(DIST.DistributionError, "sha256 mismatch"):
            DIST.verify_tree(self.manifest(), "original-standalone", self.stage)

    def test_unknown_staged_file_fails_closed(self):
        (self.stage / "unknown.png").write_bytes(b"synthetic")
        with self.assertRaisesRegex(DIST.DistributionError, "unlisted staged file"):
            DIST.verify_tree(self.manifest(), "original-standalone", self.stage)

    def _assert_state_rejected(self, state):
        text = self.base_manifest() + asset_block(f"state-{state}", "blocked.dat", state)
        self.write_manifest(text)
        (self.stage / "blocked.dat").write_bytes(b"x")
        with self.assertRaisesRegex(DIST.DistributionError, state):
            DIST.verify_tree(self.manifest(), "original-standalone", self.stage)

    def test_prohibited_file_rejected(self):
        self._assert_state_rejected("prohibited")

    def test_pending_review_file_rejected(self):
        self._assert_state_rejected("pending-review")

    def test_development_only_file_rejected(self):
        self._assert_state_rejected("development-only")

    def test_symlink_to_outside_rejected(self):
        outside = self.root / "outside"
        outside.write_bytes(self.content)
        (self.stage / "legal").mkdir()
        (self.stage / "legal" / "NOTICE").symlink_to(outside)
        with self.assertRaisesRegex(DIST.DistributionError, "symlinks are prohibited"):
            DIST.verify_tree(self.manifest(), "original-standalone", self.stage)

    def test_symlink_staging_root_rejected(self):
        real = self.root / "real-stage"
        real.mkdir()
        linked = self.root / "linked-stage"
        linked.symlink_to(real, target_is_directory=True)
        with self.assertRaisesRegex(DIST.DistributionError, "staging root"):
            DIST.verify_tree(self.manifest(), "original-standalone", linked)

    def test_case_insensitive_collision_rejected_before_default_deny(self):
        (self.stage / "Case.dat").write_bytes(b"a")
        (self.stage / "case.dat").write_bytes(b"b")
        with self.assertRaisesRegex(DIST.DistributionError, "case-insensitive staging collision"):
            DIST.verify_tree(self.manifest(), "original-standalone", self.stage)

    def test_unreadable_approved_file_rejected(self):
        target = self.stage_approved()
        target.chmod(0)
        try:
            with self.assertRaisesRegex(DIST.DistributionError, "no readable permission"):
                DIST.verify_tree(self.manifest(), "original-standalone", self.stage)
        finally:
            target.chmod(0o600)

    def test_unexpected_empty_directory_rejected(self):
        (self.stage / "backup").mkdir()
        with self.assertRaisesRegex(DIST.DistributionError, "unexpected empty directory"):
            DIST.verify_tree(self.manifest(), "original-standalone", self.stage)

    @unittest.skipUnless(hasattr(os, "mkfifo"), "FIFO creation unavailable")
    def test_special_file_rejected(self):
        os.mkfifo(self.stage / "pipe")
        with self.assertRaisesRegex(DIST.DistributionError, "unsupported special file"):
            DIST.verify_tree(self.manifest(), "original-standalone", self.stage)

    def test_invalid_profile_rejected(self):
        with self.assertRaisesRegex(DIST.DistributionError, "unsupported release profile"):
            DIST.verify_tree(self.manifest(), "legacy-bundle", self.stage)

    def test_restricted_looking_resource_renamed_is_still_unknown(self):
        (self.stage / "vacation-photo.bin").write_bytes(b"synthetic stand-in for commercial data")
        with self.assertRaisesRegex(DIST.DistributionError, "default-deny"):
            DIST.verify_tree(self.manifest(), "original-standalone", self.stage)

    def test_oversize_input_rejected(self):
        self.write_manifest(self.base_manifest().replace("max_file_size = 1048576", "max_file_size = 4"))
        self.stage_approved()
        with self.assertRaisesRegex(DIST.DistributionError, "file exceeds"):
            DIST.verify_tree(self.manifest(), "original-standalone", self.stage)

    def test_cli_failure_is_nonzero_and_names_policy(self):
        (self.stage / "unknown.dat").write_bytes(b"x")
        result = self.cli("verify", "--profile", "original-standalone", "--root", str(self.stage))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("default-deny", result.stderr)


class InventoryNoticeAndProjectTests(DistributionFixture):
    def test_inventory_output_is_deterministic(self):
        manifest = self.manifest()
        self.assertEqual(DIST.inventory_text(manifest), DIST.inventory_text(manifest))
        self.assertIn("approved=1", DIST.inventory_text(manifest))

    def test_notice_generation_is_deterministic_and_complete(self):
        manifest = self.manifest()
        first = DIST.notices_text(manifest, "original-standalone")
        self.assertEqual(first, DIST.notices_text(manifest, "original-standalone"))
        self.assertIn("legal/NOTICE", first)
        self.assertIn("Retain the notice", first)

    def test_notices_reject_invalid_profile(self):
        with self.assertRaisesRegex(DIST.DistributionError, "unsupported release profile"):
            DIST.notices_text(self.manifest(), "missing")

    def test_existing_project_distribution_configuration_validates(self):
        manifest = DIST.load_manifest(ROOT / "assets" / "distribution.toml", ROOT)
        self.assertIn("original-standalone", manifest.profiles)
        self.assertEqual(sum(a.redistribution == "approved" for a in manifest.assets), 2)

    def test_project_notices_cli_succeeds(self):
        result = subprocess.run(
            [sys.executable, str(TOOL), "notices", "--profile", "original-standalone"],
            cwd=ROOT, text=True, capture_output=True, timeout=20,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("emu8950-mit-notice", result.stdout)

    def test_project_manifest_valid_payload_checks_real_files(self):
        with tempfile.TemporaryDirectory() as temporary:
            stage = Path(temporary)
            (stage / "third_party" / "emu8950").mkdir(parents=True)
            (stage / "COPYING").write_bytes((ROOT / "COPYING").read_bytes())
            (stage / "third_party" / "emu8950" / "LICENSE").write_bytes(
                (ROOT / "third_party" / "emu8950" / "LICENSE").read_bytes()
            )
            manifest = DIST.load_manifest(ROOT / "assets" / "distribution.toml", ROOT)
            count, _ = DIST.verify_tree(manifest, "original-standalone", stage)
            self.assertEqual(count, 2)


if __name__ == "__main__":
    unittest.main()
