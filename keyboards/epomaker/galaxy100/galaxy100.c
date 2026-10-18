// Copyright 2024 Epomaker (@Epomaker)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "galaxy100.h"
#include "rgb_record/rgb_record.h"
#include "party.h"
#ifdef WIRELESS_ENABLE
#    include "wireless.h"
#    include "control.h"
#    include "turbo.h"
#    include "socd.h"
#endif
#include "power_ui.h"
#include "usb_main.h"

typedef union {
    uint32_t raw;
    struct {
        uint8_t flag : 1;
        uint8_t devs : 3;
        uint8_t record_channel : 4;
        uint8_t record_last_mode;
        uint8_t last_btdevs : 3;
        uint8_t hue_preset; // P41: position of the RM_HUEU preset cycle
    };
} confinfo_t;
// stored as 4 bytes at offset 0 of the keyboard datablock:
// byte 0: flag(1) devs(3) record_channel(4) | byte 1: record_last_mode | byte 2: last_btdevs(3) | byte 3: hue_preset (was unused)
_Static_assert(sizeof(confinfo_t) == sizeof(uint32_t), "confinfo_t is stored as 4 bytes in the kb datablock");
confinfo_t confinfo;

// RM_HUEU cycles through these {hue, saturation} presets
static const uint8_t hue_presets[][2] = {{0, 255}, {85, 255}, {170, 255}, {43, 255}, {191, 255}, {128, 255}, {0, 0}};

typedef struct {
    bool active;
    uint32_t timer;
    uint32_t interval;
    uint32_t times;
    uint8_t index;
    RGB rgb;
    void (*blink_cb)(uint8_t);
} hs_rgb_indicator_t;

hs_rgb_indicator_t hs_rgb_indicators[HS_RGB_INDICATOR_COUNT];

void rgb_blink_dir(void);
static bool current_is_encoder; // set in process_record_kb, read by rgb_blink_dir
void rgb_matrix_hs_indicator(void);
void rgb_matrix_hs_indicator_set(uint8_t index, RGB rgb, uint32_t interval, uint8_t times);
void rgb_matrix_hs_set_remain_time(uint8_t index, uint8_t remain_time);

#define keymap_is_mac_system() ((get_highest_layer(default_layer_state) == _MBL) || (get_highest_layer(default_layer_state) == _MFL))

uint32_t        post_init_timer = 0x00;
bool            cable_present   = false; // P24: debounced USB cable pin (A7); true = cable plugged in
bool            bat_full_stable = false; // P25: debounced charge-complete pin (A15); true = charger reports full

#define PIN_DEBOUNCE_MS 50

// Accept a new pin level only after it has been steady for PIN_DEBOUNCE_MS.
// The first call adopts the raw level at once so boot does not start with a wrong state.
static void pin_debounce(bool raw, bool *stable, bool *last, uint32_t *since, bool *primed) {
    if (!*primed) {
        *primed = true;
        *stable = *last = raw;
        *since          = timer_read32();
        return;
    }
    if (raw != *last) {
        *last  = raw;
        *since = timer_read32();
    } else if (raw != *stable && timer_elapsed32(*since) >= PIN_DEBOUNCE_MS) {
        *stable = raw;
    }
}

static void power_pins_task(void) {
    static bool     cable_last, full_last, cable_primed, full_primed;
    static uint32_t cable_since, full_since;
#ifdef HS_BAT_CABLE_PIN
    pin_debounce(gpio_read_pin(HS_BAT_CABLE_PIN), &cable_present, &cable_last, &cable_since, &cable_primed);
#endif
#ifdef BAT_FULL_PIN
    pin_debounce(gpio_read_pin(BAT_FULL_PIN) == BAT_FULL_STATE, &bat_full_stable, &full_last, &full_since, &full_primed);
#endif
}
static uint32_t rec_time;
static uint32_t ee_clr_timer = 0; // Fn+Backspace held since
#define EE_CLR_HOLD_MS 10000
static bool     rec_filp;
bool            no_record_fg;

void eeconfig_confinfo_update(uint32_t raw) {

    eeconfig_update_kb_datablock(&raw, 0, sizeof(raw));
}

uint32_t eeconfig_confinfo_read(void) {

    uint32_t raw = 0;
    eeconfig_read_kb_datablock(&raw, 0, sizeof(raw));
    return raw;
}

void eeconfig_confinfo_default(void) {

    confinfo.flag             = true;
    confinfo.record_channel   = 0;
    confinfo.record_last_mode = 0xff;
    confinfo.last_btdevs      = 1;
    confinfo.hue_preset       = 0;

    eeconfig_init_user_datablock();
    eeconfig_confinfo_update(confinfo.raw);

}

void eeconfig_confinfo_init(void) {

    confinfo.raw = eeconfig_confinfo_read();
    if (!confinfo.raw) {
        eeconfig_confinfo_default();
        return;
    }

    // validate saved values (older layouts left these bytes unused or different)
    bool fixed = false;
    if (confinfo.last_btdevs < DEVS_BT1 || confinfo.last_btdevs > DEVS_BT3) {
        confinfo.last_btdevs = DEVS_BT1;
        fixed                = true;
    }
    if (!(confinfo.devs == DEVS_USB || (confinfo.devs >= DEVS_BT1 && confinfo.devs <= DEVS_BT3) || confinfo.devs == DEVS_2G4)) {
        confinfo.devs = DEVS_USB;
        fixed         = true;
    }
    if (confinfo.record_channel >= RGBREC_CHANNEL_NUM) {
        confinfo.record_channel = 0;
        fixed                   = true;
    }
    if (confinfo.hue_preset >= ARRAY_SIZE(hue_presets)) { // byte was unused before P41
        confinfo.hue_preset = 0;
        fixed               = true;
    }
    if (fixed) eeconfig_confinfo_update(confinfo.raw);
}

void keyboard_post_init_kb(void) {

#ifdef CONSOLE_ENABLE
    debug_enable = true;
#endif

    eeconfig_confinfo_init();

    // saved default layer must be one of the two base layers (core may not check)
    {
        uint8_t base = get_highest_layer(default_layer_state);
        if (base != _BL && base != _MBL) set_single_persistent_default_layer(_BL);
    }

#ifdef LED_POWER_EN_PIN
    gpio_set_pin_output(LED_POWER_EN_PIN);
    gpio_write_pin_high(LED_POWER_EN_PIN);
    gpio_set_pin_output(HS_LED_AUX_POWER_PIN);
    gpio_write_pin_high(HS_LED_AUX_POWER_PIN);

    gpio_set_pin_output(HS_LED_BOOSTING_PIN);
    gpio_write_pin_high(HS_LED_BOOSTING_PIN);
#endif

#ifdef HS_BT_DEF_PIN
    gpio_set_pin_input_low(HS_BT_DEF_PIN);
#endif

#ifdef HS_2G4_DEF_PIN
    gpio_set_pin_input_low(HS_2G4_DEF_PIN);
#endif

#ifdef USB_POWER_EN_PIN
    gpio_write_pin_low(USB_POWER_EN_PIN);
    gpio_set_pin_output(USB_POWER_EN_PIN);
#endif

#ifdef HS_BAT_CABLE_PIN
    gpio_set_pin_input(HS_BAT_CABLE_PIN);
#endif

#ifdef BAT_FULL_PIN
    gpio_set_pin_input_high(BAT_FULL_PIN);
#endif

#ifdef WIRELESS_ENABLE
    wireless_init();
    power_ui_init();
    post_init_timer = timer_read32();
#endif

    keyboard_post_init_user();

    rgbrec_init(confinfo.record_channel);

    party_boot();
}

#ifdef WIRELESS_ENABLE

void usb_power_connect(void) {

#    ifdef USB_POWER_EN_PIN
    gpio_write_pin_low(USB_POWER_EN_PIN);
#    endif
}

void usb_power_disconnect(void) {

#    ifdef USB_POWER_EN_PIN
    gpio_write_pin_high(USB_POWER_EN_PIN);
#    endif
}

void suspend_power_down_kb(void) {

#    ifdef LED_POWER_EN_PIN
    gpio_write_pin_low(LED_POWER_EN_PIN);
#    endif

    suspend_power_down_user();
}

void suspend_wakeup_init_kb(void) {

#    ifdef LED_POWER_EN_PIN
    gpio_write_pin_high(LED_POWER_EN_PIN);
#    endif

    wireless_devs_change(wireless_get_current_devs(), wireless_get_current_devs(), false);
    suspend_wakeup_init_user();
    hs_rgb_blink_set_timer(timer_read32());
}

// No idle sleep while a USB host powers us (it suspends us itself). On the USB
// position without a cable the keyboard runs on battery and must sleep too:
// the switch has no off position, so this is where it gets left.
bool lpwr_is_allow_timeout_hook(void) {
    return wireless_get_current_devs() != DEVS_USB || !cable_present;
}

void wireless_post_task(void) {

    // auto switching devs
    if (post_init_timer && timer_elapsed32(post_init_timer) >= 100) {

        md_send_devctrl(MD_SND_CMD_DEVCTRL_FW_VERSION);   // get the module fw version.
        md_send_devctrl(MD_SND_CMD_DEVCTRL_SLEEP_BT_EN);  // timeout 30min to sleep in bt mode, enable
        md_send_devctrl(MD_SND_CMD_DEVCTRL_SLEEP_2G4_EN); // timeout 30min to sleep in 2.4g mode, enable
        wireless_devs_change(!confinfo.devs, confinfo.devs, false);
        post_init_timer = 0x00;
    }

    hs_mode_scan(false, confinfo.devs, confinfo.last_btdevs);
}

uint32_t wls_process_long_press(uint32_t trigger_time, void *cb_arg) {
    uint16_t keycode = *((uint16_t *)cb_arg);

    switch (keycode) {
        case KC_BT1: {
            uint8_t mode = confinfo.devs;
            hs_modeio_detection(true, &mode, confinfo.last_btdevs);
            if ((mode == hs_bt) || (mode == hs_wireless) || (mode == hs_none)) {
                wireless_devs_change(wireless_get_current_devs(), DEVS_BT1, true);
            }

        } break;
        case KC_BT2: {
            uint8_t mode = confinfo.devs;
            hs_modeio_detection(true, &mode, confinfo.last_btdevs);
            if ((mode == hs_bt) || (mode == hs_wireless) || (mode == hs_none)) {
                wireless_devs_change(wireless_get_current_devs(), DEVS_BT2, true);
            }
        } break;
        case KC_BT3: {
            uint8_t mode = confinfo.devs;
            hs_modeio_detection(true, &mode, confinfo.last_btdevs);
            if ((mode == hs_bt) || (mode == hs_wireless) || (mode == hs_none)) {
                wireless_devs_change(wireless_get_current_devs(), DEVS_BT3, true);
            }
        } break;
        case KC_2G4: {
            uint8_t mode = confinfo.devs;
            hs_modeio_detection(true, &mode, confinfo.last_btdevs);
            if ((mode == hs_2g4) || (mode == hs_wireless) || (mode == hs_none)) {
                wireless_devs_change(wireless_get_current_devs(), DEVS_2G4, true);
            }
        } break;
        default:
            break;
    }

    return 0;
}

bool process_record_wls(uint16_t keycode, keyrecord_t *record) {
    static uint16_t keycode_shadow                     = 0x00;
    static deferred_token wls_process_long_press_token = INVALID_DEFERRED_TOKEN;

#    ifndef WLS_KEYCODE_PAIR_TIME
#        define WLS_KEYCODE_PAIR_TIME 3000
#    endif

#    define WLS_KEYCODE_EXEC(wls_dev)                                                                                          \
        do {                                                                                                                   \
            if (record->event.pressed) {                                                                                       \
                if (wireless_get_current_devs() != wls_dev)                                                                    \
                    wireless_devs_change(wireless_get_current_devs(), wls_dev, false);                                         \
                if (wls_held_mask != wls_key_bit) {                                                                            \
                    /* P15: a second mode key is down: a chord never starts pairing, drop the pending one */                   \
                    if (wls_process_long_press_token != INVALID_DEFERRED_TOKEN) {                                              \
                        cancel_deferred_exec(wls_process_long_press_token);                                                    \
                        wls_process_long_press_token = INVALID_DEFERRED_TOKEN;                                                 \
                    }                                                                                                          \
                } else if (wls_process_long_press_token == INVALID_DEFERRED_TOKEN) {                                           \
                    keycode_shadow               = keycode;                                                                    \
                    wls_process_long_press_token = defer_exec(WLS_KEYCODE_PAIR_TIME, wls_process_long_press, &keycode_shadow); \
                }                                                                                                              \
            } else if (wls_process_long_press_token != INVALID_DEFERRED_TOKEN && keycode_shadow == keycode) {                  \
                cancel_deferred_exec(wls_process_long_press_token);                                                            \
                wls_process_long_press_token = INVALID_DEFERRED_TOKEN;                                                         \
            }                                                                                                                  \
        } while (false)

    // P15: which mode keys are held, tracked on every event (also when the mode check below fails)
    static uint8_t wls_held_mask = 0;
    uint8_t        wls_key_bit   = (keycode == KC_BT1) ? 1 : (keycode == KC_BT2) ? 2 : (keycode == KC_BT3) ? 4 : (keycode == KC_2G4) ? 8 : 0;
    if (wls_key_bit) {
        if (record->event.pressed) {
            wls_held_mask |= wls_key_bit;
        } else {
            wls_held_mask &= ~wls_key_bit;
        }
    }

    switch (keycode) {
        case KC_BT1: {
            uint8_t mode = confinfo.devs;
            hs_modeio_detection(true, &mode, confinfo.last_btdevs);
            if ((mode == hs_bt) || (mode == hs_wireless) || (mode == hs_none)) {
                WLS_KEYCODE_EXEC(DEVS_BT1);
                hs_rgb_blink_set_timer(timer_read32());
            }

        } break;
        case KC_BT2: {
            uint8_t mode = confinfo.devs;
            hs_modeio_detection(true, &mode, confinfo.last_btdevs);
            if ((mode == hs_bt) || (mode == hs_wireless) || (mode == hs_none)) {
                WLS_KEYCODE_EXEC(DEVS_BT2);
                hs_rgb_blink_set_timer(timer_read32());
            }
        } break;
        case KC_BT3: {
            uint8_t mode = confinfo.devs;
            hs_modeio_detection(true, &mode, confinfo.last_btdevs);
            if ((mode == hs_bt) || (mode == hs_wireless) || (mode == hs_none)) {
                WLS_KEYCODE_EXEC(DEVS_BT3);
                hs_rgb_blink_set_timer(timer_read32());
            }
        } break;
        case KC_2G4: {
            uint8_t mode = confinfo.devs;
            hs_modeio_detection(true, &mode, confinfo.last_btdevs);
            if ((mode == hs_2g4) || (mode == hs_wireless) || (mode == hs_none)) {
                WLS_KEYCODE_EXEC(DEVS_2G4);
                hs_rgb_blink_set_timer(timer_read32());
            }
        } break;
        case KC_USB: {
            uint8_t mode = confinfo.devs;
            hs_modeio_detection(true, &mode, confinfo.last_btdevs);
            if (record->event.pressed && ((mode == hs_usb) || (mode == hs_none))) {
                wireless_devs_change(wireless_get_current_devs(), DEVS_USB, false);
                hs_rgb_blink_set_timer(timer_read32());
            }
        } break;
        default:
            return true;
    }

    return false;
}
#endif

// While Custom Paint recording is on, key presses paint keys instead of typing.
static bool rgbrec_process_record(uint16_t keycode, keyrecord_t *record) {
    if (!rgbrec_is_started()) return true;

    switch (keycode) {
        case RP_END:
        case RP_P0:
        case RP_P1:
        case RP_P2:
        case RM_NEXT:
            return true;
        case MO(_FL):
        case MO(_MFL): {
            if (!record->event.pressed) {
                // P39: the first Fn release only ends the Fn+End chord that started
                // recording and is skipped; every later Fn release paints.
                if (no_record_fg) {
                    rgbrec_register_record(keycode, record);
                } else {
                    no_record_fg = true;
                }
            }
            return true;
        }
    }
    // releases always go through: a key held when recording started must not
    // stick on the host, and hold timers (Fn+Backspace reset) must see it end
    if (!record->event.pressed) return true;
    if (!IS_QK_MOMENTARY(keycode)) rgbrec_register_record(keycode, record);
    return false;
}

void os_switch_to(bool to_mac) {
    if (to_mac == keymap_is_mac_system()) return;
    rgb_matrix_hs_set_remain_time(to_mac ? HS_RGB_BLINK_INDEX_WIN : HS_RGB_BLINK_INDEX_MAC, 0);
    rgb_matrix_hs_indicator_set(to_mac ? HS_RGB_BLINK_INDEX_MAC : HS_RGB_BLINK_INDEX_WIN, (RGB){RGB_WHITE}, 250, 3);
    set_single_persistent_default_layer(to_mac ? _MBL : _BL);
    layer_move(0);
}

// a lighting effect RM_NEXT can cycle to (not off, not Custom Paint)
static bool is_effect(uint8_t mode) {
    return mode != RGB_MATRIX_NONE && mode != RGB_MATRIX_CUSTOM_RGBR_PLAY && mode < RGB_MATRIX_EFFECT_MAX;
}

// show (and later record into) a Custom Paint slot
static void rgbrec_select(uint8_t channel) {
    if (rgbrec_is_started()) rgbrec_end(confinfo.record_channel); // keep what was painted
    rec_time     = 0;
    no_record_fg = false;
    if (is_effect(rgb_matrix_get_mode())) confinfo.record_last_mode = rgb_matrix_get_mode(); // RM_NEXT returns here
    confinfo.record_channel = channel;
    eeconfig_confinfo_update(confinfo.raw);
    rgbrec_show(channel);
}

static uint32_t os_switch_cb(uint32_t trigger_time, void *cb_arg) {
    os_switch_to(*(uint16_t *)cb_arg == TO(_MBL));
    return 0;
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {

    current_is_encoder = IS_ENCODEREVENT(record->event);

    if (!rgbrec_process_record(keycode, record)) {
        return false;
    }
    if (!process_record_user(keycode, record)) {
        return false;
    }
    if (!party_process_record(keycode, record)) {
        return false;
    }

#ifdef WIRELESS_ENABLE
    if (power_ui_process_record(keycode, record) != true) {
        return false;
    }
    if (!turbo_process_record(keycode, record)) {
        return false;
    }
    if (!socd_process_record(keycode, record)) {
        return false;
    }
    if (process_record_wls(keycode, record) != true) {
        return false;
    }
#endif

    switch (keycode) {
        case EE_CLR: {
            ee_clr_timer = record->event.pressed ? timer_read32() : 0;
            return false;
        } break;
        case NK_TOGG: {
            if (record->event.pressed) {
                rgb_matrix_hs_indicator_set(0xFF, (RGB){0x00, 0x6E, 0x00}, 250, 1);
            }
        } break;
        case RM_SPDU: {
            if (record->event.pressed) {
                if (rgb_matrix_get_speed() >= (RGB_MATRIX_SPD_STEP * 5)) {
                    rgb_blink_dir();
                }
            }
        } break;
        case RM_SPDD: {
            if (record->event.pressed) {
                if (rgb_matrix_get_speed() <= RGB_MATRIX_SPD_STEP) {
                    rgb_blink_dir();
                    rgb_matrix_set_speed(RGB_MATRIX_SPD_STEP);

                    return false;
                }
            }
        } break;
        case RM_VALU: {
            if (record->event.pressed) {
                if (rgb_matrix_get_val() >= (RGB_MATRIX_MAXIMUM_BRIGHTNESS - RGB_MATRIX_VAL_STEP)) {
                    rgb_blink_dir();
                }
            }
        } break;
        case RM_VALD: {
            if (record->event.pressed) {
                if (rgb_matrix_get_val() <= RGB_MATRIX_VAL_STEP) {
                    rgb_blink_dir();
                }
            }
        } break;
        case TO(_BL):
        case TO(_MBL): {
            // hold 3 s to switch Win/Mac, so a stray Fn+A / Fn+S can't flip it
            static deferred_token os_token = INVALID_DEFERRED_TOKEN;
            static uint16_t       os_target;
            if (record->event.pressed) {
                os_target = keycode;
                cancel_deferred_exec(os_token);
                os_token = defer_exec(3000, os_switch_cb, &os_target);
            } else {
                cancel_deferred_exec(os_token);
                os_token = INVALID_DEFERRED_TOKEN;
            }
            return false;
        } break;
        case RP_P0:
        case RP_P1:
        case RP_P2: {
            if (record->event.pressed) rgbrec_select(keycode - RP_P0);
            return false;
        } break;
        case RP_END: {
            if (record->event.pressed) {
                if (rgb_matrix_get_mode() != RGB_MATRIX_CUSTOM_RGBR_PLAY) {

                    return false;
                }
                if (!rgbrec_is_started()) {
                    if (rgbrec_start(confinfo.record_channel)) {
                        no_record_fg = false;
                        rec_time     = timer_read32();
                        rgbrec_set_close_all();
                    }
                } else {
                    rec_time = 0;
                    rgbrec_end(confinfo.record_channel);
                }
            }

            return false;
        } break;
        case RM_NEXT: {
            if (record->event.pressed) {
                if (rgb_matrix_get_mode() == RGB_MATRIX_CUSTOM_RGBR_PLAY) {
                    if (rgbrec_is_started()) {
                        rgbrec_read_current_channel(confinfo.record_channel);
                        rgbrec_end(confinfo.record_channel);
                        no_record_fg = false;
                    }
                    rgb_matrix_mode(is_effect(confinfo.record_last_mode) ? confinfo.record_last_mode : RGB_MATRIX_DEFAULT_MODE);
                    return false;
                }
                // effects only: Custom Paint has its own keys (RP_P0..2)
                do {
                    rgb_matrix_step();
                } while (!is_effect(rgb_matrix_get_mode()));
                confinfo.record_last_mode = rgb_matrix_get_mode();
                eeconfig_confinfo_update(confinfo.raw);
            }

            return false;
        } break;
        case RM_HUEU: {
            if (record->event.pressed) {
                confinfo.hue_preset = (confinfo.hue_preset + 1) % ARRAY_SIZE(hue_presets);
                eeconfig_confinfo_update(confinfo.raw);
                rgb_matrix_sethsv(hue_presets[confinfo.hue_preset][0], hue_presets[confinfo.hue_preset][1], rgb_matrix_get_val());
            }

            return false;
        } break;
        case KC_LCMD: {
            if (keymap_is_mac_system()) {
                if (keymap_config.no_gui && !rgbrec_is_started()) {
                    if (record->event.pressed) {
                        register_code16(KC_LCMD);
                    } else {
                        unregister_code16(KC_LCMD);
                    }
                }
            }

            return true;
        } break;
        case KC_RCMD: {
            if (keymap_is_mac_system()) {
                if (keymap_config.no_gui && !rgbrec_is_started()) {
                    if (record->event.pressed) {
                        register_code16(KC_RCMD);
                    } else {
                        unregister_code16(KC_RCMD);
                    }
                }
            }

            return true;
        } break;

        default:
            break;
    }

    return true;
}

void housekeeping_task_kb(void) {
#ifdef WIRELESS_ENABLE
    wireless_housekeeping_task();
#endif
    housekeeping_task_user();

    // Fn+Backspace held 10 s: same as VIA "Reset everything" (for when VIA is not reachable)
    if (ee_clr_timer && timer_elapsed32(ee_clr_timer) > EE_CLR_HOLD_MS) {
        eeconfig_disable();
        soft_reset_keyboard();
    }

    power_pins_task();

    // asleep: our sleep, or the PC suspended USB (lights off, see transport.c)
    bool awake = USB_DRIVER.state != USB_SUSPENDED;
#ifdef WIRELESS_ENABLE
    awake = awake && lpwr_get_state() == LPWR_NORMAL;
    if (awake) {
        power_ui_task();
        turbo_task(); // not while going to sleep: its key activity would cancel it
    }
#endif

    // QMK stops drawing indicators while the backlight is off (Fn+T / VIA "Off"),
    // so keep Caps/Num/Win lock, status light and battery visible on a black frame.
    // (not while asleep: sleep disables the matrix too, and the LEDs must stay dark)
    static uint32_t dark_frame = 0;
    if (awake && !rgb_matrix_is_enabled() && timer_elapsed32(dark_frame) >= 30) {
        dark_frame = timer_read32();
        rgb_matrix_set_color_all(0, 0, 0);
        rgb_matrix_indicators_advanced_kb(0, RGB_MATRIX_LED_COUNT);
        rgb_matrix_update_pwm_buffers();
    }
}

#ifdef WIRELESS_ENABLE
// remember the selected mode (and last Bluetooth host) across power cycles
void wireless_devs_change_kb(uint8_t old_devs, uint8_t new_devs, bool reset) {
    if (old_devs != new_devs) turbo_stop();
    if (confinfo.devs != new_devs) {
        confinfo.devs = new_devs;
        if (new_devs >= DEVS_BT1 && new_devs <= DEVS_BT3) confinfo.last_btdevs = new_devs;
        eeconfig_confinfo_update(confinfo.raw);
    }
}
#endif

void rgb_blink_dir(void) {
    // limit feedback on the arrows only for Fn+arrow presses, not for knob turns
    if (current_is_encoder) return;

    rgb_matrix_hs_indicator_set(HS_RGB_BLINK_INDEX_VAI, (RGB){0x20, 0x20, 0x20}, 250, 3);
    rgb_matrix_hs_indicator_set(HS_RGB_BLINK_INDEX_VAD, (RGB){0x20, 0x20, 0x20}, 250, 3);
    rgb_matrix_hs_indicator_set(HS_RGB_BLINK_INDEX_SPI, (RGB){0x20, 0x20, 0x20}, 250, 3);
    rgb_matrix_hs_indicator_set(HS_RGB_BLINK_INDEX_SPD, (RGB){0x20, 0x20, 0x20}, 250, 3);
}

void nkr_indicators_hook(uint8_t index) {

    if ((hs_rgb_indicators[index].rgb.r == 0x00) && (hs_rgb_indicators[index].rgb.g == 0x6E) && (hs_rgb_indicators[index].rgb.b == 0x00)) {

        rgb_matrix_hs_indicator_set(0xFF, (RGB){0x00, 0x00, 0x6F}, 250, 1);
    }
}

void rgb_matrix_hs_indicator_set(uint8_t index, RGB rgb, uint32_t interval, uint8_t times) {

    int slot = -1;
    for (int i = 0; i < HS_RGB_INDICATOR_COUNT; i++) {
        // reuse the live slot of the same key instead of filling the pool (not for the whole-board flash)
        if (index != 0xFF && hs_rgb_indicators[i].active && hs_rgb_indicators[i].index == index) {
            slot = i;
            break;
        }
    }
    for (int i = 0; slot < 0 && i < HS_RGB_INDICATOR_COUNT; i++) {
        if (!hs_rgb_indicators[i].active) slot = i;
    }
    if (slot >= 0) {
        int i = slot;
        {
            hs_rgb_indicators[i].active   = true;
            hs_rgb_indicators[i].timer    = timer_read32();
            hs_rgb_indicators[i].interval = interval;
            hs_rgb_indicators[i].times    = times * 2;
            hs_rgb_indicators[i].index    = index;
            hs_rgb_indicators[i].rgb      = rgb;
            if (index != 0xFF)
                hs_rgb_indicators[i].blink_cb = NULL;
            else {
                hs_rgb_indicators[i].blink_cb = nkr_indicators_hook;
            }
        }
    }
}

void rgb_matrix_hs_set_remain_time(uint8_t index, uint8_t remain_time) {

    for (int i = 0; i < HS_RGB_INDICATOR_COUNT; i++) {
        if (hs_rgb_indicators[i].active && hs_rgb_indicators[i].index == index) {
            hs_rgb_indicators[i].times = remain_time * 2;
            if (!remain_time) hs_rgb_indicators[i].active = false;
            break;
        }
    }
}

void rgb_matrix_hs_indicator(void) {

    for (int i = 0; i < HS_RGB_INDICATOR_COUNT; i++) {
        if (hs_rgb_indicators[i].active) {
            if (timer_elapsed32(hs_rgb_indicators[i].timer) >= hs_rgb_indicators[i].interval) {
                hs_rgb_indicators[i].timer = timer_read32();

                if (hs_rgb_indicators[i].times) {
                    hs_rgb_indicators[i].times--;
                }

                if (hs_rgb_indicators[i].times <= 0) {
                    hs_rgb_indicators[i].active = false;
                    hs_rgb_indicators[i].timer  = 0x00;
                    if (hs_rgb_indicators[i].blink_cb != NULL)
                        hs_rgb_indicators[i].blink_cb(i);
                    continue;
                }
            }

            if ((hs_rgb_indicators[i].times % 2)) {
                // P28: both paths go through the battery cap
                if (hs_rgb_indicators[i].index == 0xFF) {
                    for (uint8_t led = 0; led < RGB_MATRIX_LED_COUNT; led++) {
                        ui_set_color_capped(led, hs_rgb_indicators[i].rgb.r, hs_rgb_indicators[i].rgb.g, hs_rgb_indicators[i].rgb.b);
                    }
                } else {
                    ui_set_color_capped(hs_rgb_indicators[i].index, hs_rgb_indicators[i].rgb.r, hs_rgb_indicators[i].rgb.g, hs_rgb_indicators[i].rgb.b);
                }
            } else {
                if (hs_rgb_indicators[i].index == 0xFF) {
                    rgb_matrix_set_color_all(0x00, 0x00, 0x00);
                } else {
                    rgb_matrix_set_color(hs_rgb_indicators[i].index, 0x00, 0x00, 0x00);
                }
            }
        }
    }
}

void rgb_matrix_start_rec(void) {

    if (rgbrec_is_started()) {
        if (!rec_time || timer_elapsed32(rec_time) > 250) {
            rec_time = timer_read32();
            rec_filp = !rec_filp;
        }
        if (rec_filp) {
            rgb_matrix_set_color(0, 0x20, 0x20, 0x20);
            rgb_matrix_set_color(1, 0x20, 0x20, 0x20);
            rgb_matrix_set_color(2, 0x20, 0x20, 0x20);
        } else {
            rgb_matrix_set_color(0, 0x00, 0x00, 0x00);
            rgb_matrix_set_color(1, 0x00, 0x00, 0x00);
            rgb_matrix_set_color(2, 0x00, 0x00, 0x00);
        }
    } else {
        rec_time = 0;
        rec_filp = false;
    }
}

bool rgb_matrix_indicators_advanced_kb(uint8_t led_min, uint8_t led_max) {

    if (!rgb_matrix_indicators_advanced_user(led_min, led_max)) {
        return false;
    }
    // the overlays below cover all LEDs: draw them once, after the effect's last chunk
    if (led_max < RGB_MATRIX_LED_COUNT) return true;

#ifdef WIRELESS_ENABLE
    power_ui_layer_highlight();
#endif

    bool caps = host_keyboard_led_state().caps_lock;
#ifdef CAPS_WORD_ENABLE
    caps = caps || is_caps_word_on();
#endif
    if (caps)
        ui_set_color(HS_RGB_INDEX_CAPS, 0xFF, 0xFF, 0xFF);
    else
        rgb_matrix_set_color(HS_RGB_INDEX_CAPS, RGB_BLACK);
    if (host_keyboard_led_state().num_lock)
        ui_set_color(HS_RGB_INDEX_NUM_LOCK, 0xFF, 0xFF, 0xFF);
    else
        rgb_matrix_set_color(HS_RGB_INDEX_NUM_LOCK, RGB_BLACK);
    if (!keymap_is_mac_system() && keymap_config.no_gui)
        ui_set_color(HS_RGB_INDEX_WIN_LOCK, 0xFF, 0xFF, 0xFF);
    else if (!power_ui_layer_highlight_active()) // P29: on a Fn layer the highlight already colored this key
        rgb_matrix_set_color(HS_RGB_INDEX_WIN_LOCK, RGB_BLACK);

    rgb_matrix_hs_indicator();

    // Fn+Backspace held: Backspace blinks red, faster as the 10 s reset approaches
    if (ee_clr_timer) {
        uint32_t held = timer_elapsed32(ee_clr_timer);
        uint16_t period = held > EE_CLR_HOLD_MS * 3 / 4 ? 100 : 400;
        uint8_t  led    = g_led_config.matrix_co[1][13];
        if (led != NO_LED) ui_set_color(led, (timer_read32() / period) % 2 ? 0xFF : 0, 0, 0);
    }

#ifdef WIRELESS_ENABLE
    power_ui_indicators();
    turbo_indicators();
#endif
    party_indicators();

    rgb_matrix_start_rec();

    return true;
}

// Deep sleep: cut every LED supply line that lpwr_wakeup_hook turns back on
// (the LED enable alone left the boost converter running all night).
void lpwr_presleep_hook(void) {
    turbo_stop();
    gpio_write_pin_low(LED_POWER_EN_PIN);
    gpio_write_pin_low(HS_LED_AUX_POWER_PIN);
    gpio_write_pin_low(HS_LED_BOOSTING_PIN);
}

void lpwr_wakeup_hook(void) {

    gpio_write_pin_high(LED_POWER_EN_PIN);
    gpio_write_pin_high(HS_LED_AUX_POWER_PIN);
    gpio_write_pin_high(HS_LED_BOOSTING_PIN);
    party_boot();
}
