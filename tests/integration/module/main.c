/* SPDX-License-Identifier: MIT */
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
static const struct device *a = DEVICE_DT_GET(DT_NODELABEL(source_a));
static const struct device *b = DEVICE_DT_GET(DT_NODELABEL(source_b));
static const struct device *gate = DEVICE_DT_GET(DT_NODELABEL(input_suppress));
static unsigned sends;
static int last_x;
int __real_zmk_endpoint_send_mouse_report(void);
int __wrap_zmk_endpoint_send_mouse_report(void) {
 sends++; last_x = zmk_hid_get_mouse_report()->body.d_x;
 printk("HID order=%u buttons=%u x=%d time=%lld\n", sends,
        zmk_hid_get_mouse_report()->body.buttons, last_x, k_uptime_get());
 return __real_zmk_endpoint_send_mouse_report();
}
static void report(const struct device *source,uint8_t type,uint16_t code,int32_t value,bool sync) {
 __ASSERT_NO_MSG(input_report(source,type,code,value,sync,K_FOREVER)==0); k_msleep(3);
}
static void button(const struct device *source,uint16_t code,int32_t value,bool sync) {
 report(source,INPUT_EV_KEY,code,value,sync);
}
static void held(bool expected) { __ASSERT_NO_MSG(!!(zmk_hid_get_mouse_report()->body.buttons&1U)==expected); }
static void route(bool enabled) {
 int rc=enabled?zmk_keymap_layer_activate(1,false):zmk_keymap_layer_deactivate(1,false);
 __ASSERT_NO_MSG(rc>=0); k_msleep(1);
}
static void run(void *x,void *y,void *z) {
 ARG_UNUSED(x); ARG_UNUSED(y); ARG_UNUSED(z);
 __ASSERT_NO_MSG(device_is_ready(gate)); held(false);
 unsigned before=sends;
 button(a,INPUT_BTN_0,1,false); held(true);
 button(a,INPUT_BTN_0,2,true); __ASSERT_NO_MSG(sends==before+1);
 route(true); button(a,INPUT_BTN_0,1,true); held(true);
 button(a,INPUT_BTN_0,0,false); held(false); __ASSERT_NO_MSG(sends==before+2);
 route(false); button(b,INPUT_BTN_0,1,true); held(true);
 route(true); button(a,INPUT_BTN_0,1,true); route(false);
 button(a,INPUT_BTN_0,0,true); held(true);
 button(b,INPUT_BTN_0,0,true); held(false);
 printk("PASS: repeat and cross-route independent source ownership\n");
 button(a,INPUT_BTN_TOUCH,1,false); button(a,INPUT_BTN_0,1,false); held(true);
 button(a,INPUT_BTN_TOUCH,0,false); held(true);
 button(a,INPUT_BTN_0,-1,false); held(false);
 button(a,INPUT_BTN_0,0,true); held(false);
 printk("PASS: touch and physical aliases have balanced core claims\n");
 route(true); before=sends;
 button(a,INPUT_BTN_TOUCH,1,false);
 report(a,INPUT_EV_ABS,INPUT_ABS_X,100,true);
 button(a,INPUT_BTN_TOUCH,0,true); __ASSERT_NO_MSG(sends==before);
 report(a,INPUT_EV_REL,INPUT_REL_X,7,false);
 report(a,INPUT_EV_ABS,INPUT_ABS_Y,100,true);
 __ASSERT_NO_MSG(sends==before+1 && last_x==7);
 report(a,INPUT_EV_REL,INPUT_REL_X,3,true);
 __ASSERT_NO_MSG(sends==before+2 && last_x==3);
 printk("PASS: silent filtered frames and exact pending-motion boundaries\n");
 route(false);
 /* Listener masks, no runtime registry or producer notification. */
 __ASSERT_NO_MSG(zmk_keymap_layer_activate(2,false)>=0);
 button(a,INPUT_BTN_TOUCH,1,false); button(b,INPUT_BTN_TOUCH,1,false);
 button(a,INPUT_BTN_0,1,true); held(true);
 route(true);
 report(a,INPUT_EV_REL,INPUT_REL_X,0,false);
 report(b,INPUT_EV_REL,INPUT_REL_X,0,false);
 button(a,INPUT_BTN_0,0,true); held(false);
 button(b,INPUT_BTN_0,1,true); held(false);
 route(false);
 button(a,INPUT_BTN_TOUCH,0,true); button(b,INPUT_BTN_TOUCH,0,true);
 button(a,INPUT_BTN_0,1,true); held(false); button(a,INPUT_BTN_0,0,true);
 button(b,INPUT_BTN_0,0,true); held(false);
 button(b,INPUT_BTN_0,1,true); held(false); button(b,INPUT_BTN_0,0,true);
 button(a,INPUT_BTN_TOUCH,1,false);
 button(a,INPUT_BTN_0,1,true); held(true); button(a,INPUT_BTN_0,0,true); held(false);
 button(a,INPUT_BTN_TOUCH,0,true);
 button(b,INPUT_BTN_0,1,true); held(false); button(b,INPUT_BTN_0,0,true);
 printk("PASS: traversed masks, delayed taps, independent contacts and owed release\n");
 /* A complete route activation without an event is deliberately unobserved. */
 button(a,INPUT_BTN_TOUCH,1,false);
 route(true); route(false);
 button(a,INPUT_BTN_TOUCH,0,true);
 button(a,INPUT_BTN_0,1,true); held(true); button(a,INPUT_BTN_0,0,true); held(false);
 /* A touch-up actually selected on the suppressing route contributes policy. */
 button(a,INPUT_BTN_TOUCH,1,false); route(true);
 button(a,INPUT_BTN_TOUCH,0,true); route(false);
 before=sends;
 button(a,INPUT_BTN_0,1,true); button(a,INPUT_BTN_0,0,true);
 __ASSERT_NO_MSG(sends==before); held(false);
 button(a,INPUT_BTN_TOUCH,1,false);
 button(a,INPUT_BTN_0,1,true); held(true); button(a,INPUT_BTN_0,0,true); held(false);
 button(a,INPUT_BTN_TOUCH,0,true);
 printk("PASS: untraversed route has no side effect; touch-up route latches; first click recovers\n");
 printk("final suppression native tests: PASS\n"); exit(0);
}
K_THREAD_DEFINE(suppress_native_tests,4096,run,NULL,NULL,NULL,K_LOWEST_APPLICATION_THREAD_PRIO,0,100);
