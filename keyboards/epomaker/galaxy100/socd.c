// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later
//
// SOCD (simultaneous opposite cardinal directions) for opposite key pairs.
// When both keys of a pair are held:
//   last wins  - only the most recently pressed one counts
//   neutral    - neither counts
//   first wins - only the one pressed first counts
// Releasing a key brings the other one back if it is still held.

#include QMK_KEYBOARD_H
#include "socd.h"

uint8_t socd_mode  = SOCD_OFF;
uint8_t socd_pairs = SOCD_PAIRS_AD;

// pairs: A/D, W/S, Left/Right, Up/Down
static const uint16_t keys[8] = {KC_A, KC_D, KC_W, KC_S, KC_LEFT, KC_RGHT, KC_UP, KC_DOWN};
static bool           held[8], active[8];
static uint8_t        newest[4]; // per pair: index of the key pressed last

static bool pair_enabled(uint8_t pair) {
    switch (socd_pairs) {
        case SOCD_PAIRS_AD:      return pair == 0;
        case SOCD_PAIRS_WASD:    return pair <= 1;
        case SOCD_PAIRS_ARROWS:  return pair >= 2;
        case SOCD_PAIRS_ALL:     return true;
        default:                 return false;
    }
}

static void set_active(uint8_t k, bool on) {
    if (active[k] == on) return;
    active[k] = on;
    if (on)
        register_code(keys[k]);
    else
        unregister_code(keys[k]);
}

// turning SOCD off (VIA) while a key is active must not leave it held on the host
void socd_release_all(void) {
    for (uint8_t k = 0; k < 8; k++) {
        set_active(k, false);
        held[k] = false;
    }
}

bool socd_process_record(uint16_t keycode, keyrecord_t *record) {
    if (socd_mode == SOCD_OFF) return true;

    int8_t i = -1;
    for (uint8_t k = 0; k < 8; k++) {
        if (keys[k] == keycode && pair_enabled(k / 2)) i = k;
    }
    if (i < 0) return true;

    uint8_t p = i ^ 1, pair = i / 2;
    held[i]   = record->event.pressed;
    if (record->event.pressed) newest[pair] = i;

    bool want_i = held[i], want_p = held[p];
    if (held[i] && held[p]) {
        switch (socd_mode) {
            case SOCD_LAST_WINS:
                want_i = newest[pair] == i;
                want_p = !want_i;
                break;
            case SOCD_NEUTRAL:
                want_i = want_p = false;
                break;
            case SOCD_FIRST_WINS:
                want_i = newest[pair] != i;
                want_p = !want_i;
                break;
        }
    }
    // release before press so the host never sees both
    if (!want_i) set_active(i, false);
    if (!want_p) set_active(p, false);
    if (want_i) set_active(i, true);
    if (want_p) set_active(p, true);
    return false;
}
