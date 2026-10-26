"""Exercise validation on real temporary source packages, including corruption."""
from pathlib import Path
import hashlib
import importlib.util
import json
import tempfile
import unittest

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("source_verifier", root / "scripts/VerifySource.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class SourcePackageTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        (self.root / "overlay").mkdir()
        (self.root / "changes.patch").write_bytes(b"patch")
        self.manifest = {"mainPatch": "changes.patch", "mainPatchSha256": hashlib.sha256(b"patch").hexdigest(), "overlayFiles": []}
        self.save()

    def save(self):
        (self.root / "SOURCE_MANIFEST.json").write_text(json.dumps(self.manifest), encoding="utf-8")

    def test_valid_minimal_package(self):
        self.assertEqual(module.validate_repository(self.root), 1)

    def test_corrupt_patch(self):
        (self.root / "changes.patch").write_bytes(b"changed")
        with self.assertRaisesRegex(ValueError, "hash mismatch"):
            module.validate_repository(self.root)

    def test_parent_path_rejected(self):
        self.manifest["mainPatch"] = "../outside.patch"
        self.save()
        with self.assertRaisesRegex(ValueError, "escapes"):
            module.validate_repository(self.root)

    def test_unlisted_overlay_rejected(self):
        (self.root / "overlay/unlisted.txt").write_text("unexpected", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "manifest differ"):
            module.validate_repository(self.root)

    def test_case_insensitive_duplicate_rejected(self):
        self.manifest["overlayFiles"] = [{"path": "x", "sha256": "bad"}, {"path": "X", "sha256": "bad"}]
        (self.root / "overlay/x").write_bytes(b"x")
        self.manifest["overlayFiles"][0]["sha256"] = hashlib.sha256(b"x").hexdigest()
        self.save()
        with self.assertRaisesRegex(ValueError, "Duplicate"):
            module.validate_repository(self.root)


if __name__ == "__main__":
    unittest.main()
