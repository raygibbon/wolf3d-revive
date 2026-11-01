# Repository safeguards

Inspected on 2026-11-01 before source changes: clean `master` at `0516778`,
tracking `origin/master`. `origin` already pointed to
`https://github.com/raygibbon/wolf3d-revive.git`. No existing remote URL,
branch, commit or hook was replaced. CART's working tree was also clean.

Local configuration now has:

| Setting | Value |
| --- | --- |
| origin fetch/push | `https://github.com/raygibbon/wolf3d-revive.git` |
| upstream fetch | `https://github.com/id-Software/wolf3d.git` |
| upstream push | `https://invalid.invalid/upstream-push-disabled` |
| remote.pushDefault | `origin` |
| push.default | `simple` |
| core.hooksPath | `.githooks` |

The executable `.githooks/pre-push` rejects destinations referencing
`id-Software/wolf3d`, ignoring case, including HTTPS, SCP-style SSH and
`ssh://` URLs. It checks both Git hook arguments and all effective push URLs
of a named remote, including URL rewrites. Tests invoke the hook directly in
temporary repositories. No push, commit, upstream fetch, or PR was performed.

Verify locally without network activity:

```sh
git remote -v
git config --local --get remote.pushDefault
git config --local --get push.default
git config --local --get core.hooksPath
python3 -m unittest discover -s tests -v
```

Git config is repository-local and is not transported by cloning. Future
checkouts must inspect their existing remotes and hooks before applying these
settings. Hooks can be deliberately bypassed; this guard prevents accidents,
and is not server-side access control. No pushes are part of this task.
