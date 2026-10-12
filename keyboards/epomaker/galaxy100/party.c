// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Party tricks, drawn on top of the lighting effect:
// - boot sweep: a rainbow wave reveals the keyboard at power-on and wake-up
// - Konami code (Up Up Down Down Left Right Left Right B A): 3 s rainbow rings.
//   The keys still reach the host, the code is only watched.
// - DICE / COIN keys: roll a die or flip a coin on the numpad 1-9, rest of the numpad dark

#include QMK_KEYBOARD_H
#include "party.h"
#include "power_ui.h"
#include "lib/lib8tion/lib8tion.h"

#define BOOT_MS 900
#define BOOT_TAIL 64 // rainbow band width, in LED x units (keyboard is 0..224)
#define KONAMI_MS 3000
#define ROLL_MS 600  // dice/coin animation
#define SHOW_MS 3000 // result shown after the animation
#define ROLL_STEP 80

static uint32_t boot_at = 0, konami_at = 0, roll_at = 0;
static bool     boot_on = false, konami_on = false, roll_on = false;
static bool     roll_is_coin;
static uint8_t  roll_result, roll_seed;

// numpad 7 8 9 / 4 5 6 / 1 2 3, matrix positions
static const uint8_t pad[9][2] = {{2, 15}, {2, 16}, {2, 17}, {3, 15}, {3, 16}, {3, 17}, {4, 15}, {4, 16}, {4, 17}};

// the rest of the numpad, dark while a result is shown: Num / * - + Enter 0 .
static const uint8_t pad_frame[][2] = {{1, 15}, {1, 16}, {1, 17}, {1, 18}, {2, 18}, {4, 18}, {5, 16}, {5, 17}};

// bit i = pad[i] lit
static const uint16_t dice_faces[6] = {
    0x010, // 5
    0x044, // 9 1
    0x054, // 9 5 1
    0x145, // 7 9 1 3
    0x155, // 7 9 5 1 3
    0x16D, // 7 4 1 9 6 3
};
#define COIN_HEADS 0x1EF // ring: all but the center
#define COIN_TAILS 0x0BA // cross: 8 4 5 6 2

void party_boot(void) {
    boot_at = timer_read32();
    boot_on = true;
}

static void konami_watch(uint16_t keycode) {
    static const uint16_t code[] = {KC_UP, KC_UP, KC_DOWN, KC_DOWN, KC_LEFT, KC_RGHT, KC_LEFT, KC_RGHT, KC_B, KC_A};
    static uint8_t        pos    = 0;

    if (keycode == code[pos]) {
        pos++;
    } else {
        pos = keycode == KC_UP ? (pos == 2 ? 2 : 1) : 0; // "Up Up Up" still counts as the start
    }
    if (pos == ARRAY_SIZE(code)) {
        pos       = 0;
        konami_at = timer_read32();
        konami_on = true;
    }
}

bool party_process_record(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return keycode != DICE && keycode != COIN;

    if (keycode == DICE || keycode == COIN) {
        random16_add_entropy(timer_read()); // human press timing
        roll_is_coin = keycode == COIN;
        uint8_t r;
        do r = random8(); while (r >= 252); // 252 = 6 * 42: no face more likely than another
        roll_result  = r % (roll_is_coin ? 2 : 6);
        roll_seed    = random8();
        roll_at      = timer_read32();
        roll_on      = true;
        return false;
    }
    konami_watch(keycode);
    return true;
}

static void draw_pad(uint16_t bits, uint8_t r, uint8_t g, uint8_t b) {
    for (uint8_t i = 0; i < 9; i++) {
        uint8_t led = g_led_config.matrix_co[pad[i][0]][pad[i][1]];
        if (led == NO_LED) continue;
        bool on = bits & (1 << i);
        rgb_matrix_set_color(led, on ? r : 0, on ? g : 0, on ? b : 0);
    }
}

static void roll_draw(void) {
    uint32_t t = timer_elapsed32(roll_at);
    if (t > ROLL_MS + SHOW_MS) {
        roll_on = false;
        return;
    }
    uint8_t face = roll_result;
    if (t < ROLL_MS) face = (uint8_t)(t / ROLL_STEP * 37 + roll_seed) % (roll_is_coin ? 2 : 6); // tumbling
    uint8_t v = ui_val();
    for (uint8_t i = 0; i < ARRAY_SIZE(pad_frame); i++) { // the Num Lock light right above distracts
        uint8_t led = g_led_config.matrix_co[pad_frame[i][0]][pad_frame[i][1]];
        if (led != NO_LED) rgb_matrix_set_color(led, 0, 0, 0);
    }
    if (roll_is_coin) {
        draw_pad(face ? COIN_TAILS : COIN_HEADS, v, v * 3 / 4, 0); // gold; testa = ring, croce = cross
    } else {
        draw_pad(dice_faces[face], v, v, v);
    }
}

static void boot_draw(void) {
    uint32_t t = timer_elapsed32(boot_at);
    if (t > BOOT_MS) {
        boot_on = false;
        return;
    }
    int16_t front = t * (224 + BOOT_TAIL) / BOOT_MS;
    uint8_t v     = ui_val();
    for (uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
        int16_t x = g_led_config.point[i].x;
        if (x > front) {
            rgb_matrix_set_color(i, 0, 0, 0); // not revealed yet
        } else if (front - x < BOOT_TAIL) {
            RGB c = hsv_to_rgb((HSV){x, 255, v});
            rgb_matrix_set_color(i, c.r, c.g, c.b);
        } // behind the band: the normal effect
    }
}

static void konami_draw(void) {
    uint32_t t = timer_elapsed32(konami_at);
    if (t > KONAMI_MS) {
        konami_on = false;
        return;
    }
    uint8_t v = ui_val();
    if (t > KONAMI_MS - 500) v = v * (KONAMI_MS - t) / 500; // fade out
    for (uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
        int16_t dx = g_led_config.point[i].x - 112, dy = g_led_config.point[i].y - 32;
        uint8_t d  = sqrt16(dx * dx + dy * dy);
        RGB     c  = hsv_to_rgb((HSV){d * 3 - t / 3, 255, v}); // rings moving outwards
        rgb_matrix_set_color(i, c.r, c.g, c.b);
    }
}

void party_indicators(void) {
    if (boot_on) boot_draw();
    if (konami_on) konami_draw();
    if (roll_on) roll_draw();
}
