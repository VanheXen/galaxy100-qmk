// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Turbo latch: press TURBO (Fn+Right Alt) to arm; the next key pressed starts
// auto-firing on its own, no need to hold it. Press TURBO again to stop.

#include QMK_KEYBOARD_H
#include "turbo.h"

#ifndef TURBO_RATE_HZ
#    define TURBO_RATE_HZ 20 // presses per second
#endif

typedef enum { TURBO_OFF, TURBO_ARMED, TURBO_ON } turbo_state_t;

static turbo_state_t state = TURBO_OFF;
static uint16_t      turbo_kc;
static keypos_t      turbo_pos, arm_pos;
static bool          down;
static uint32_t      next_toggle;

void last_matrix_activity_trigger(void); // quantum/keyboard.c

static bool can_turbo(uint16_t keycode) {
    return IS_BASIC_KEYCODE(keycode) || IS_MOUSE_KEYCODE(keycode);
}

static void stop(void) {
    if (down) unregister_code16(turbo_kc);
    down  = false;
    state = TURBO_OFF;
}

bool turbo_process_record(uint16_t keycode, keyrecord_t *record) {
    if (keycode == TURBO) {
        if (record->event.pressed) {
            if (state == TURBO_OFF) {
                state   = TURBO_ARMED;
                arm_pos = record->event.key;
            } else {
                stop();
            }
        }
        return false;
    }

    if (state == TURBO_ARMED && record->event.pressed && can_turbo(keycode)) {
        state       = TURBO_ON;
        turbo_kc    = keycode;
        turbo_pos   = record->event.key;
        down        = false;
        next_toggle = timer_read32();
        return false; // turbo_task does the pressing from now on
    }

    // while firing, the turbo key itself is ignored (only TURBO stops it)
    if (state == TURBO_ON && keycode == turbo_kc) return false;
    return true;
}

void turbo_task(void) {
    if (state != TURBO_ON) return;
    if ((int32_t)(timer_read32() - next_toggle) < 0) return;
    next_toggle += 500 / TURBO_RATE_HZ; // half a period down, half up
    if (down)
        unregister_code16(turbo_kc);
    else
        register_code16(turbo_kc);
    down = !down;
    last_matrix_activity_trigger(); // counts as typing: no dimming or sleep while firing
}

static void blink_key(keypos_t pos, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t led = g_led_config.matrix_co[pos.row][pos.col];
    if (led == NO_LED) return;
    bool on = (timer_read32() / 300) % 2;
    rgb_matrix_set_color(led, on ? r : 0, on ? g : 0, on ? b : 0);
}

void turbo_indicators(void) {
    if (state == TURBO_ARMED) blink_key(arm_pos, 0xFF, 0xFF, 0xFF);
    if (state == TURBO_ON) blink_key(turbo_pos, 0xFF, 0x00, 0x00);
}
