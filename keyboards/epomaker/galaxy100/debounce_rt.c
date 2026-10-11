// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Per-key debounce with time and mode adjustable at runtime from VIA.
//   defer: a change is reported once the key has been stable for N ms
//   eager: a change is reported at once, then the key is ignored for N ms
//          (lower latency, same protection against chatter)
// 0 ms = no debounce.

#include "debounce.h"
#include "timer.h"

uint8_t debounce_rt_ms   = DEBOUNCE;
uint8_t debounce_rt_mode = 0; // 0 = defer, 1 = eager

static matrix_row_t pending[MATRIX_ROWS]; // defer: waiting / eager: locked out
static uint16_t     started[MATRIX_ROWS][MATRIX_COLS];

void debounce_init(void) {}

bool debounce(matrix_row_t raw[], matrix_row_t cooked[], bool changed) {
    bool     cooked_changed = false;
    uint16_t now            = timer_read();

    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        matrix_row_t diff = raw[r] ^ cooked[r];

        if (debounce_rt_mode) {
            // eager: release lockouts that expired
            for (uint8_t c = 0; pending[r] && c < MATRIX_COLS; c++) {
                matrix_row_t bit = (matrix_row_t)1 << c;
                if ((pending[r] & bit) && TIMER_DIFF_16(now, started[r][c]) >= debounce_rt_ms) pending[r] &= ~bit;
            }
            matrix_row_t take = diff & ~pending[r];
            if (!take) continue;
            for (uint8_t c = 0; c < MATRIX_COLS; c++) {
                matrix_row_t bit = (matrix_row_t)1 << c;
                if (!(take & bit)) continue;
                cooked[r] ^= bit;
                if (debounce_rt_ms) {
                    pending[r] |= bit;
                    started[r][c] = now;
                }
                cooked_changed = true;
            }
            continue;
        }

        pending[r] &= diff; // defer: keys that settled back cancel their timer
        if (!diff) continue;
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            matrix_row_t bit = (matrix_row_t)1 << c;
            if (!(diff & bit)) continue;
            if (!(pending[r] & bit)) {
                pending[r] |= bit;
                started[r][c] = now;
            }
            if (TIMER_DIFF_16(now, started[r][c]) >= debounce_rt_ms) {
                cooked[r] ^= bit;
                pending[r] &= ~bit;
                cooked_changed = true;
            }
        }
    }
    return cooked_changed;
}

// switching modes must not leave stale timers/lockouts behind
void debounce_rt_reset(void) {
    for (uint8_t r = 0; r < MATRIX_ROWS; r++) pending[r] = 0;
}
