/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <string.h>
#include "../src/input_processor_suppress.c"

static struct suppress_data data;
static const struct device dev = {.data = &data, .api = &suppress_api};

int main(void) {
    assert(suppress_init(&dev) == 0);
    assert(SOURCE_COUNT == (CONFIG_ZMK_INPUT_SUPPRESS_MAX_SOURCES > 0 ?
                           CONFIG_ZMK_INPUT_SUPPRESS_MAX_SOURCES :
                           MAX(TEST_LISTENER_COUNT, 1)));
    for (unsigned source = 0; source < 256; source++) {
        const struct suppress_data before = data;
        struct zmk_input_processor_state state = {.input_device_index = (uint8_t)source};
        struct input_event event = {
            .type = INPUT_EV_KEY, .code = INPUT_BTN_0, .value = 1, .sync = true};
        const int rc = suppress_event(&dev, &event, 0, 0, &state);
        if (source < SOURCE_COUNT) {
            assert(rc == ZMK_INPUT_PROC_CONTINUE && event.type == INPUT_EV_KEY && event.sync);
            assert(data.sources[source].forwarded == BIT(0));
            event.value = 0;
            assert(suppress_event(&dev, &event, SUPPRESS_ALL, 0, &state) == 0);
            assert(event.type == INPUT_EV_KEY && event.sync);
        } else {
            assert(rc == ZMK_INPUT_PROC_STOP && event.type == INPUT_EV_MSC && !event.sync);
        }
        assert(memcmp(&before, &data, sizeof(data)) == 0);
        event = (struct input_event){
            .type = INPUT_EV_KEY, .code = INPUT_BTN_TOUCH, .value = 1, .sync = true};
        assert(suppress_event(&dev, &event, UINT32_MAX, 0, &state) == ZMK_INPUT_PROC_STOP);
        assert(event.type == INPUT_EV_MSC && !event.sync);
        assert(memcmp(&before, &data, sizeof(data)) == 0);
    }
    struct input_event event = {
        .type = INPUT_EV_KEY, .code = INPUT_BTN_0, .value = 1, .sync = true};
    assert(suppress_event(&dev, &event, 0, 0, NULL) == 0);
    assert(data.sources[0].forwarded == BIT(0));
    event.value = -1;
    assert(suppress_event(&dev, &event, SUPPRESS_ALL, 0, NULL) == 0);
    assert(data.sources[0].forwarded == 0);
    printf("source allocation: listeners=%u limit=%u slots=%u history=%zu data=%zu; "
           "all256 IDs, invalid masks, balanced pairs, independent histories and NULL fallback PASS\n",
           TEST_LISTENER_COUNT, CONFIG_ZMK_INPUT_SUPPRESS_MAX_SOURCES, SOURCE_COUNT,
           sizeof(struct zmk_suppress_history), sizeof(data));
    return 0;
}
