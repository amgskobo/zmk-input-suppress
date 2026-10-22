# zmk-input-suppress

A small final input processor for ZMK. Put the same instance at the end of
**every route** of each participating listener. Its one argument is a suppression
mask. The listener selects the route; this module retains source/button history.
There are no profile tables, layer subscriptions, producer links or settings APIs.
The module supports unchanged stock ZMK and DYA.

```dts
#include <zmk-input-suppress/input_processor_suppress.dtsi>
&trackpad_listener {
    input-processors = <&zip_absolute_to_relative>,
                       <&input_suppress (SUPPRESS_TOUCH | SUPPRESS_ABS)>;
    scroll {
        layers = <1>;
        input-processors = <&zip_absolute_to_relative>, <&scroll_mapper>,
                           <&input_suppress (SUPPRESS_TOUCH | SUPPRESS_BTN0 | SUPPRESS_ABS)>;
    };
    gestures {
        layers = <2>;
        input-processors = <&zip_matrix>, <&input_suppress SUPPRESS_ALL>;
    };
};
```

The include supplies the standard `input_suppress` node and mask definitions.
Converter/mapper/Matrix nodes belong to their respective modules. A listener
child replaces its parent chain, so the final processor must appear on each
route. Coordinate and gesture processors must preserve original button edges,
including touch, through their resets. Each original edge must reach the final
instance exactly once. Use one stable listener index per source.

| Symbol | Value / meaning |
| --- | --- |
| `SUPPRESS_BTN0` | `0x00001`: suppress new BTN_0 presses |
| `SUPPRESS_BUTTONS` | `0x0ffff`: suppress new BTN_0..15 presses |
| `SUPPRESS_TOUCH` | `0x10000`: suppress new BTN_TOUCH presses |
| `SUPPRESS_ABS` | `0x20000`: discard residual absolute events |
| `SUPPRESS_ALL` | `0x3ffff`: all of the above |

Individual buttons use their corresponding bits0..15. The mask is passed directly;
it is not a profile ID. There is no root policy block to duplicate route settings.

## Contact and button lifetime

Each input source has its own contact mask and first-press disposition. While
a raw BTN_TOUCH contact is open, button masks from **routes actually traversed**
are accumulated. Include the touch-up event's route before closing the contact.
The accumulated mask survives lift for delayed clicks until that source's next
distinct touch. Repeated positive touch values do not start a new contact.
Residual-ABS filtering uses the current route only; it is not accumulated.

A forwarded press owes exactly one release across all mask/route changes. A
suppressed pair stays suppressed until release, including when a new touch starts.
Positive values are presses; zero/negative values are releases. Repeats never
acquire a second HID claim; unmatched releases and unknown KEY codes are filtered.
Touch and physical BTN_0 remain independent even when the core maps both to HID
button0. A source without touch gets pair disposition without a contact latch.

Accepted button edges force a sync boundary. Filtered payload becomes neutral
MSC_SCAN, flushing preceding pending motion only when its frame ends. An entirely
filtered frame stays silent. Histories are fixed-size and owned by the input
thread. Invalid masks/sources are neutralized and stopped without aliasing another
source. NULL processor state retains the legacy source0 fallback. Malformed
invocations cannot repair an owed release; all routes must pass valid masks.

This module never queries layers or observes activations while a source is
silent. If another pad raises and lowers a layer without an event from this
source traversing its route, this source inherits no suppression. Put the same
final instance on both sides' scroll routes to apply a shared layer when their
events actually use it. The layer-raising event can still use its previously
selected route; edge-touch drops the transition coordinate, and the next routed
event, including touch-up, supplies the scroll mask. Complete source contact
boundaries are required; this filter does not reconstruct lost events.

## Set operation and migration

Use this independent module with pure abs2rel/scroll, PadStick, Matrix and
edge-touch modules. They do not reference this filter, publish windows or alter
its state. They own conversion, motion scheduling, gestures and layer claims.
Matrix enable changes gestures only. The application owns route ordering and
masks. The independent module is published at
https://github.com/amgskobo/zmk-input-suppress. Select a reviewed commit of this
module alongside the matching producer versions, or select its local checkout
explicitly as a Zephyr module. A minimal west project entry is:

```yaml
- name: zmk-input-suppress
  url: https://github.com/amgskobo/zmk-input-suppress
  revision: main # Pin the reviewed full commit for reproducible builds.
```

The earlier unpublished profile/layer design is superseded: remove profile
children, `suppress-layers`, `layer-mask`, `latch-until-next-touch` and profile/window
API calls. Replace each former profile argument with the actual mask. Remove
`CONFIG_ZMK_INPUT_SUPPRESS_CUSTOM_SETTINGS` and the former converter option
`CONFIG_ZMK_INPUT_ABS2REL_CUSTOM_SETTINGS`. The minimal core has no runtime
suppression registry or legacy settings adapter. Saved `amgskobo__a2r`/`amgskobo__sup`
suppression values are left in storage but are **not loaded or applied**. DTS masks
now determine policy; old Studio suppression controls are unavailable. No storage
erase is necessary.

See [the owner test guide and retirement map](tests/README.md).
Inertia's scheduled outputs and virtual KSCAN/keymap outputs have their own
lifetimes and may bypass this source filter. Source loss/epoch reuse and consumer
replacement cleanup are not implemented. Host/native/compile results do not
establish physical USB/BLE/split, timing or power acceptance.
