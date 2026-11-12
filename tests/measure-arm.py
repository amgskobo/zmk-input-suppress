#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Compare real ARM driver objects using one selected generated build contract."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def run(command, cwd, log):
    start = time.monotonic_ns()
    with log.open('wb') as stream:
        result = subprocess.run(command, cwd=cwd, stdout=stream, stderr=subprocess.STDOUT)
    log.with_suffix(log.suffix + '.json').write_text(json.dumps({
        'command': command, 'cwd': str(cwd), 'exit': result.returncode,
        'start_monotonic_ns': start, 'end_monotonic_ns': time.monotonic_ns()}, indent=2) + '\n')
    if result.returncode:
        raise ValueError('command failed: ' + str(log))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--workspace', required=True, type=Path)
    parser.add_argument('--baseline', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    if not args.output.is_absolute() or args.output.exists() or args.output.is_symlink():
        parser.error('fresh absolute output required')
    args.output.parent.resolve(strict=True)
    args.output.mkdir()
    build = args.output / 'build'
    run(['west', 'build', '--cmake-only', '-p=always', '-s', str(args.workspace / 'zmk/app'),
         '-d', str(build), '-b', 'xiao_ble//zmk', '--',
         f'-DZMK_CONFIG={ROOT}/tests/arm/runtime', f'-DZMK_EXTRA_MODULES={ROOT}',
         f'-DEXTRA_DTC_OVERLAY_FILE={ROOT}/tests/integration/runtime/low-sources.overlay',
         '-DEXTRA_CFLAGS=-g -fstack-usage', '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'],
        args.workspace, args.output / 'configure.log')
    entries = [entry for entry in json.loads((build / 'compile_commands.json').read_text())
               if Path(entry['file']) == ROOT / 'src/input_processor_suppress.c']
    if len(entries) != 1:
        raise ValueError('exactly one actual ARM driver compilation required')
    entry = entries[0]
    command = shlex.split(entry['command'])
    target = command[command.index('-o') + 1]
    run(['ninja', '-j1', '-C', str(build), target], build, args.output / 'generate-and-compile.log')
    compiler = Path(command[0])
    prefix = str(compiler).removesuffix('gcc')
    results = []
    for name, owner, limit in (('baseline-auto', args.baseline, 0),
                               ('current-auto', ROOT, 0), ('current-limit2', ROOT, 2)):
        output = args.output / (name + '.o')
        if name == 'current-auto':
            output.write_bytes((build / target).read_bytes())
            stack = (build / target).with_suffix('.su')
            if stack.exists():
                (args.output / (name + '.su')).write_bytes(stack.read_bytes())
        else:
            selected = [part.replace(str(ROOT / 'include'), str(owner / 'include'))
                        .replace(str(ROOT / 'dts'), str(owner / 'dts')) for part in command]
            selected[selected.index('-o') + 1] = str(output)
            selected[selected.index(str(ROOT / 'src/input_processor_suppress.c'))] = str(
                owner / 'src/input_processor_suppress.c')
            selected.extend([f'-DTEST_SOURCE_LIMIT={limit}', '-include',
                             str(ROOT / 'tests/arm/source-limit.h')])
            run(selected, build, args.output / (name + '.compile.log'))
        run([prefix + 'size', '-A', str(output)], build, args.output / (name + '.size.txt'))
        run([prefix + 'nm', '-S', '--size-sort', str(output)], build,
            args.output / (name + '.symbols.txt'))
        symbols = (args.output / (name + '.symbols.txt')).read_text()
        data = re.findall(r'^\w+ ([0-9a-fA-F]+) [bB] data_0$', symbols, re.M)
        if len(data) != 1:
            raise ValueError('real data_0 symbol missing or duplicated')
        sections = re.findall(r'^(\.text\S*)\s+(\d+)\s',
                              (args.output / (name + '.size.txt')).read_text(), re.M)
        text = sum(int(size) for section, size in sections)
        text_data = bytearray()
        for section, size in sections:
            if not int(size):
                continue
            binary = args.output / (name + section + '.bin')
            run([prefix + 'objcopy', '-O', 'binary', '--only-section=' + section,
                 str(output), str(binary)], build,
                args.output / (name + section + '.objcopy.log'))
            contents = binary.read_bytes()
            if len(contents) != int(size):
                raise ValueError('text section size mismatch')
            text_data.extend(section.encode() + b'\0' + contents)
        (args.output / (name + '.text.bin')).write_bytes(text_data)
        results.append({'profile': name, 'data_0_bytes': int(data[0], 16), 'text_bytes': text,
                        'object_sha256': hashlib.sha256(output.read_bytes()).hexdigest(),
                        'text_sha256': hashlib.sha256((args.output / (name + '.text.bin')).read_bytes()).hexdigest(),
                        'stack_usage': (args.output / (name + '.su')).read_text()})
    if [item['data_0_bytes'] for item in results] != [80, 80, 32]:
        raise ValueError('expected five unchanged 16-byte histories, then two histories')
    if results[0]['text_sha256'] != results[1]['text_sha256']:
        raise ValueError('automatic production text differs from archived baseline')
    frames = []
    for result in results:
        frames.append({line.split('\t')[0].rsplit(':', 1)[1]: int(line.split('\t')[1])
                       for line in result['stack_usage'].splitlines()})
    if frames[0] != frames[1] or any(frames[2][name] > size for name, size in frames[0].items()):
        raise ValueError('automatic frames changed or limited frames increased')
    (args.output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    print('PASS: real ARM baseline/automatic/limit2 objects; histories80/80/32 bytes, '
          'automatic text unchanged; compiler stack frames retained')


if __name__ == '__main__':
    main()
