#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -euo pipefail
[ "$#" -eq 2 ] || { echo 'resolved workspace and exact archived baseline owner required' >&2; exit 2; }
workspace="$(cd "$1" && pwd)"
baseline="$(cd "$2" && pwd)"
root="$(cd "$(dirname "$0")/.." && pwd)"
out="${ZMK_TEST_OUTPUT:?fresh absolute evidence path}"
[[ "$out" = /* ]] && [ ! -e "$out" ] && [ ! -L "$out" ] || { echo 'fresh absolute output required' >&2; exit 2; }
cd "$workspace"
source "$root/tests/integration/workspace-env.sh"
zmk_test_select_workspace "$workspace"
python3 "$root/tests/measure-arm.py" --workspace "$workspace" --baseline "$baseline" --output "$out"
