"""Reproducibility/fail-closed checks for the pinned LVGL hardening script."""
from pathlib import Path
import importlib.util
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location("patch_lvgl", ROOT / "tools/patch_lvgl.py")
patch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(patch)


class PatchTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Read the pinned dependency, never modify it. All patch operations in
        # this test are confined to an independently owned temporary directory.
        cls.originals = {}
        lvgl = ROOT / "targets/waveshare_p4/managed_components/lvgl__lvgl"
        for relative, changes in patch.replacements().items():
            text = (lvgl / relative).read_text(encoding="utf-8")
            prefix = "// %s\n" % patch.MARKER
            if text.startswith(prefix):
                text = text[len(prefix):]
                for original, patched, count in reversed(changes):
                    if text.count(patched) != count:
                        raise RuntimeError("Incomplete installed LVGL patch")
                    text = text.replace(patched, original)
            cls.originals[relative] = text

    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="aurora-lvgl-test-")
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.version = self.root / "lv_version.h"
        self.version.write_text("#define LVGL_VERSION_MAJOR 9\n"
                                "#define LVGL_VERSION_MINOR 5\n"
                                "#define LVGL_VERSION_PATCH 0\n", encoding="utf-8")
        for relative, text in self.originals.items():
            path = self.root / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text, encoding="utf-8")

    def test_apply_and_idempotence(self):
        self.assertTrue(patch.patch_tree(self.root))
        self.assertFalse(patch.patch_tree(self.root))
        for relative, changes in patch.replacements().items():
            text = (self.root / relative).read_text(encoding="utf-8")
            for _, expected, count in changes:
                self.assertEqual(text.count(expected), count)

    def test_changed_version_rejected(self):
        self.version.write_text("#define LVGL_VERSION_MAJOR 10\n", encoding="utf-8")
        with self.assertRaises(RuntimeError):
            patch.patch_tree(self.root)

    def test_changed_source_does_not_partially_patch(self):
        textarea = self.root / "src/widgets/textarea/lv_textarea.c"
        original = textarea.read_bytes()
        (self.root / "src/misc/lv_assert.h").write_text("unknown source", encoding="utf-8")
        with self.assertRaises(RuntimeError):
            patch.patch_tree(self.root)
        self.assertEqual(textarea.read_bytes(), original)

    def test_tampered_patch_rejected(self):
        patch.patch_tree(self.root)
        textarea = self.root / "src/widgets/textarea/lv_textarea.c"
        textarea.write_text(textarea.read_text(encoding="utf-8").replace(
            "ta->pwd_tmp = new_pwd_tmp;", "/* tampered */", 1), encoding="utf-8")
        with self.assertRaises(RuntimeError):
            patch.patch_tree(self.root)

    def test_unrelated_pristine_tamper_rejected_without_partial_write(self):
        textarea = self.root / "src/widgets/textarea/lv_textarea.c"
        original = textarea.read_bytes()
        header = self.root / "src/misc/lv_assert.h"
        header.write_text(header.read_text(encoding="utf-8") + "\n/* unrelated tamper */\n",
                          encoding="utf-8")
        with self.assertRaises(RuntimeError):
            patch.patch_tree(self.root)
        self.assertEqual(textarea.read_bytes(), original)

    def test_unrelated_patched_tamper_rejected(self):
        patch.patch_tree(self.root)
        textarea = self.root / "src/widgets/textarea/lv_textarea.c"
        textarea.write_text(textarea.read_text(encoding="utf-8") + "\n/* unrelated tamper */\n",
                            encoding="utf-8")
        with self.assertRaises(RuntimeError):
            patch.patch_tree(self.root)

    def test_line_ending_normalization(self):
        for relative in patch.replacements():
            path = self.root / relative
            path.write_bytes(path.read_text(encoding="utf-8").replace("\n", "\r\n").encode("utf-8"))
        self.assertTrue(patch.patch_tree(self.root))
        self.assertFalse(patch.patch_tree(self.root))


if __name__ == "__main__":
    unittest.main()
