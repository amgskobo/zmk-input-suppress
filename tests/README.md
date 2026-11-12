# Owner tests

Read AGENTS.md and the resolved supported-core/Zephyr instructions before tests.
Record selected paths/revisions, instructions read or absent, feature/backend and
raw output. Use a fresh output directory; retain first failures.

```sh
bash tests/run-docker.sh
ZMK_TEST_MODE=full bash tests/run-docker.sh
# In the pinned Linux image, using an already resolved west workspace:
ZMK_TEST_OUTPUT=/fresh/host bash tests/run.sh
ZMK_TEST_OUTPUT=/fresh/native bash tests/run-native.sh /resolved/workspace
# Separate strict upstream sanitizer boundary; both fixed profiles run three times:
ZMK_TEST_OUTPUT=/fresh/strict bash tests/run-native-sanitized.sh /resolved/workspace
# Real ARM object comparison against an exact archived baseline owner:
ZMK_TEST_OUTPUT=/fresh/arm bash tests/run-arm.sh /resolved/workspace /exact/baseline-owner
```

Quick executes every behavioral case under ASan/UBSan. Full adds optimized and
coverage variants and requires complete measured production driver/core lines
and branches. The whole unchanged driver is compiled by driver.c. Shared-history
and no-contact-latch controls must fail their named ownership/delayed-click
assertions. Existing outputs and invalid modes fail; no snapshot auto-accept.

| Fixture | Assertions |
| --- | --- |
| driver.c / repeats | 48 source/button/value/route cases, positive repeats and negative releases |
| driver.c / overlaps | 24 overlap/release-order/source cases; independent touch/button claims |
| driver.c / frames_and_lifetime | All17 button slots, suppressed pairs, unmatched/unknown edges, source-local pending/silent frames and residual ABS |
| driver.c / invalid | Invalid source/mask neutralization without aliasing, NULL-state fallback and cold initialization |
| driver.c / contact_policy | Traversed masks, delayed tap, repeated touch, lifting-route mask, source independence, pair obligations across new contacts and fresh-contact recovery |
| integration/module/main.c | Real input queue/listener/HID, direct-mask child/default routes, core reference counts, aliases, exact pending/silent frames, per-source delayed tap and untraversed-layer neutrality |

Run the native entry on both unchanged supported cores: stock
`5b51501fead672c41b5cfb396f3dafe0894bf4e9` and DYA
`e5c9b6915b56801193e359dd9bad4a167ce0d1b8`, shared Zephyr
`10ba6d0cb38bc3d258775d27982f707599320085`. Each binary runs three times.
The entry does not fetch dependencies. Set ZEPHYR_BASE/Zephyr_DIR and explicit
module discovery for symlinked workspaces as required by the selected manifest.
The canonical native product entry fixes all ten profiles: original automatic
indices3/4, automatic and limited2 alternate indices0/1 with five enabled
listeners, limit256, disabled-device/listener admission, and four named allocation
rejections (original high indices, high base, child and disabled-child routes).
Each of the six accepted binaries runs three times and must complete all five
original case families and exactly19 ordered HID reports. Rejection controls
require the named static assertion and are reported separately from product PASS.
The alternate overlay redeclares only the test mouse listeners in a later DTS
position; the original high-index topology remains required. Ambient profile,
limit and workspace-negative-control selectors are rejected before compilation.

Both portable modes also exercise all256 input IDs and invalid masks under
ASan/UBSan for automatic zero/two/five/256-listener and explicit1/2/256-slot
allocations. They check pair balance, unchanged neighboring histories, minimum-one
allocation and NULL-state source0 fallback. Existing48 repeats,24 overlaps,17
button slots and named history/contact controls remain unchanged. Full measures
the original complete driver/core coverage in addition to these boundary probes.

The separate strict native entry enables ASan and UBSan, pins ASAN_RECOVER=n,
checks real owner/upstream compiler instrumentation and nonrecovery flags, and
retains three runs of each exact original-auto/low-limit2 ELF. The documented
upstream `kscan_mock.c:103` signed left shift can block fixture completion on the
unchanged supported cores. Three matching location-specific failures are recorded
as BLOCKED_UPSTREAM_SIGNED_SHIFT with a nonzero final exit; they establish no
owner product PASS. Other failures and timeouts fail. Do not patch the core or
weaken sanitizer flags to clear this gate.

The ARM entry configures `xiao_ble//zmk` against the selected unchanged core and
its complete resolved Zephyr/platform graph. It builds the actual driver object
once, then reuses that exact compile command/config/DTS for archived-baseline
automatic and current-limit2 objects. A test-only forced header changes only the
allocation option after the generated config. It retains real objects, section
sizes, symbols, text hashes and compiler stack-usage files; requires80/80/32-byte
`data_0` and unchanged baseline/default automatic text; and does not claim physical
stack high-water or a complete ARM firmware build. Use fresh outputs on each core.
No runtime settings subsystem is supported by the minimal core; retired settings
and global-layer assertions are classified below; their targeted sources and
raw failures are externally archived.

Both host modes run test_coverage.py against the actual public validator. It
rejects empty/unrelated/partial reports, uncalled functions, a function's
metadata without measured executable lines, missed lines and untaken branches.
Validation also runs under optimized Python; no gate relies on removable Python
assert statements. These synthetic reports establish admission only; full host
still measures the complete compiled driver/core and retains all behavioral cases.

[maintenance.json](maintenance.json) and [contract.json](contract.json) map current
commands and limits. Physical USB/BLE/split, missing-source recovery, consumer
replacement, timing and power remain separate, unvalidated boundaries.

## Retained and retired contracts

| Prior assertion | Current disposition |
| --- | --- |
| 48 repeat and24 overlap cases, independent original codes/sources | Retained in common driver.c repeats/overlaps with direct route masks |
| All17 slots, suppressed pairs, owed releases, pending/silent frames | Retained in common host and actual input/listener/HID fixtures |
| Window/policy edits | Direct route-mask changes; retained pair obligations and contact masks across changes |
| Final layer/contact latch | Replaced with per-source masks from routes actually traversed; includes lifting event and delayed taps |
| Silent complete layer activation / snapshot race | RETIRED contract: no layer watcher; native explicitly verifies no inherited suppression without a routed event |
| Profile validation/CAS/settings namespace/serialized callback race | RETIRED interface: no profiles, mutable policy API, registry or legacy adapter; old values remain stored but unapplied |
| Matrix policy independence | Composition selects a dedicated listener route; Matrix setters cannot change its final mask |
| Edge delayed tap and first-contact recovery | Actual listener route selected by edge layer, direct final mask; producer remains transparent |

A contact that traversed a suppressing gesture/scroll route retains its button
mask through lift until a new distinct touch. Returning to a permissive route
alone does not allow that contact's delayed click. Already admitted pairs still
release exactly once. Consumers must supply complete observed contact/pair edges.
Source loss, consumer replacement and physical transport remain explicit gaps.


Converter reference/rounding, PadStick repeat/cancellation, Matrix gesture/KSCAN/
generated-key ownership and edge layer claims stay in their original owners.
Their original source KEY edges remain transparent; composition tests use an
explicit final companion. Stored old suppression values remain unapplied without
erasure. Duplicate historical migration prose was consolidated here; no current
behavioral assertion was removed. Unused log configuration and old layer/logging/
atomic host scaffolding were removed after checking actual include/symbol use.

## Selected workspace admission

The owner-local `integration/workspace-env.sh` derives `ZEPHYR_BASE`,
`Zephyr_DIR` and a nonempty `ZEPHYR_MODULES` from the selected workspace.
Discovery failure and empty module lists stop before compilation; ambient cache
modules cannot supply missing dependencies. The canonical host entry runs
`integration/test_workspace_env.py`, including failed-discovery/empty-list
regressions and a symlinked cache with paths containing spaces. The native entry
uses the same selection helper. This validates dependency selection, not hardware.
