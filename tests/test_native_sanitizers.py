# SPDX-License-Identifier: MIT
"""The strict lane must instrument both the owner and its upstream fixture."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location('native_matrix',
                                             Path(__file__).with_name('native-matrix.py'))
MATRIX = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MATRIX)


class StrictSanitizers(unittest.TestCase):
    def fixture(self, root, recovery=False, flags='-fsanitize=address,undefined -fno-sanitize-recover=all',
                omit_core=False):
        (root / 'zephyr').mkdir()
        (root / 'zephyr/.config').write_text('CONFIG_ASAN=y\nCONFIG_UBSAN=y\n' +
            ('CONFIG_ASAN_RECOVER=y\n' if recovery else '# CONFIG_ASAN_RECOVER is not set\n'))
        names = ['input_processor_suppress.c'] + ([] if omit_core else ['kscan_mock.c'])
        (root / 'compile_commands.json').write_text(json.dumps([
            {'file': '/synthetic/' + name, 'command': 'cc ' + flags + ' -c ' + name}
            for name in names]))

    def test_both_instrumented_nonrecovering_inputs_are_accepted(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.fixture(root)
            MATRIX.check_sanitizers(root)

    def test_recovery_configuration_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.fixture(root, recovery=True)
            with self.assertRaisesRegex(ValueError, 'without recovery'):
                MATRIX.check_sanitizers(root)

    def test_missing_instrumentation_and_recovery_flags_are_rejected(self):
        for flags in ('-fno-sanitize-recover=all', '-fsanitize=address,undefined',
                      '-fsanitize=address,undefined -fno-sanitize-recover=all -fsanitize-recover=all'):
            with self.subTest(flags=flags), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                self.fixture(root, flags=flags)
                with self.assertRaisesRegex(ValueError, 'compiler flags'):
                    MATRIX.check_sanitizers(root)

    def test_omitting_upstream_instrumentation_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.fixture(root, omit_core=True)
            with self.assertRaisesRegex(ValueError, 'upstream compiler input'):
                MATRIX.check_sanitizers(root)


if __name__ == '__main__':
    unittest.main()
