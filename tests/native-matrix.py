#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Fixed complete native matrices; profiles cannot be omitted through environment."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / 'tests/integration/runtime'
GUARD = 'static assertion failed: "Input Suppress listener index exceeds MAX_SOURCES"'
MARKERS = (
    'PASS: repeat and cross-route independent source ownership',
    'PASS: touch and physical aliases have balanced core claims',
    'PASS: silent filtered frames and exact pending-motion boundaries',
    'PASS: traversed masks, delayed taps, independent contacts and owed release',
    'PASS: untraversed route has no side effect; touch-up route latches; first click recovers',
    'final suppression native tests: PASS',
)


def product_profiles():
    return (
        ('original-high-auto', 0, False, (), False),
        ('low-auto', 0, True, (), False),
        ('low-limit2', 2, True, (), False),
        ('low-limit256', 256, True, (), False),
        ('disabled-device-limit2', 2, True, ('high-disabled-device.overlay',), False),
        ('disabled-listener-limit2', 2, True, ('high-disabled-listener.overlay',), False),
        ('reject-original-high-limit2', 2, False, (), True),
        ('reject-high-base-limit2', 2, True, ('high-base.overlay',), True),
        ('reject-high-child-limit2', 2, True, ('high-child.overlay',), True),
        ('reject-high-disabled-child-limit2', 2, True, ('high-disabled-child.overlay',), True),
    )


def execute(command, cwd, output, timeout=None, env=None):
    start = time.monotonic_ns()
    with output.open('wb') as log:
        try:
            result = subprocess.run(command, cwd=cwd, stdout=log, stderr=subprocess.STDOUT,
                                    timeout=timeout, env=env)
            rc = result.returncode
        except subprocess.TimeoutExpired:
            rc = 124
    record = {'command': command, 'cwd': str(cwd), 'exit': rc,
              'start_monotonic_ns': start, 'end_monotonic_ns': time.monotonic_ns()}
    output.with_suffix(output.suffix + '.json').write_text(json.dumps(record, indent=2) + '\n')
    return rc


def check_run(text, low, limit):
    if any(text.count(marker) != 1 for marker in MARKERS):
        raise ValueError('required nonempty native case completions missing or duplicated')
    expected = f'listener indices={"0/1" if low else "3/4"}'
    if expected not in text or f'allocation={limit}\n' not in text:
        raise ValueError('native listener numbering/allocation proof missing')
    reports = [tuple(map(int, row)) for row in
               re.findall(r'^HID order=(\d+) buttons=(\d+) x=(-?\d+) time=(\d+)$', text, re.M)]
    if len(reports) != 19 or [row[0] for row in reports] != list(range(1, 20)):
        raise ValueError('expected exactly 19 ordered HID reports')
    if any(a[3] > b[3] for a, b in zip(reports, reports[1:])):
        raise ValueError('HID report time regressed')
    return reports


def check_sanitizers(build):
    config = (build / 'zephyr/.config').read_text()
    if any(line not in config.splitlines() for line in
           ('CONFIG_ASAN=y', 'CONFIG_UBSAN=y', '# CONFIG_ASAN_RECOVER is not set')):
        raise ValueError('strict native sanitizer config is not active without recovery')
    entries = json.loads((build / 'compile_commands.json').read_text())
    for name in ('input_processor_suppress.c', 'kscan_mock.c'):
        selected = [entry for entry in entries if Path(entry['file']).name == name]
        if len(selected) != 1:
            raise ValueError('one sanitized owner/upstream compiler input required: ' + name)
        flags = shlex.split(selected[0]['command'])
        if ('-fsanitize=address,undefined' not in flags or
                '-fno-sanitize-recover=all' not in flags or
                any(flag.startswith('-fsanitize-recover=') for flag in flags)):
            raise ValueError('strict native compiler flags missing or recovery enabled: ' + name)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--workspace', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--kind', choices=('product', 'sanitizer'), required=True)
    args = parser.parse_args()
    if not args.output.is_absolute() or args.output.exists() or args.output.is_symlink():
        parser.error('fresh absolute output required')
    args.output.parent.resolve(strict=True)
    args.output.mkdir()
    profiles = product_profiles() if args.kind == 'product' else (
        ('strict-original-high-auto', 0, False, (), False),
        ('strict-low-limit2', 2, True, (), False),
    )
    results = []
    try:
        for name, limit, low, extra, reject in profiles:
            conf = args.output / (name + '.conf')
            conf.write_text(f'CONFIG_ZMK_INPUT_SUPPRESS_MAX_SOURCES={limit}\n'
                            f'CONFIG_INPUT_SUPPRESS_NATIVE_LOW_SOURCES={"y" if low else "n"}\n' +
                            ('CONFIG_ASAN=y\nCONFIG_UBSAN=y\nCONFIG_ASAN_RECOVER=n\n'
                             if args.kind == 'sanitizer' else ''))
            overlays = ([str(FIXTURE / 'low-sources.overlay')] if low else [])
            overlays.extend(str(FIXTURE / item) for item in extra)
            build = args.output / name
            command = ['west', 'build', '-p=always', '-o=-j1', '-s', str(args.workspace / 'zmk/app'),
                       '-d', str(build), '-b', 'native_sim//zmk_test_mock', '--',
                       f'-DZMK_CONFIG={FIXTURE}',
                       f'-DZMK_EXTRA_MODULES={ROOT};{ROOT}/tests/integration/module',
                       f'-DEXTRA_CONF_FILE={conf}', '-DEXTRA_DTC_OVERLAY_FILE=' + ';'.join(overlays),
                       '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON']
            if args.kind == 'sanitizer':
                command.append('-DEXTRA_CFLAGS=-fno-sanitize-recover=all -fno-omit-frame-pointer')
            build_log = args.output / (name + '.build.log')
            rc = execute(command, args.workspace, build_log)
            result = {'profile': name, 'build_exit': rc, 'limit': limit, 'low_indices': low}
            results.append(result)
            if reject:
                if rc == 0 or GUARD not in build_log.read_text():
                    raise ValueError(name + ': expected named allocation assertion did not reject')
                result['status'] = 'EXPECTED_REJECTION'
                print(name + ': EXPECTED_REJECTION (allocation assertion)', flush=True)
                continue
            if rc != 0:
                raise ValueError(name + ': native build failed')
            if args.kind == 'sanitizer':
                check_sanitizers(build)
            exe = build / 'zephyr/zmk.exe'
            result['elf_sha256'] = hashlib.sha256(exe.read_bytes()).hexdigest()
            result['runs'] = []
            for repeat in range(1, 4):
                log = args.output / f'{name}.run-{repeat}.log'
                environment = dict(os.environ)
                if args.kind == 'sanitizer':
                    environment.update(ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
                                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
                run_rc = execute([str(exe)], args.workspace, log, timeout=30, env=environment)
                text = log.read_text()
                run = {'repeat': repeat, 'exit': run_rc}
                result['runs'].append(run)
                if run_rc == 0:
                    run['hid_reports'] = check_run(text, low, limit)
                    run['case_completions'] = len(MARKERS) - 1
                    run['status'] = 'PASS'
                elif (args.kind == 'sanitizer' and run_rc != 124 and
                      re.search(r'kscan_mock\.c:103(?::\d+)?: runtime error: left shift', text)):
                    run['status'] = 'BLOCKED_UPSTREAM_SIGNED_SHIFT'
                else:
                    run['status'] = 'FAIL'
            statuses = {run['status'] for run in result['runs']}
            result['status'] = next(iter(statuses)) if len(statuses) == 1 else 'FAIL'
            print(name + ': ' + result['status'] + ' (same ELF, three repeats)', flush=True)
            if result['status'] == 'FAIL':
                raise ValueError(name + ': native run failed or repeats disagreed')
    except (ValueError, OSError) as error:
        print('FAIL: ' + str(error), flush=True)
        return_code = 1
    else:
        return_code = int(any(result['status'] != 'PASS' and
                              result['status'] != 'EXPECTED_REJECTION' for result in results))
        print(('BLOCKED: strict native upstream boundary' if return_code else
               f'PASS: complete {args.kind} native matrix'), flush=True)
    (args.output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    return return_code


if __name__ == '__main__':
    raise SystemExit(main())
