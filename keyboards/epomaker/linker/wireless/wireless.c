
// Copyright 2024 Su (@isuua)
// SPDX-License-Identifier: GPL-2.0-or-later

#include "quantum.h"
#include "wireless.h"

#ifndef WLS_INQUIRY_BAT_TIME
#    define WLS_INQUIRY_BAT_TIME 3000
#endif

// Send-path invariants kept from 2026101301 (checked again in 2026101801, see the triage ledger):
//  1. Resend cap: an unacknowledged packet is resent up to MD_SEND_PKT_RETRY (40) times, then dropped
//     (module.c, md_send_pkt_task). Do not lower it or add a second limit here.
//  2. Queued-only: every report goes through md_send_*() into the queue (module.c). Never write to the
//     UART from here and never pump wireless_task() from inside a send.
//  3. Releases survive: key/consumer/system/mouse states are held in module.c's pend_buf when the queue is
//     full and queued later, so a release is never lost. wireless_devs_change() clears that backlog first
//     (P12) and only then queues the releases for the old host (P13).
static uint8_t wls_devs = DEVS_USB;

void last_matrix_activity_trigger(void);

uint8_t wireless_keyboard_leds(void);
static void wireless_reselect(void);
void wireless_send_keyboard(report_keyboard_t *report);
void wireless_send_nkro(report_nkro_t *report);
void wireless_send_mouse(report_mouse_t *report);
void wireless_send_extra(report_extra_t *report);

host_driver_t wireless_driver = {
    .keyboard_leds = wireless_keyboard_leds,
    .send_keyboard = wireless_send_keyboard,
    .send_nkro     = wireless_send_nkro,
    .send_mouse    = wireless_send_mouse,
    .send_extra    = wireless_send_extra,
};

void wireless_init(void) {

    md_init();
}

// P11: link down, a report arrived. Re-send device-select at most once per 500 ms
// (was: every report, which filled the queue and held off sleep). The report is dropped.
#ifndef WLS_RESELECT_INTERVAL
#    define WLS_RESELECT_INTERVAL 500
#endif
static void wireless_reselect(void) {
    static uint32_t timer = 0;
    static bool     armed = false;

    if (armed && sync_timer_elapsed32(timer) < WLS_RESELECT_INTERVAL) {
        return;
    }
    armed = true;
    timer = sync_timer_read32();
    wireless_devs_change(wls_devs, wls_devs, false);
}

uint8_t wireless_keyboard_leds(void) {

    if (*md_getp_state() == MD_STATE_CONNECTED) {
        return *md_getp_indicator();
    }

    return 0;
}

void wireless_send_keyboard(report_keyboard_t *report) {
    if(MD_STATE_PAIRING == *md_getp_state()){
        return;
    }
    uint8_t wls_report_kb[MD_SND_CMD_KB_LEN] = {0};

    if (*md_getp_state() != MD_STATE_CONNECTED) {
        wireless_reselect();
        return;
    }

    if (report != NULL) {
        memcpy(wls_report_kb, (uint8_t *)report, sizeof(wls_report_kb));
    }
    md_send_kb(wls_report_kb);
}

void wireless_send_nkro(report_nkro_t *report) {
    static report_keyboard_t temp_report_keyboard = {0};
    uint8_t wls_report_nkro[MD_SND_CMD_NKRO_LEN]  = {0};

    if(MD_STATE_PAIRING == *md_getp_state()){
        return;
    }

    if (*md_getp_state() != MD_STATE_CONNECTED) {
        wireless_reselect();
        return;
    }

    if (report != NULL) {
        report_nkro_t temp_report_nkro = *report;
        uint8_t key_count              = 0;

        temp_report_keyboard.mods = temp_report_nkro.mods;
        for (uint8_t i = 0; i < NKRO_REPORT_BITS; i++) {
            key_count += __builtin_popcount(temp_report_nkro.bits[i]);
        }

        // find key up and del it.
        for (uint8_t i = 0; i < KEYBOARD_REPORT_KEYS && temp_report_keyboard.keys[i]; i++) {
            uint8_t usageid = 0x00;
            uint8_t n;

            for (uint8_t c = 0; c < key_count; c++) {
                for (n = 0; n < NKRO_REPORT_BITS && !temp_report_nkro.bits[n]; n++) {}
                if (n == NKRO_REPORT_BITS) break; // no bits left
                usageid = (n << 3) | biton(temp_report_nkro.bits[n]);
#ifdef NKRO_ENABLE
                del_key_bit(&temp_report_nkro, usageid);
#endif
                if (usageid == temp_report_keyboard.keys[i]) {
                    break;
                }
            }

            if (usageid != temp_report_keyboard.keys[i]) {
                temp_report_keyboard.keys[i] = 0x00;
            }
        }

        /*
         * Use NKRO for sending when more than 6 keys are pressed
         * to solve the issue of the lack of a protocol flag in wireless mode.
         */

        temp_report_nkro = *report;

        for (uint8_t i = 0; i < key_count; i++) {
            uint8_t usageid;
            uint8_t idx, n = 0;

            for (n = 0; n < NKRO_REPORT_BITS && !temp_report_nkro.bits[n]; n++) {}
            if (n == NKRO_REPORT_BITS) break; // no bits left
            usageid = (n << 3) | biton(temp_report_nkro.bits[n]);
#ifdef NKRO_ENABLE
            del_key_bit(&temp_report_nkro, usageid);
#endif

            for (idx = 0; idx < KEYBOARD_REPORT_KEYS; idx++) {
                if (temp_report_keyboard.keys[idx] == usageid) {
                    break;
                }
                if (temp_report_keyboard.keys[idx] == 0x00) {
                    temp_report_keyboard.keys[idx] = usageid;
                    break;
                }
            }

            if (idx == KEYBOARD_REPORT_KEYS && (usageid < (MD_SND_CMD_NKRO_LEN * 8))) {
                wls_report_nkro[usageid / 8] |= 0x01 << (usageid % 8);
            }
        }
    } else {
        memset(&temp_report_keyboard, 0, sizeof(temp_report_keyboard));
    }

    // both reports are queued in order; never pump wireless_task() from inside a send
    host_keyboard_send(&temp_report_keyboard);
    md_send_nkro(wls_report_nkro);
}

void wireless_send_mouse(report_mouse_t *report) {
    
    if(MD_STATE_PAIRING == *md_getp_state()){
        return;
    }

    typedef struct {
        uint8_t buttons;
        int8_t x;
        int8_t y;
        int8_t z;
        int8_t h;
    } __attribute__((packed)) wls_report_mouse_t;

    wls_report_mouse_t wls_report_mouse = {0};

    if (*md_getp_state() != MD_STATE_CONNECTED) {
        wireless_reselect();
        return;
    }

    if (report != NULL) {
        wls_report_mouse.buttons = report->buttons;
        wls_report_mouse.x       = report->x;
        wls_report_mouse.y       = report->y;
        // P19 (OPEN, board test): z <- h and h <- v looks swapped against QMK (v = wheel, h = tilt), but the
        // module's report layout is unknown. Kept as shipped until wheel/tilt is tested over BT and 2.4G.
        wls_report_mouse.z       = report->h;
        wls_report_mouse.h       = report->v;
    }

    md_send_mouse((uint8_t *)&wls_report_mouse);
}

void wireless_send_extra(report_extra_t *report) {
    if(MD_STATE_PAIRING == *md_getp_state()){
        return;
    }

    uint16_t usage = 0;

    if (*md_getp_state() != MD_STATE_CONNECTED) {
        wireless_reselect();
        return;
    }

    if (report != NULL) {
        usage = report->usage;

        // by report id: a system release (usage 0) must clear the system bits, not consumer
        if (report->report_id == REPORT_ID_SYSTEM) {
            usage = (usage >= 0x81 && usage <= 0x83) ? 0x01 << (usage - 0x81) : 0;
            md_send_system((uint8_t *)&usage);
        } else {
            md_send_consumer((uint8_t *)&usage);
        }
    }
}

void wireless_devs_change_user(uint8_t old_devs, uint8_t new_devs, bool reset) __attribute__((weak));
void wireless_devs_change_user(uint8_t old_devs, uint8_t new_devs, bool reset) {}

void wireless_devs_change_kb(uint8_t old_devs, uint8_t new_devs, bool reset) __attribute__((weak));
void wireless_devs_change_kb(uint8_t old_devs, uint8_t new_devs, bool reset) {}

void wireless_devs_change(uint8_t old_devs, uint8_t new_devs, bool reset) {
    bool changed        = (old_devs == DEVS_USB) ? (new_devs != DEVS_USB) : (new_devs == DEVS_USB);
    bool device_changed = (wls_devs != new_devs) || reset;

    if (device_changed) {
        // P12: drop commands still queued for the old link, so the switch/PAIR goes out at once
        md_clear_queue();

        // P13: release held keys on the old host while its link is still up
        // (USB<->wireless is released by set_transport)
        if (get_transport() == TRANSPORT_WLS && *md_getp_state() == MD_STATE_CONNECTED) {
            report_mouse_t mouse = {0};
            clear_keyboard();
            host_consumer_send(0);
            host_system_send(0);
            host_mouse_send(&mouse);
        }
    }

    if (changed) {
        set_transport((new_devs != DEVS_USB) ? TRANSPORT_WLS : TRANSPORT_USB);
    }

    if (device_changed) {
        *md_getp_state()     = MD_STATE_DISCONNECTED;
        *md_getp_indicator() = 0;
    }

    wls_devs = new_devs;
    last_matrix_activity_trigger();

    md_devs_change(new_devs, reset);
    wireless_devs_change_kb(old_devs, new_devs, reset);
    wireless_devs_change_user(old_devs, new_devs, reset);
}

uint8_t wireless_get_current_devs(void) {
    return wls_devs;
}

void wireless_pre_task(void) __attribute__((weak));
void wireless_pre_task(void) {}

void wireless_post_task(void) __attribute__((weak));
void wireless_post_task(void) {}

void wireless_task(void) {

    static uint8_t last_state = MD_STATE_NONE;

    wireless_pre_task();
    lpwr_task();
    md_main_task();
    wireless_post_task();

    // link back up: keys or knob released while it was down never reached the
    // host, so send the current state again
    if (*md_getp_state() != last_state) {
        last_state = *md_getp_state();
        if (last_state == MD_STATE_CONNECTED && get_transport() == TRANSPORT_WLS) {
            report_extra_t extra = {.report_id = REPORT_ID_SYSTEM};
            report_mouse_t mouse = {0};
            send_keyboard_report();
            wireless_send_extra(&extra);
            extra.report_id = REPORT_ID_CONSUMER;
            wireless_send_extra(&extra);
            wireless_send_mouse(&mouse);
        }
    }

    /* usb_remote_wakeup() should be invoked last so that we have chance
     * to switch to wireless after start-up when usb is not connected
     */
    if (get_transport() == TRANSPORT_USB) {
        usb_remote_wakeup();
    }
    // battery in every mode (also USB on battery: the critical-level sleep needs it)
    if (lpwr_get_state() == LPWR_NORMAL) {
        static uint32_t inqtimer = 0x00;

        if (sync_timer_elapsed32(inqtimer) >= (WLS_INQUIRY_BAT_TIME)) {
            if (md_inquire_bat()) {
                inqtimer = sync_timer_read32();
            }
        }
    }
}

// called from the keyboard's housekeeping_task_kb
void wireless_housekeeping_task(void) {
    wireless_task();
}