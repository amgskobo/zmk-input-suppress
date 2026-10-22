# SPDX-License-Identifier: MIT
"""Gate complete production function measurements independently of fixture totals."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
SOURCES = {
    'input_processor_suppress.c.gcov': ROOT/'src/input_processor_suppress.c',
    'core.h.gcov': ROOT/'include/zmk-input-suppress/core.h',
}


def function_ranges(source):
    ranges = {}
    for match in re.finditer(r'^static[^;{}]+?\b(\w+)\([^;{}]*\)\s*\{', source, re.M):
        depth = 1
        end = match.end()
        while depth:
            if end == len(source):
                raise ValueError('unclosed production function')
            depth += (source[end] == '{') - (source[end] == '}')
            end += 1
        ranges[match[1]] = (source.count('\n', 0, match.start()) + 1,
                            source.count('\n', 0, end) + 1)
    return ranges


def check_report(text, source):
    ranges = function_ranges(source)
    calls = {name: int(count) for name, count in
             re.findall(r'^function (\w+) called (\d+)', text, re.M)}
    if not ranges or any(calls.get(name, 0) == 0 for name in ranges):
        raise ValueError('missing or uncalled production function')
    measured = [(count.strip(), int(line)) for count, line in
                re.findall(r'^\s*([^:\r\n]+):\s*([1-9]\d*):', text, re.M)
                if count.strip() != '-']
    for name, (first, last) in ranges.items():
        if not any(first <= line <= last for _, line in measured):
            raise ValueError('no executable lines for production function: ' + name)
    if any(count.startswith(('#####', '=====')) for count, _ in measured):
        raise ValueError('unexecuted production line')
    branches = re.findall(r'^branch.*', text, re.M)
    if not branches or any(re.search(r'taken 0(?: |$)|never executed', b) for b in branches):
        raise ValueError('unexecuted production branch')


def main():
    if len(sys.argv) != 2:
        raise SystemExit('usage: check-coverage.py COVERAGE_DIRECTORY')
    for name, source in SOURCES.items():
        check_report((Path(sys.argv[1])/name).read_text(), source.read_text())
        print(name + ': complete production functions, lines and branches PASS')


if __name__ == '__main__':
    main()
