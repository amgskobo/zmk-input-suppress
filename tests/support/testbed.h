/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#define BIT(n) (UINT32_C(1) << (n))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define ARG_UNUSED(x) ((void)(x))
#define DT_NUM_INST_STATUS_OKAY(c) 2
#define DT_INST_FOREACH_STATUS_OKAY(fn)
#define ZMK_INPUT_PROC_CONTINUE 0
#define ZMK_INPUT_PROC_STOP 1
#define INPUT_EV_KEY 1
#define INPUT_EV_REL 2
#define INPUT_EV_ABS 3
#define INPUT_EV_MSC 4
#define INPUT_BTN_0 256
#define INPUT_BTN_TOUCH 330
#define INPUT_REL_X 0
#define INPUT_REL_Y 1
#define INPUT_MSC_SCAN 4
struct device { void *data; const void *config; const void *api; const char *name; };
struct input_event { const struct device *dev; uint8_t type; uint16_t code; int32_t value; bool sync; };
struct zmk_input_processor_state { uint8_t input_device_index; };
struct zmk_input_processor_driver_api {
    int (*handle_event)(const struct device *, struct input_event *, uint32_t, uint32_t,
                        struct zmk_input_processor_state *);
};
