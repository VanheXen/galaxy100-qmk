// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

void power_ui_task(void);
bool power_ui_process_record(uint16_t keycode, keyrecord_t *record);
void power_ui_indicators(void);
void power_ui_init(void);
void power_ui_layer_highlight(void);

// Overlay colors at the backlight's current brightness (incl. dimming and the
// battery cap), so indicators never outshine the effect: r/g/b are full-scale.
uint8_t ui_val(void);
void    ui_set_color(uint8_t led, uint8_t r, uint8_t g, uint8_t b);
// Same but scaled only by the battery cap (fixed-color indicator flashes).
void ui_set_color_capped(uint8_t led, uint8_t r, uint8_t g, uint8_t b);
// True while the Fn-layer highlight owns the key colors.
bool power_ui_layer_highlight_active(void);
