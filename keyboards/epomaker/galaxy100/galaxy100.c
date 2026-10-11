// Copyright 2024 Epomaker (@Epomaker)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "rgb_record/rgb_record.h"
#ifdef WIRELESS_ENABLE
#    include "wireless.h"
#    include "control.h"
#    include "power_ui.h"
#    include "turbo.h"
#endif

typedef union {
    uint32_t raw;
    struct {
        uint8_t flag : 1;
        uint8_t devs : 3;
        uint8_t record_channel : 4;
        uint8_t record_last_mode;
        uint8_t last_btdevs : 3;
    };
} confinfo_t;
confinfo_t confinfo;

typedef struct {
    bool active;
    uint32_t timer;
    uint32_t interval;
    uint32_t times;
    uint8_t index;
    RGB rgb;
    void (*blink_cb)(uint8_t);
} hs_rgb_indicator_t;

enum layers {
    _BL = 0,
    _FL,
    _MBL,
    _MFL,
    _FBL,
};

hs_rgb_indicator_t hs_rgb_indicators[HS_RGB_INDICATOR_COUNT];

void rgb_blink_dir(void);
static keyrecord_t *current_record; // set in process_record_kb, read by rgb_blink_dir
void hs_reset_settings(void);
void rgb_matrix_hs_indicator(void);
void rgb_matrix_hs_indicator_set(uint8_t index, RGB rgb, uint32_t interval, uint8_t times);
void rgb_matrix_hs_set_remain_time(uint8_t index, uint8_t remain_time);

#define keymap_is_mac_system() ((get_highest_layer(default_layer_state) == _MBL) || (get_highest_layer(default_layer_state) == _MFL))

uint32_t        post_init_timer = 0x00;
bool            charging_state  = false;
static uint32_t ee_clr_timer    = 0;
static uint32_t rec_time;
static bool     rec_filp;
HSV             start_hsv;
bool            no_record_fg;
bool            lower_sleep = false; // used by control.c

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

    eeconfig_init_user_datablock();
    eeconfig_confinfo_update(confinfo.raw);

}

void eeconfig_confinfo_init(void) {

    confinfo.raw = eeconfig_confinfo_read();
    if (!confinfo.raw) {
        eeconfig_confinfo_default();
    }
}

void keyboard_post_init_kb(void) {

#ifdef CONSOLE_ENABLE
    debug_enable = true;
#endif

    eeconfig_confinfo_init();

#ifdef LED_POWER_EN_PIN
    gpio_set_pin_output(LED_POWER_EN_PIN);
    gpio_write_pin_high(LED_POWER_EN_PIN);
    gpio_set_pin_output(A9);
    gpio_write_pin_high(A9);

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

    start_hsv = rgb_matrix_get_hsv();
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

bool lpwr_is_allow_timeout_hook(void) {

    if (wireless_get_current_devs() == DEVS_USB) {
        return false;
    }

    return true;
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

    keycode_shadow = keycode;

#    ifndef WLS_KEYCODE_PAIR_TIME
#        define WLS_KEYCODE_PAIR_TIME 3000
#    endif

#    define WLS_KEYCODE_EXEC(wls_dev)                                                                                          \
        do {                                                                                                                   \
            if (record->event.pressed) {                                                                                       \
                if (wireless_get_current_devs() != wls_dev)                                                                    \
                    wireless_devs_change(wireless_get_current_devs(), wls_dev, false);                                         \
                if (wls_process_long_press_token == INVALID_DEFERRED_TOKEN) {                                                  \
                    wls_process_long_press_token = defer_exec(WLS_KEYCODE_PAIR_TIME, wls_process_long_press, &keycode_shadow); \
                }                                                                                                              \
            } else {                                                                                                           \
                cancel_deferred_exec(wls_process_long_press_token);                                                            \
                wls_process_long_press_token = INVALID_DEFERRED_TOKEN;                                                         \
            }                                                                                                                  \
        } while (false)

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
            if (record->event.pressed) {
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
    switch (keycode) {
        case MO(_FL):
        case MO(_MFL): {
            if (!record->event.pressed && rgbrec_is_started()) {
                if (no_record_fg == true) {
                    no_record_fg = false;
                    rgbrec_register_record(keycode, record);
                }
                no_record_fg = true;
            }
            break;
        }
        case RP_END:
        case RP_P0:
        case RP_P1:
        case RP_P2:
        case RM_NEXT:
            break;
        default: {
            if (rgbrec_is_started()) {
                if (!IS_QK_MOMENTARY(keycode) && record->event.pressed) {
                    rgbrec_register_record(keycode, record);

                    return false;
                }
            }
        } break;
    }

    if (rgbrec_is_started() && (!(keycode == RP_P0 || keycode == RP_P1 || keycode == RP_P2 || keycode == RP_END || keycode == RM_NEXT || keycode == MO(_FL) || keycode == MO(_MFL)))) {

        return false;
    }

    return true;
}

void os_switch_to(bool to_mac) {
    if (to_mac == keymap_is_mac_system()) return;
    rgb_matrix_hs_set_remain_time(to_mac ? HS_RGB_BLINK_INDEX_WIN : HS_RGB_BLINK_INDEX_MAC, 0);
    rgb_matrix_hs_indicator_set(to_mac ? HS_RGB_BLINK_INDEX_MAC : HS_RGB_BLINK_INDEX_WIN, (RGB){RGB_WHITE}, 250, 3);
    set_single_persistent_default_layer(to_mac ? _MBL : _BL);
    layer_move(0);
}

static uint32_t os_switch_cb(uint32_t trigger_time, void *cb_arg) {
    os_switch_to(*(uint16_t *)cb_arg == TO(_MBL));
    return 0;
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {

    current_record = record;

    if (!rgbrec_process_record(keycode, record)) {
        return false;
    }
    if (!process_record_user(keycode, record)) {
        return false;
    }

#ifdef WIRELESS_ENABLE
    if (power_ui_process_record(keycode, record) != true) {
        return false;
    }
    if (!turbo_process_record(keycode, record)) {
        return false;
    }
    if (process_record_wls(keycode, record) != true) {
        return false;
    }
#endif

    switch (keycode) {
        case QK_BOOT: {
            if (record->event.pressed) {
                bootloader_jump(); // keep keymap and settings (Esc-hold at plug-in still resets)
            }
        } break;

        case NK_TOGG: {
            if (rgbrec_is_started()) {

                return false;
            }
            if (record->event.pressed) {
                rgb_matrix_hs_indicator_set(0xFF, (RGB){0x00, 0x6E, 0x00}, 250, 1);
            }
        } break;
        case EE_CLR: {
            if (record->event.pressed) {
                ee_clr_timer = timer_read32();
            } else {
                ee_clr_timer = 0;
            }

            return false;
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
                    start_hsv.v = RGB_MATRIX_MAXIMUM_BRIGHTNESS;
                } else {
                    start_hsv.v = rgb_matrix_get_val() + RGB_MATRIX_VAL_STEP;
                }
            }
        } break;
        case RM_VALD: {
            if (record->event.pressed) {
                if (rgb_matrix_get_val() <= RGB_MATRIX_VAL_STEP) {
                    rgb_blink_dir();

                    rgb_matrix_set_color_all(0, 0, 0);
                    start_hsv.v = 0;
                } else {
                    start_hsv.v = rgb_matrix_get_val() - RGB_MATRIX_VAL_STEP;
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
        case RP_P0: {
            if (record->event.pressed) {
                confinfo.record_channel = 0;
                rgbrec_read_current_channel(confinfo.record_channel);
                rgbrec_end(confinfo.record_channel);
                eeconfig_confinfo_update(confinfo.raw);
                rgbrec_show(confinfo.record_channel);
            }

            return false;
        } break;
        case RP_P1: {
            if (record->event.pressed) {
                confinfo.record_channel = 1;
                rgbrec_read_current_channel(confinfo.record_channel);
                rgbrec_end(confinfo.record_channel);
                eeconfig_confinfo_update(confinfo.raw);
                rgbrec_show(confinfo.record_channel);
            }

            return false;
        } break;
        case RP_P2: {
            if (record->event.pressed) {
                confinfo.record_channel = 2;
                rgbrec_read_current_channel(confinfo.record_channel);
                rgbrec_end(confinfo.record_channel);
                eeconfig_confinfo_update(confinfo.raw);
                rgbrec_show(confinfo.record_channel);
            }

            return false;
        } break;
        case RP_END: {
            if (record->event.pressed) {
                if (rgb_matrix_get_mode() != RGB_MATRIX_CUSTOM_RGBR_PLAY) {

                    return false;
                }
                if (!rgbrec_is_started()) {
                    rgbrec_start(confinfo.record_channel);
                    no_record_fg = false;
                    rec_time     = timer_read32();
                    rgbrec_set_close_all(HSV_BLACK);
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
                    if (confinfo.record_last_mode != 0xFF)
                        rgb_matrix_mode(confinfo.record_last_mode);
                    else
                        rgb_matrix_mode(RGB_MATRIX_DEFAULT_MODE);
                    eeconfig_confinfo_update(confinfo.raw);
                    start_hsv = rgb_matrix_get_hsv();
                    return false;
                }
                rgb_matrix_step();
                if (rgb_matrix_get_mode() == RGB_MATRIX_NONE) rgb_matrix_step(); // skip "off"
                confinfo.record_last_mode = rgb_matrix_get_mode();
                eeconfig_confinfo_update(confinfo.raw);
                start_hsv = rgb_matrix_get_hsv();
            }

            return false;
        } break;
        case RM_HUEU: {
            if (record->event.pressed) {
                static const uint8_t presets[][2] = {{0, 255}, {85, 255}, {170, 255}, {43, 255}, {191, 255}, {128, 255}, {0, 0}};
                static uint8_t       preset       = 0;
                preset                            = (preset + 1) % ARRAY_SIZE(presets);
                rgb_matrix_sethsv(presets[preset][0], presets[preset][1], rgb_matrix_get_val());
                start_hsv = rgb_matrix_get_hsv();
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

void housekeeping_task_user(void) {

    charging_state = gpio_read_pin(HS_BAT_CABLE_PIN);
#ifdef WIRELESS_ENABLE
    power_ui_task();
    turbo_task();
#endif

    // QMK stops drawing indicators while the backlight is off (Fn+T / VIA "Off"),
    // so keep Caps/Num/Win lock, status light and battery visible on a black frame.
    static uint32_t dark_frame = 0;
    if (!rgb_matrix_is_enabled() && timer_elapsed32(dark_frame) >= 30) {
        dark_frame = timer_read32();
        rgb_matrix_set_color_all(0, 0, 0);
        rgb_matrix_indicators_advanced_kb(0, RGB_MATRIX_LED_COUNT);
        rgb_matrix_update_pwm_buffers();
    }
}

#ifdef WIRELESS_ENABLE
// remember the selected mode (and last Bluetooth host) across power cycles
void wireless_devs_change_kb(uint8_t old_devs, uint8_t new_devs, bool reset) {
    if (confinfo.devs != new_devs) {
        confinfo.devs = new_devs;
        if (new_devs >= DEVS_BT1 && new_devs <= DEVS_BT3) confinfo.last_btdevs = new_devs;
        eeconfig_confinfo_update(confinfo.raw);
    }
}
#endif

void rgb_blink_dir(void) {
    // limit feedback on the arrows only for Fn+arrow presses, not for knob turns
    if (current_record && IS_ENCODEREVENT(current_record->event)) return;

    rgb_matrix_hs_indicator_set(HS_RGB_BLINK_INDEX_VAI, (RGB){0x20, 0x20, 0x20}, 250, 3);
    rgb_matrix_hs_indicator_set(HS_RGB_BLINK_INDEX_VAD, (RGB){0x20, 0x20, 0x20}, 250, 3);
    rgb_matrix_hs_indicator_set(HS_RGB_BLINK_INDEX_SPI, (RGB){0x20, 0x20, 0x20}, 250, 3);
    rgb_matrix_hs_indicator_set(HS_RGB_BLINK_INDEX_SPD, (RGB){0x20, 0x20, 0x20}, 250, 3);
}

bool hs_reset_settings_user(void) {

    rgb_matrix_hs_indicator_set(0xFF, (RGB){0x10, 0x10, 0x10}, 250, 3);

    return true;
}

void nkr_indicators_hook(uint8_t index) {

    if ((hs_rgb_indicators[index].rgb.r == 0x6E) && (hs_rgb_indicators[index].rgb.g == 0x00) && (hs_rgb_indicators[index].rgb.b == 0x00)) {

        rgb_matrix_hs_indicator_set(0xFF, (RGB){0x6E, 0x00, 0x00}, 250, 1);

    } else if ((hs_rgb_indicators[index].rgb.r == 0x00) && (hs_rgb_indicators[index].rgb.g == 0x6E) && (hs_rgb_indicators[index].rgb.b == 0x00)) {

        rgb_matrix_hs_indicator_set(0xFF, (RGB){0x00, 0x00, 0x6F}, 250, 1);
    }
}

void rgb_matrix_hs_indicator_set(uint8_t index, RGB rgb, uint32_t interval, uint8_t times) {

    for (int i = 0; i < HS_RGB_INDICATOR_COUNT; i++) {
        if (!hs_rgb_indicators[i].active) {
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
            break;
        }
    }
}

void rgb_matrix_hs_set_remain_time(uint8_t index, uint8_t remain_time) {

    for (int i = 0; i < HS_RGB_INDICATOR_COUNT; i++) {
        if (hs_rgb_indicators[i].index == index) {
            hs_rgb_indicators[i].times  = 0;
            hs_rgb_indicators[i].active = false;
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
                if (hs_rgb_indicators[i].index == 0xFF) {
                    rgb_matrix_set_color_all(hs_rgb_indicators[i].rgb.r, hs_rgb_indicators[i].rgb.g, hs_rgb_indicators[i].rgb.b);
                } else {
                    rgb_matrix_set_color(hs_rgb_indicators[i].index, hs_rgb_indicators[i].rgb.r, hs_rgb_indicators[i].rgb.g, hs_rgb_indicators[i].rgb.b);
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

#define LOCK_LIGHT 0xFF // Caps / Num / Win lock indicator brightness (stock: 0x20)

bool rgb_matrix_indicators_advanced_kb(uint8_t led_min, uint8_t led_max) {

    if (!rgb_matrix_indicators_advanced_user(led_min, led_max)) {
        return false;
    }

#ifdef WIRELESS_ENABLE
    power_ui_layer_highlight();
#endif

    if (ee_clr_timer && timer_elapsed32(ee_clr_timer) > 3000) {
        hs_reset_settings();
        ee_clr_timer = 0;
    }

    if (host_keyboard_led_state().caps_lock)
        rgb_matrix_set_color(HS_RGB_INDEX_CAPS, LOCK_LIGHT, LOCK_LIGHT, LOCK_LIGHT);
    else
        rgb_matrix_set_color(HS_RGB_INDEX_CAPS, RGB_BLACK);
    if (host_keyboard_led_state().num_lock)
        rgb_matrix_set_color(HS_RGB_INDEX_NUM_LOCK, LOCK_LIGHT, LOCK_LIGHT, LOCK_LIGHT);
    else
        rgb_matrix_set_color(HS_RGB_INDEX_NUM_LOCK, RGB_BLACK);
    if (!keymap_is_mac_system() && keymap_config.no_gui)
        rgb_matrix_set_color(HS_RGB_INDEX_WIN_LOCK, LOCK_LIGHT, LOCK_LIGHT, LOCK_LIGHT);
    else
        rgb_matrix_set_color(HS_RGB_INDEX_WIN_LOCK, RGB_BLACK);

    rgb_matrix_hs_indicator();

#ifdef WIRELESS_ENABLE
    power_ui_indicators();
    turbo_indicators();
#endif

    rgb_matrix_start_rec();

    return true;
}

void hs_reset_settings(void) {
    eeconfig_init();
    eeconfig_update_rgb_matrix_default();

    eeconfig_read_keymap(&keymap_config);

#if defined(NKRO_ENABLE) && defined(FORCE_NKRO)
    keymap_config.nkro = 0;
    eeconfig_update_keymap(&keymap_config);
#endif

    if (hs_reset_settings_user() != true) {

        return;
    }
    keyboard_post_init_kb();
}

void lpwr_wakeup_hook(void) {

    gpio_write_pin_high(LED_POWER_EN_PIN);
    gpio_write_pin_high(A9);
    gpio_write_pin_high(HS_LED_BOOSTING_PIN);
}
