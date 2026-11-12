# SPDX-License-Identifier: MIT
"""Exercise public native entry admission before any external build command."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class CanonicalAdmission(unittest.TestCase):
    def invoke(self, script, workspace, output, **environment):
        return subprocess.run(['bash', str(ROOT / 'tests' / script), str(workspace)],
                              env={**os.environ, 'ZMK_TEST_OUTPUT': str(output), **environment},
                              text=True, capture_output=True)

    def test_ambient_profile_cannot_omit_original_cases(self):
        with tempfile.TemporaryDirectory() as temporary:
            workspace = Path(temporary)
            for script in ('run-native.sh', 'run-native-sanitized.sh'):
                for selector in ('ZMK_TEST_NATIVE_PROFILE', 'ZMK_TEST_SOURCE_PROFILE',
                                 'ZMK_TEST_MAX_SOURCES', 'ZMK_TEST_WORKSPACE_NEGATIVE_CONTROL'):
                    with self.subTest(script=script, selector=selector):
                        output = workspace / 'fresh'
                        result = self.invoke(script, workspace, output, **{selector: 'limited-only'})
                        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                        self.assertIn('rejects ambient ' + selector, result.stderr)
                        self.assertNotIn('PASS', result.stdout)
                        self.assertFalse(output.exists())

    def test_existing_evidence_cannot_be_reused(self):
        with tempfile.TemporaryDirectory() as temporary:
            workspace = Path(temporary)
            output = workspace / 'existing'
            output.mkdir()
            for script in ('run-native.sh', 'run-native-sanitized.sh'):
                with self.subTest(script=script):
                    result = self.invoke(script, workspace, output)
                    self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                    self.assertIn('fresh absolute output required', result.stderr)
                    self.assertEqual(list(output.iterdir()), [])


if __name__ == '__main__':
    unittest.main()
