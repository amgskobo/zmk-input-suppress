#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -euo pipefail
root="$(cd -- "$(dirname -- "$0")/.." && pwd)"
mode="${ZMK_TEST_MODE:-quick}"
case "$mode" in quick|full) ;; *) echo 'invalid test mode' >&2; exit 2;; esac
python3 -m unittest discover -s "$root/tests" -p 'test_*.py' -v
python3 -m unittest discover -s "$root/tests/integration" -p 'test_*.py' -v
out="${ZMK_TEST_OUTPUT:-$(mktemp -u /tmp/input-suppress-tests.XXXXXX)}"
mkdir -- "$out"
printf '%s\n' "$mode" >"$out/mode.txt"
cc --version >"$out/compiler.txt"
variants=(sanitized)
[ "$mode" = quick ] || variants+=(optimized coverage)
for variant in "${variants[@]}"; do
  flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  [ "$variant" != optimized ] || flags=(-O2)
  [ "$variant" != coverage ] || flags=(-O0 --coverage)
  cc -std=c11 -Wall -Wextra -Werror "${flags[@]}" -I"$root/tests/support" -I"$root/include" -I"$root/dts" \
    "$root/tests/driver.c" -o "$out/$variant"
  ASAN_OPTIONS=detect_leaks=0 "$out/$variant" | tee "$out/$variant.log"
done
cc -std=c11 -Wall -Wextra -Werror -O1 -DTEST_SHARED_HISTORY \
  -I"$root/tests/support" -I"$root/include" -I"$root/dts" "$root/tests/driver.c" -o "$out/shared-history"
set +e
"$out/shared-history" >"$out/shared-history.log" 2>&1
control=$?
set -e
if [ "$control" -eq 0 ] || ! grep -q 'independent button ownership' "$out/shared-history.log"; then
  echo 'negative control did not fail ownership assertion' >&2; exit 1
fi
cc -std=c11 -Wall -Wextra -Werror -O1 -DTEST_NO_CONTACT_LATCH \
  -I"$root/tests/support" -I"$root/include" -I"$root/dts" "$root/tests/driver.c" -o "$out/no-contact-latch"
set +e
"$out/no-contact-latch" >"$out/no-contact-latch.log" 2>&1
control=$?
set -e
if [ "$control" -eq 0 ] || ! grep -q 'delayed click remains suppressed after route exit' "$out/no-contact-latch.log"; then
  echo 'negative control did not fail delayed-click assertion' >&2; exit 1
fi
if [ "$mode" = full ]; then
  (cd "$out" && gcov -b -c coverage-driver.gcno >coverage.log)
  python3 "$root/tests/check-coverage.py" "$out"
fi
printf 'PASS: %s\n' "$out"
