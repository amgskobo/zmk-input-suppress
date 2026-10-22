# SPDX-License-Identifier: MIT
"""Exercise the public coverage validator, including optimized Python."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCES = {
    'input_processor_suppress.c.gcov': (
        'src/input_processor_suppress.c', ('neutral', 'suppress_event', 'suppress_init')),
    'core.h.gcov': ('include/zmk-input-suppress/core.h',
                    ('zmk_suppress_contact', 'zmk_suppress_button')),
}


def report(source, names, *, only_one=False, zero_call=False, missed=False, branch=False,
           missing_line=False):
    lines = (ROOT/source).read_text().splitlines()
    selected = names[:1] if only_one else names
    result = []
    for index, name in enumerate(selected):
        first = next(i + 1 for i, line in enumerate(lines) if name + '(' in line)
        result.append(f'function {name} called {0 if zero_call and index == 0 else 1}')
        if not missing_line or index != 0:
            result.append(f'{"#####" if missed and index == 0 else "1"}:{first}:{lines[first-1]}')
    result.append(f'branch 0 taken {0 if branch else 1}')
    return '\n'.join(result) + '\n'


class CoverageAdmission(unittest.TestCase):
    def invoke(self, *, optimize=False, override=None, **change):
        with tempfile.TemporaryDirectory(prefix='suppress-coverage-') as folder:
            for name, (source, functions) in SOURCES.items():
                (Path(folder)/name).write_text(
                    override if override is not None else report(source, functions, **change))
            return subprocess.run([sys.executable, *(['-O'] if optimize else []),
                str(ROOT/'tests/check-coverage.py'), folder], capture_output=True, text=True)

    def test_complete_function_measurement_is_accepted(self):
        for optimize in (False, True):
            with self.subTest(optimize=optimize):
                self.assertEqual(self.invoke(optimize=optimize).returncode, 0)

    def test_partial_function_report_is_rejected(self):
        result = self.invoke(only_one=True)
        self.assertNotEqual(result.returncode, 0, result.stdout)

    def test_uncalled_function_is_rejected(self):
        result = self.invoke(zero_call=True)
        self.assertNotEqual(result.returncode, 0, result.stdout)

    def test_function_metadata_without_executable_lines_is_rejected(self):
        result = self.invoke(missing_line=True)
        self.assertNotEqual(result.returncode, 0, result.stdout)

    def test_empty_and_unrelated_reports_are_rejected(self):
        for text in ('', '1:9999:unrelated\nfunction other called 1\nbranch 0 taken 1\n'):
            with self.subTest(text=text):
                result = self.invoke(override=text)
                self.assertNotEqual(result.returncode, 0, result.stdout)

    def test_unexecuted_line_remains_rejected_under_optimized_python(self):
        result = self.invoke(optimize=True, missed=True)
        self.assertNotEqual(result.returncode, 0, result.stdout)

    def test_untaken_branch_remains_rejected_under_optimized_python(self):
        result = self.invoke(optimize=True, branch=True)
        self.assertNotEqual(result.returncode, 0, result.stdout)


if __name__ == '__main__':
    unittest.main()
