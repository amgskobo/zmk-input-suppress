#!/usr/bin/env bash
set -euo pipefail
workspace="$(cd "${1:?resolved west workspace}" && pwd)"
root="$(cd "$(dirname "$0")/.." && pwd)"
out="${ZMK_TEST_OUTPUT:?fresh absolute evidence path}"
[[ "$out" = /* ]] && [ ! -e "$out" ] && [ ! -L "$out" ] && [ ! -e "$out.build.log" ] && [ ! -L "$out.build.log" ] || { echo 'fresh absolute output required' >&2; exit 2; }
mkdir -p "$(dirname "$out")"
cd "$workspace"
source "$root/tests/integration/workspace-env.sh"
zmk_test_select_workspace "$workspace"
west zephyr-export
west build -p=always -o=-j2 -s "$workspace/zmk/app" -d "$out" -b native_sim//zmk_test_mock -- \
 -DZMK_CONFIG="$root/tests/integration/runtime" -DZMK_EXTRA_MODULES="$root;$root/tests/integration/module" >"$out.build.log" 2>&1
for repeat in 1 2 3; do
 timeout 30 "$out/zephyr/zmk.exe" >"$out.run-$repeat.log" 2>&1
 grep -Fq 'final suppression native tests: PASS' "$out.run-$repeat.log"
 cat "$out.run-$repeat.log"
done
