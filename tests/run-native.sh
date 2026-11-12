#!/usr/bin/env bash
set -euo pipefail
[ "$#" -eq 1 ] || { echo 'one resolved west workspace required' >&2; exit 2; }
workspace="$(cd "$1" && pwd)"
root="$(cd "$(dirname "$0")/.." && pwd)"
out="${ZMK_TEST_OUTPUT:?fresh absolute evidence path}"
for selector in ZMK_TEST_NATIVE_PROFILE ZMK_TEST_SOURCE_PROFILE ZMK_TEST_MAX_SOURCES ZMK_TEST_WORKSPACE_NEGATIVE_CONTROL; do
 [ -z "${!selector:-}" ] || { echo "canonical native matrix rejects ambient $selector" >&2; exit 2; }
done
[[ "$out" = /* ]] && [ ! -e "$out" ] && [ ! -L "$out" ] || { echo 'fresh absolute output required' >&2; exit 2; }
mkdir -p "$(dirname "$out")"
cd "$workspace"
source "$root/tests/integration/workspace-env.sh"
zmk_test_select_workspace "$workspace"
west zephyr-export
python3 "$root/tests/native-matrix.py" --workspace "$workspace" --output "$out" --kind product
