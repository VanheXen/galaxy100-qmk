// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

void power_ui_task(void);
bool power_ui_process_record(uint16_t keycode, keyrecord_t *record);
void power_ui_indicators(void);
void power_ui_init(void);
void power_ui_layer_highlight(void);
