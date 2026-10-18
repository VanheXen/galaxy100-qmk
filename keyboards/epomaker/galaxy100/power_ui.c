// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Power and status UI:
// - idle dimming: full -> dim -> off (battery only, LED power cut)
// - Fn+Space battery level, low / critical battery handling
// - status light (charging, battery, connection...) on the hidden LEDs, Esc or Left Ctrl
// - pairing feedback on the mode key (Fn+Q/W/E/R)
// - settings stored in the keyboard datablock and edited from VIA

#include QMK_KEYBOARD_H
#include "wireless.h"
#include "power_ui.h"
#include "socd.h"
#ifdef VIA_ENABLE
#    include "via.h"
#endif

#define PUI_DIM_DIVISOR 4      // dimmed brightness = brightness / 4
#define PUI_BAT_STEP 150 // Fn+Space bar: ms per key when filling/emptying
#define PUI_BAT_LOW 15         // low-battery warning, %
#define PUI_BAT_CRITICAL 3     // forced sleep, %
#define PUI_CRITICAL_HOLD 10000
#define PUI_LOW_BAT_DIM 10000  // dim after 10 s idle when the battery is low
#define PUI_PAIR_TIMEOUT (3 * 60 * 1000)
#define PUI_CONNECTED_SHOW 2000

// VIA dropdown choices, in milliseconds (0 = never)
static const uint32_t dim_opts[]   = {0, 30000, 60000, 120000, 300000, 600000};
static const uint32_t off_opts[]   = {0, 60000, 120000, 300000, 600000, 1800000};
static const uint32_t sleep_opts[] = {0, 600000, 1800000, 3600000, 7200000};

enum { SL_SMART, SL_CONNECTION, SL_BATTERY, SL_LOCKS, SL_EFFECT, SL_OFF, SL_COUNT };

enum {
    id_status_mode = 1,
    id_status_val,
    id_dim_after,
    id_off_after,
    id_sleep_after,
    id_debounce,
    id_socd,
    id_status_loc,
    id_debounce_mode,
    id_socd_pairs,
    id_auto_os,
    id_layer_hl,
    id_battery_text,
    id_reset,
    id_factory_unlock,
    id_factory_reset,
    id_bat_cap,
    id_turbo_hz,
};

enum { HL_OFF, HL_OTHERS_OFF, HL_WHITE, HL_COUNT }; // highlight keys of the held Fn layer

enum { LOC_HIDDEN, LOC_ESC, LOC_LCTRL, LOC_COUNT }; // where the status light is drawn

#define SETTINGS_MAGIC 0xA8

typedef struct {
    uint8_t magic;
    uint8_t status_mode;
    uint8_t status_val;
    uint8_t dim_idx;
    uint8_t off_idx;
    uint8_t sleep_idx;
    uint8_t debounce_ms;
    uint8_t socd;
    uint8_t status_loc;
    uint8_t debounce_mode;
    uint8_t socd_pairs;
    uint8_t auto_os;
    uint8_t layer_hl;
    uint8_t bat_cap;  // index into cap_pct
    uint8_t turbo_hz;
} settings_t;
_Static_assert(4 + sizeof(settings_t) <= EECONFIG_KB_DATA_SIZE, "EECONFIG_KB_DATA_SIZE too small");

static const settings_t defaults = {SETTINGS_MAGIC, SL_SMART, 0x80, 2, 3, 2, DEBOUNCE, SOCD_OFF, LOC_HIDDEN, 0, SOCD_PAIRS_AD, 0, HL_OTHERS_OFF, 0, 20};

static const uint8_t cap_pct[] = {100, 75, 50, 25}; // max brightness on battery, % of RGB_MATRIX_MAXIMUM_BRIGHTNESS
#define STATUS_VAL_MIN 8 // VIA slider minimum: 0 would make the status light invisible
#define TURBO_HZ_MIN 5
#define TURBO_HZ_MAX 50
static settings_t       set;

extern uint8_t debounce_rt_ms, debounce_rt_mode; // debounce_rt.c
extern uint8_t turbo_rate_hz;                     // turbo.c
void           debounce_rt_reset(void);
extern bool    charging_state; // galaxy100.c: cable present
void           lpwr_set_timeout_value(uint32_t timeout);

typedef enum { PUI_FULL, PUI_DIM, PUI_OFF } pui_state_t;

static pui_state_t state       = PUI_FULL;
static uint32_t    bat_show_at = 0; // Fn+Space pressed (0 = not showing)
static uint32_t    bat_release_at = 0; // Fn+Space released (0 = still held)

static bool on_cable(void) {
    return charging_state;
}

static bool bat_full(void) {
    return gpio_read_pin(BAT_FULL_PIN) == BAT_FULL_STATE;
}

#define PUI_BAT_SETTLE 60000

static uint8_t bat_shown = 0; // smoothed %, 0 = no reading yet

static uint8_t bat_pct(void) {
    return bat_shown;
}

// On the cable follow the module directly (charging). On battery, a lower or
// higher reading must persist for 60 s before it is shown, which hides the
// dips caused by LED load and the jumps when the lights turn off.
static void bat_filter_task(void) {
    static uint32_t differs_since = 0;
    uint8_t         raw           = *md_getp_bat();

    if (!raw || raw > 100) return; // 0 = no reading, >100 is not a percentage: keep the last good value
    if (!bat_shown || charging_state) {
        bat_shown     = raw;
        differs_since = 0;
        return;
    }
    if (raw == bat_shown) {
        differs_since = 0;
    } else if (!differs_since) {
        differs_since = timer_read32();
    } else if (timer_elapsed32(differs_since) > PUI_BAT_SETTLE) {
        bat_shown     = raw;
        differs_since = 0;
    }
}

// a >= b on the wrapping 32-bit timer
static bool not_before(uint32_t a, uint32_t b) {
    return TIMER_DIFF_32(a, b) < 0x80000000UL;
}

static bool blink(uint16_t period) {
    return (timer_read32() / period) % 2;
}

// 5 blinks at 1 s, repeated every 30 s
static bool low_bat_blink(void) {
    return (timer_read32() % 30000) < 5000 && blink(500);
}

static bool bat_low(void) {
    uint8_t pct = bat_pct();
    return !on_cable() && pct && pct <= PUI_BAT_LOW;
}

// ---------------------------------------------------------------------------
// Settings

static void apply_settings(void) {
    if (debounce_rt_mode != set.debounce_mode) debounce_rt_reset();
    debounce_rt_ms   = set.debounce_ms;
    debounce_rt_mode = set.debounce_mode;
    if (set.socd == SOCD_OFF || set.socd_pairs != socd_pairs) socd_release_all();
    socd_mode        = set.socd;
    socd_pairs       = set.socd_pairs;
    lpwr_set_timeout_value(sleep_opts[set.sleep_idx]);
    turbo_rate_hz = set.turbo_hz;
}

static void save_settings(void) {
    eeconfig_update_kb_datablock(&set, 4, sizeof(set));
}

void power_ui_init(void) {
    eeconfig_read_kb_datablock(&set, 4, sizeof(set));
    if (set.magic != SETTINGS_MAGIC || set.status_mode >= SL_COUNT || set.status_val < STATUS_VAL_MIN || set.dim_idx >= ARRAY_SIZE(dim_opts) || set.off_idx >= ARRAY_SIZE(off_opts) || set.sleep_idx >= ARRAY_SIZE(sleep_opts) || set.socd >= SOCD_COUNT || set.status_loc >= LOC_COUNT || set.debounce_mode > 1 || set.socd_pairs >= SOCD_PAIRS_COUNT || set.auto_os > 1 || set.layer_hl >= HL_COUNT || set.debounce_ms > 20 || set.bat_cap >= ARRAY_SIZE(cap_pct) || set.turbo_hz < TURBO_HZ_MIN || set.turbo_hz > TURBO_HZ_MAX) {
        set = defaults;
        save_settings();
    }
    apply_settings();
}

// ---------------------------------------------------------------------------
// Idle dimming

static void set_led_power(bool on) {
    // same three pins the sleep/wake hooks in galaxy100.c switch
    gpio_write_pin(LED_POWER_EN_PIN, on);
    gpio_write_pin(HS_LED_AUX_POWER_PIN, on);
    gpio_write_pin(HS_LED_BOOSTING_PIN, on);
}

// "Max brightness on battery", applied while drawing: the stored brightness is
// never touched, so the cap can't leak into EEPROM or fight Fn+arrows.
static uint8_t bat_cap_val(void) {
    return on_cable() ? 0xFF : (uint16_t)RGB_MATRIX_MAXIMUM_BRIGHTNESS * cap_pct[set.bat_cap] / 100;
}

// every effect converts its colors through this hook (quantum/rgb_matrix runners)
rgb_t rgb_matrix_hsv_to_rgb(hsv_t hsv) {
    uint8_t v = rgb_matrix_get_val(), cap = bat_cap_val();
    if (v > cap) hsv.v = (uint16_t)hsv.v * cap / v; // scale, so effects keep their shading
    return hsv_to_rgb(hsv);
}

// Brightness actually shown = the user's value, divided when dimmed. Any change
// we did not make ourselves (Fn+arrows, knob, VIA) becomes the new user value.
// Called every tick; cheap.
static uint8_t user_val = 0xFF, applied_val = 0xFF; // 0xFF = not seen yet

static void enter(pui_state_t next) {
    HSV hsv = rgb_matrix_get_hsv();

    if (hsv.v != applied_val) user_val = hsv.v;
    uint8_t want = next == PUI_DIM ? user_val / PUI_DIM_DIVISOR : user_val;
    if (next == PUI_OFF) want = hsv.v; // keep the value, cut the LED power
    set_led_power(next != PUI_OFF);
    if (want != hsv.v) rgb_matrix_sethsv_noeeprom(hsv.h, hsv.s, want);
    applied_val = want;
    state       = next;
}

// ---------------------------------------------------------------------------
// Pairing feedback

static uint32_t pair_since      = 0;
static uint32_t connected_at    = 0;
static uint8_t  last_link_state = 0xFF;

static void pairing_task(void) {
    uint8_t st = *md_getp_state();

    if (st != last_link_state) {
        if (st == MD_STATE_PAIRING) pair_since = timer_read32();
        if (st == MD_STATE_CONNECTED && last_link_state != 0xFF) connected_at = timer_read32();
        last_link_state = st;
    }
    // give up pairing after 3 min and go back to reconnecting
    if (st == MD_STATE_PAIRING && timer_elapsed32(pair_since) > PUI_PAIR_TIMEOUT) {
        uint8_t devs = wireless_get_current_devs();
        pair_since   = timer_read32(); // once, not every tick until the module answers
        wireless_devs_change(devs, devs, false);
    }
}

static uint8_t mode_key_led(uint8_t devs) {
    switch (devs) {
        case DEVS_BT1: return HS_RGB_BLINK_INDEX_BT1;
        case DEVS_BT2: return HS_RGB_BLINK_INDEX_BT2;
        case DEVS_BT3: return HS_RGB_BLINK_INDEX_BT3;
        case DEVS_2G4: return HS_RGB_BLINK_INDEX_2G4;
        default:       return NO_LED;
    }
}

static RGB connection_color(void) {
    switch (wireless_get_current_devs()) {
        case DEVS_USB: return (RGB){0xFF, 0xFF, 0xFF};
        case DEVS_2G4: return (RGB){0x00, 0xFF, 0x00};
        default:       return (RGB){0x00, 0x40, 0xFF}; // Bluetooth
    }
}

static void pairing_indicators(void) {
    uint8_t led = mode_key_led(wireless_get_current_devs());
    if (led == NO_LED) return;

    RGB     c  = connection_color();
    uint8_t st = *md_getp_state();
    bool    on;

    if (st == MD_STATE_PAIRING) {
        on = blink(1000); // slow: pairing
    } else if (st == MD_STATE_DISCONNECTED) {
        on = blink(250); // fast: reconnecting
    } else if (connected_at && timer_elapsed32(connected_at) < PUI_CONNECTED_SHOW) {
        on = true; // solid: just connected
    } else {
        return;
    }
    ui_set_color(led, on ? c.r : 0, on ? c.g : 0, on ? c.b : 0);
}

// ---------------------------------------------------------------------------
// Main task

static uint32_t critical_since = 0;
static uint32_t via_active_at  = 0;

#ifdef VIA_ENABLE
static bool     via_ui_dirty = false;
static uint32_t via_dirty_at = 0; // last keyboard-side change

// "87 % - charging" (ASCII, null terminated)
static void battery_text(char *out, uint8_t size) {
    uint8_t     pct   = bat_pct();
    const char *state = !on_cable() ? "on battery" : bat_full() ? "full" : "charging";
    char        buf[32];
    uint8_t     n = 0;
    if (!pct) {
        strncpy(buf, "no reading yet", sizeof(buf));
    } else {
        if (pct >= 100) buf[n++] = '1';
        if (pct >= 10) buf[n++] = '0' + (pct / 10) % 10;
        buf[n++] = '0' + pct % 10;
        snprintf(buf + n, sizeof(buf) - n, " %% - %s", state); // ASCII: VIA does not decode UTF-8 here
    }
    strncpy(out, buf, size);
    out[size - 1] = 0;
}

// Ask an open VIA to re-read everything after changes made from the keyboard.
// (VIA listens for 0x16/0x01 since protocol 12; sent only while VIA is in use.)
void replaced_hid_send(uint8_t *data, uint8_t length); // linker/wireless/md_raw.c: USB or wireless
static void via_ui_sync_task(void) {
    // wait until changes have settled (150 ms after the last one), so VIA reads final values
    if (!via_ui_dirty || timer_elapsed32(via_dirty_at) < 150) return;
    via_ui_dirty = false;
    if (!via_active_at || timer_elapsed32(via_active_at) > 600000) return; // VIA not used in the last 10 min
    uint8_t buf[32] = {0x16, 0x01}; // VIA packet size
    replaced_hid_send(buf, sizeof(buf));
}
#endif

// Smart status light events
#define SL_EVENT_CONN 3000
#define SL_EVENT_CABLE 5000
static uint32_t ev_devs_at = 0, ev_cable_at = 0, ev_full_at = 0;

static bool recent(uint32_t at, uint32_t span) {
    return at && timer_elapsed32(at) < span;
}

static void events_task(void) {
    static uint8_t last_devs = 0xFF;
    static int8_t  last_cable = -1, last_full = -1;
    uint8_t        devs = wireless_get_current_devs();
    bool           cable = on_cable(), full = cable && bat_full();

    if (devs != last_devs) {
        if (last_devs != 0xFF) ev_devs_at = timer_read32();
        last_devs = devs;
    }
    if (cable != last_cable) {
        if (last_cable >= 0) ev_cable_at = timer_read32();
        last_cable = cable;
    }
    if (full != last_full) {
        if (last_full >= 0 && full) ev_full_at = timer_read32();
        last_full = full;
    }
}

void power_ui_task(void) {
    uint32_t idle = last_input_activity_elapsed();

    // Asleep (or going to sleep): the MCU still wakes briefly between stop cycles
    // (module UART traffic). Touching the LED power here would switch the LEDs
    // back on for the whole night, showing only the lock lights on a black frame.
    if (lpwr_get_state() != LPWR_NORMAL) return;

    pairing_task();
    bat_filter_task();
    events_task();
#ifdef VIA_ENABLE
    via_ui_sync_task();
#endif

    // critically low on battery for 10 s: force deep sleep to protect the cell
    // read the raw module value: the 60 s smoothed one would delay this by a minute
    uint8_t pct = *md_getp_bat();
    if (!pct || pct > 100) pct = bat_pct(); // no valid raw reading: fall back to the smoothed value
    if (!on_cable() && pct && pct <= PUI_BAT_CRITICAL) {
        if (!critical_since) critical_since = timer_read32();
        if (timer_elapsed32(critical_since) > PUI_CRITICAL_HOLD) {
            critical_since = 0;
            lpwr_set_timeout_manual(true);
        }
    } else {
        critical_since = 0;
    }

    if (via_active_at) idle = MIN(idle, timer_elapsed32(via_active_at)); // VIA in use counts as activity
    if (bat_show_at) idle = 0;                                             // keep lights up while showing the level

    uint32_t dim = dim_opts[set.dim_idx], off = off_opts[set.off_idx];
    if (bat_low()) dim = dim ? MIN(dim, PUI_LOW_BAT_DIM) : PUI_LOW_BAT_DIM; // low battery: dim early
    if (!on_cable() && off && idle >= off) {
        enter(PUI_OFF);
    } else if (dim && idle >= dim) {
        enter(PUI_DIM);
    } else {
        enter(PUI_FULL);
    }
}

bool power_ui_process_record(uint16_t keycode, keyrecord_t *record) {
    // restore brightness before the key runs, so Fn+arrows adjust the real value
    if (record->event.pressed) enter(PUI_FULL);
#ifdef VIA_ENABLE
    if (keycode >= QK_RGB_MATRIX_ON && keycode <= QK_RGB_MATRIX_SPEED_DOWN) {
        via_ui_dirty = true; // press and release both restart the settle timer
        via_dirty_at = timer_read32();
    }
#endif

    if (keycode == BAT_CHK) {
        if (record->event.pressed) {
            md_inquire_bat();
            bat_show_at    = timer_read32();
            bat_release_at = 0;
        } else if (bat_show_at) {
            bat_release_at = timer_read32();
        }
        return false;
    }
    // SOCD runs after Turbo, from process_record_kb (galaxy100.c), so Turbo can claim a SOCD key first
    return true;
}

// ---------------------------------------------------------------------------
// LED overlays

uint8_t ui_val(void) {
    uint8_t v = MIN(rgb_matrix_get_val(), bat_cap_val());
    return v ? v : 0x20; // brightness 0: keep indicators faintly visible
}

void ui_set_color(uint8_t led, uint8_t r, uint8_t g, uint8_t b) {
    uint16_t v = ui_val();
    rgb_matrix_set_color(led, r * v / 255, g * v / 255, b * v / 255);
}

static void set_key(uint8_t row, uint8_t col, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t led = g_led_config.matrix_co[row][col];
    if (led != NO_LED) rgb_matrix_set_color(led, r, g, b);
}

static void set_key_ui(uint8_t row, uint8_t col, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t led = g_led_config.matrix_co[row][col];
    if (led != NO_LED) ui_set_color(led, r, g, b);
}

static RGB battery_color(uint8_t pct) {
    if (pct > 50) return (RGB){0x00, 0xFF, 0x00};
    if (pct > 20) return (RGB){0xFF, 0xA0, 0x00};
    return (RGB){0xFF, 0x00, 0x00};
}

static RGB charging_color(void) {
    if (bat_full()) return (RGB){0x00, 0xFF, 0x00};
    uint16_t t = (timer_read32() / 4) % 512; // ~2 s breathing cycle
    uint8_t  v = t < 256 ? t : 511 - t;
    return (RGB){v, v / 2, 0};
}

static void status_light(void) {
    RGB     c      = {0, 0, 0};
    uint8_t pct    = bat_pct();
    bool    linked = wireless_get_current_devs() == DEVS_USB || *md_getp_state() == MD_STATE_CONNECTED;

    switch (set.status_mode) {
        case SL_EFFECT:
            return;
        case SL_OFF:
            break;
        case SL_SMART: {
            // off unless something is happening
            uint8_t st  = *md_getp_state();
            bool    wls = wireless_get_current_devs() != DEVS_USB;
            if (wls && st == MD_STATE_PAIRING)
                c = blink(1000) ? connection_color() : (RGB){0, 0, 0};
            else if (wls && st == MD_STATE_DISCONNECTED)
                c = blink(250) ? connection_color() : (RGB){0, 0, 0};
            else if (bat_low())
                c = low_bat_blink() ? (RGB){0xFF, 0x00, 0x00} : (RGB){0, 0, 0};
            else if (recent(connected_at, SL_EVENT_CONN) || recent(ev_devs_at, SL_EVENT_CONN))
                c = connection_color();
            else if (recent(ev_full_at, SL_EVENT_CABLE))
                c = (RGB){0x00, 0xFF, 0x00};
            else if (recent(ev_cable_at, SL_EVENT_CABLE) && on_cable())
                c = charging_color();
            else if (bat_show_at)
                c = on_cable() ? charging_color() : connection_color();
            break;
        }
        case SL_CONNECTION:
            if (linked || blink(300)) c = connection_color();
            break;
        case SL_BATTERY:
            if (pct) c = battery_color(pct);
            break;
        case SL_LOCKS:
            if (keymap_config.no_gui)
                c = (RGB){0xFF, 0x00, 0x00};
            else if (host_keyboard_led_state().caps_lock)
                c = (RGB){0xFF, 0xFF, 0xFF};
            break;
    }
    uint8_t r = (uint16_t)c.r * set.status_val / 255, g = (uint16_t)c.g * set.status_val / 255, b = (uint16_t)c.b * set.status_val / 255;
    switch (set.status_loc) {
        case LOC_HIDDEN:
            for (uint8_t i = 0; i < 3; i++) rgb_matrix_set_color(i, r, g, b);
            break;
        case LOC_ESC:
            set_key(0, 0, r, g, b);
            break;
        case LOC_LCTRL:
            set_key(5, 0, r, g, b);
            break;
    }
}

void power_ui_indicators(void) {
    uint8_t pct = bat_pct();

    // Fn+Space: number row 1..0 shows 10..100 %. Fills one key every 150 ms,
    // stays full while held, empties one key every 150 ms after release
    // (a quick tap still fills completely first).
    if (bat_show_at) {
        uint8_t  level    = (pct + 9) / 10;
        uint32_t fill_end = bat_show_at + (uint32_t)level * PUI_BAT_STEP;
        uint8_t  lit      = MIN(level, timer_elapsed32(bat_show_at) / PUI_BAT_STEP + 1);

        if (bat_release_at) {
            uint32_t drain_from = not_before(bat_release_at, fill_end) ? bat_release_at : fill_end; // later of the two
            if (not_before(timer_read32(), drain_from)) {
                uint32_t gone = timer_elapsed32(drain_from) / PUI_BAT_STEP;
                lit           = gone >= level ? 0 : level - gone;
                if (!lit) bat_show_at = bat_release_at = 0;
            }
        }
        if (bat_show_at) {
            RGB c = battery_color(pct);
            for (uint8_t i = 0; i < 10; i++) {
                set_key_ui(1, 1 + i, i < lit ? c.r : 0, i < lit ? c.g : 0, i < lit ? c.b : 0);
            }
        }
    }

    pairing_indicators();
    status_light();
}

// ---------------------------------------------------------------------------
// VIA "Status light" and "Power" menus (custom channel)

#ifdef VIA_ENABLE
// any VIA traffic: restore full brightness first, so a dimmed value is never saved
bool via_command_kb(uint8_t *data, uint8_t length) {
    via_active_at = timer_read32();
    enter(PUI_FULL);
    return false; // let VIA handle the command
}

void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    uint8_t *command_id = &data[0], *channel_id = &data[1], *value_id = &data[2], *value = &data[3];

    if (*channel_id != id_custom_channel) {
        *command_id = id_unhandled;
        return;
    }

    uint8_t *field = NULL, min = 0, max = 0xFF;
    switch (*value_id) {
        case id_status_mode: field = &set.status_mode, max = SL_COUNT - 1; break;
        case id_status_val:  field = &set.status_val, min = STATUS_VAL_MIN; break;
        case id_dim_after:   field = &set.dim_idx, max = ARRAY_SIZE(dim_opts) - 1; break;
        case id_off_after:   field = &set.off_idx, max = ARRAY_SIZE(off_opts) - 1; break;
        case id_sleep_after: field = &set.sleep_idx, max = ARRAY_SIZE(sleep_opts) - 1; break;
        case id_debounce:    field = &set.debounce_ms, max = 20; break;
        case id_socd:        field = &set.socd, max = SOCD_COUNT - 1; break;
        case id_status_loc:  field = &set.status_loc, max = LOC_COUNT - 1; break;
        case id_debounce_mode: field = &set.debounce_mode, max = 1; break;
        case id_socd_pairs:  field = &set.socd_pairs, max = SOCD_PAIRS_COUNT - 1; break;
        case id_auto_os:     field = &set.auto_os, max = 1; break;
        case id_layer_hl:    field = &set.layer_hl, max = HL_COUNT - 1; break;
        case id_bat_cap:     field = &set.bat_cap, max = ARRAY_SIZE(cap_pct) - 1; break;
        case id_turbo_hz:    field = &set.turbo_hz, min = TURBO_HZ_MIN, max = TURBO_HZ_MAX; break;
    }

    if (*value_id == id_battery_text && *command_id == id_custom_get_value) {
        battery_text((char *)value, length - 3);
        return;
    }
    // factory reset: only after the unlock toggle (VIA has no confirm dialog)
    static uint8_t factory_unlocked = 0;
    if (*value_id == id_factory_unlock) {
        if (*command_id == id_custom_get_value) value[0] = factory_unlocked;
        if (*command_id == id_custom_set_value) factory_unlocked = value[0] ? 1 : 0;
        return; // save: nothing to store, the unlock is never persisted
    }
    if (*value_id == id_factory_reset) {
        if (*command_id == id_custom_get_value) value[0] = 0;
        if (*command_id == id_custom_set_value && factory_unlocked) {
            eeconfig_disable();    // everything back to this firmware's defaults on next boot
            soft_reset_keyboard(); // (keymap, macros, lighting, settings; pairings untouched)
        }
        return;
    }
    if (*value_id == id_reset && *command_id == id_custom_get_value) {
        value[0] = 0; // VIA reads every control on load, buttons included; a refusal breaks all menus
        return;
    }
    if (*value_id == id_reset && *command_id == id_custom_set_value) {
        set = defaults;
        apply_settings();
        save_settings();
        via_ui_dirty = true; // make VIA re-read the restored values
        via_dirty_at = timer_read32();
        return;
    }
    if (!field && *command_id != id_custom_save) {
        *command_id = id_unhandled; // unknown setting (e.g. newer JSON on older firmware)
        return;
    }

    switch (*command_id) {
        case id_custom_set_value:
            if (value[0] >= min && value[0] <= max) {
                *field = value[0];
                apply_settings();
            }
            break;
        case id_custom_get_value:
            value[0] = *field;
            break;
        case id_custom_save:
            save_settings();
            break;
        default:
            *command_id = id_unhandled;
            break;
    }
}
#endif

// ---------------------------------------------------------------------------
// Automatic Win/Mac layer, cable only (over 2.4G/BT the host talks to the
// dongle or radio, so the OS can't be detected). Off unless enabled in VIA.

void os_switch_to(bool mac); // galaxy100.c

#ifdef OS_DETECTION_ENABLE
bool process_detected_host_os_kb(os_variant_t os) {
    if (!process_detected_host_os_user(os)) return false;
    if (set.auto_os && wireless_get_current_devs() == DEVS_USB && os != OS_UNSURE) {
        os_switch_to(os == OS_MACOS || os == OS_IOS);
    }
    return true;
}
#endif

// ---------------------------------------------------------------------------
// While a Fn layer is held, keys that do nothing different there go dark, so the
// active ones stand out (optionally shown in white). Runs before the other
// indicators so Caps/status lights still draw on top.

void power_ui_layer_highlight(void) {
    uint8_t layer = get_highest_layer(layer_state);
    uint8_t base  = get_highest_layer(default_layer_state);
    if (layer <= base) return;

    for (uint8_t row = 0; set.layer_hl != HL_OFF && row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            uint8_t led = g_led_config.matrix_co[row][col];
            if (led == NO_LED) continue;
            keypos_t pos = {.row = row, .col = col};
            uint16_t kc  = keymap_key_to_keycode(layer, pos);
            // unused on this layer: transparent, empty, or the same as the base layer
            if (kc == KC_TRNS || kc == KC_NO || kc == keymap_key_to_keycode(base, pos)) {
                rgb_matrix_set_color(led, 0, 0, 0);
            } else if (set.layer_hl == HL_WHITE || !rgb_matrix_is_enabled()) {
                // backlight off (Fn+T): active keys have no color of their own, show them in white
                ui_set_color(led, 0xFF, 0xFF, 0xFF);
            }
        }
    }
    // the active wireless connection's mode key (Q/W/E = BT 1-3, R = 2.4G) in its color
    uint8_t led = mode_key_led(wireless_get_current_devs());
    if (led != NO_LED) {
        RGB c = connection_color();
        ui_set_color(led, c.r, c.g, c.b);
    }
}
