/* SPDX-License-Identifier: MIT */
#define DT_DRV_COMPAT zmk_input_processor_suppress
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/input/input.h>
#include <drivers/input_processor.h>
#include <zmk-input-suppress/core.h>

#define SOURCE_COUNT MAX(DT_NUM_INST_STATUS_OKAY(zmk_input_listener), 1)
struct suppress_data { struct zmk_suppress_history sources[SOURCE_COUNT]; };

static void neutral(struct input_event *event, bool sync) {
    event->type = INPUT_EV_MSC;
    event->code = INPUT_MSC_SCAN;
    event->value = 0;
    event->sync = sync;
}

static int suppress_event(const struct device *dev, struct input_event *event,
                          uint32_t mask, uint32_t unused,
                          struct zmk_input_processor_state *state) {
    ARG_UNUSED(unused);
    struct suppress_data *data = dev->data;
    const unsigned source = state != NULL ? state->input_device_index : 0U;
    if (source >= SOURCE_COUNT || (mask & ~SUPPRESS_ALL)) {
        neutral(event, false);
        return ZMK_INPUT_PROC_STOP;
    }
    struct zmk_suppress_history *history = &data->sources[source];
    mask = zmk_suppress_contact(history,
        event->type == INPUT_EV_KEY && event->code == INPUT_BTN_TOUCH, event->value, mask);
    bool pass = true;
    bool edge = false;
    if (event->type == INPUT_EV_KEY) {
        unsigned slot;
        if (event->code == INPUT_BTN_TOUCH) { slot = ZMK_SUPPRESS_TOUCH_BIT; }
        else if (event->code >= INPUT_BTN_0 && event->code <= INPUT_BTN_0 + 15) {
            slot = event->code - INPUT_BTN_0;
        } else { slot = ZMK_SUPPRESS_TOUCH_BIT + 1U; }
        pass = zmk_suppress_button(history, slot, event->value,
                                   slot <= ZMK_SUPPRESS_TOUCH_BIT && (mask & BIT(slot)) != 0U);
        edge = pass;
    } else if (event->type == INPUT_EV_ABS) {
        pass = (mask & SUPPRESS_ABS) == 0U;
    } else if (event->type == INPUT_EV_MSC && event->code == INPUT_MSC_SCAN) {
        pass = false;
    }
    if (!pass) {
        neutral(event, event->sync && history->frame_pending);
    } else if (edge) {
        /* Touch/physical aliases must each update the core reference count. */
        event->sync = true;
    }
    if (event->sync) { history->frame_pending = false; }
    else if (pass && event->type != INPUT_EV_MSC) { history->frame_pending = true; }
    return ZMK_INPUT_PROC_CONTINUE;
}

static int suppress_init(const struct device *dev) {
    struct suppress_data *data = dev->data;
    *data = (struct suppress_data){0};
    return 0;
}
static const struct zmk_input_processor_driver_api suppress_api = {.handle_event = suppress_event};
#define SUPPRESS_INST(n) \
    static struct suppress_data data_##n; \
    DEVICE_DT_INST_DEFINE(n, suppress_init, NULL, &data_##n, NULL, POST_KERNEL, \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &suppress_api);
DT_INST_FOREACH_STATUS_OKAY(SUPPRESS_INST)
