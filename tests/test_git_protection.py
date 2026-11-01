"""Exercise the pre-push hook directly; never invoke git push or a network operation."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
HOOK = ROOT / ".githooks" / "pre-push"


class PushProtection(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.cwd = self.temp.name
        self.env = dict(os.environ, GIT_CONFIG_NOSYSTEM="1", GIT_CONFIG_GLOBAL=os.devnull)
        subprocess.run(["git", "init", "-q", self.cwd], check=True, env=self.env)

    def git(self, *args):
        subprocess.run(["git", *args], cwd=self.cwd, env=self.env, check=True)

    def hook(self, remote, url):
        return subprocess.run([str(HOOK), remote, url], cwd=self.cwd, env=self.env,
                              input="", text=True, capture_output=True)

    def test_original_urls_rejected(self):
        for url in (
            "https://github.com/id-Software/wolf3d.git",
            "git@github.com:id-Software/wolf3d.git",
            "ssh://git@github.com/id-Software/wolf3d.git",
            "https://github.com/ID-SOFTWARE/WOLF3D",
            "git://github.com/id-Software/wolf3d",
        ):
            with self.subTest(url=url):
                self.assertNotEqual(self.hook("arbitrary", url).returncode, 0)

    def test_owned_origin_allowed(self):
        url = "https://github.com/raygibbon/wolf3d-revive.git"
        self.git("remote", "add", "origin", url)
        self.assertEqual(self.hook("origin", url).returncode, 0)

    def test_all_configured_push_urls_checked(self):
        url = "https://github.com/raygibbon/wolf3d-revive.git"
        self.git("remote", "add", "alias", url)
        self.git("remote", "set-url", "--add", "--push", "alias", url)
        self.git("remote", "set-url", "--add", "--push", "alias",
                 "git@github.com:id-Software/wolf3d.git")
        self.assertNotEqual(self.hook("alias", url).returncode, 0)

    def test_url_rewrite_checked(self):
        self.git("remote", "add", "alias", "wolf-original:repo")
        self.git("config", "url.https://github.com/id-Software/wolf3d.pushInsteadOf",
                 "wolf-original:")
        self.assertNotEqual(self.hook("alias", "wolf-original:repo").returncode, 0)


if __name__ == "__main__":
    unittest.main()
