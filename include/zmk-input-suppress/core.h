/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <dt-bindings/input/suppress.h>

#define ZMK_SUPPRESS_TOUCH_BIT 16U
struct zmk_suppress_history {
    uint32_t forwarded;
    uint32_t suppressed;
    uint32_t contact_mask;
    bool touching;
    bool frame_pending;
};

/* Only routes actually traversed during this contact contribute button policy.
 * Keep it after lift for delayed clicks. A distinct touch starts a new contact;
 * repeats and route changes never clear an owed press/release disposition. */
static inline uint32_t zmk_suppress_contact(struct zmk_suppress_history *history,
                                           bool touch, int32_t value, uint32_t mask) {
    if (touch && value > 0 && !history->touching) {
        history->contact_mask = 0;
        history->touching = true;
    }
    if (history->touching) {
        history->contact_mask |= mask & (SUPPRESS_BUTTONS | SUPPRESS_TOUCH);
    }
    if (touch && value <= 0) { history->touching = false; }
    return mask | history->contact_mask;
}

/* One original source/code owns at most one downstream claim. */
static inline bool zmk_suppress_button(struct zmk_suppress_history *history,
                                       unsigned slot, int32_t value, bool deny) {
    if (slot > ZMK_SUPPRESS_TOUCH_BIT) { return false; }
    const uint32_t bit = UINT32_C(1) << slot;
    if (value > 0) {
        if ((history->forwarded | history->suppressed) & bit) { return false; }
        if (deny) { history->suppressed |= bit; return false; }
        history->forwarded |= bit;
        return true;
    }
    const bool pass = (history->forwarded & bit) != 0U;
    history->forwarded &= ~bit;
    history->suppressed &= ~bit;
    return pass;
}
