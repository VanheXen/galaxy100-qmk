// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

enum { SOCD_OFF, SOCD_LAST_WINS, SOCD_NEUTRAL, SOCD_FIRST_WINS, SOCD_COUNT };
enum { SOCD_PAIRS_AD, SOCD_PAIRS_WASD, SOCD_PAIRS_ARROWS, SOCD_PAIRS_ALL, SOCD_PAIRS_COUNT };

extern uint8_t socd_mode;
extern uint8_t socd_pairs;
bool           socd_process_record(uint16_t keycode, keyrecord_t *record);
