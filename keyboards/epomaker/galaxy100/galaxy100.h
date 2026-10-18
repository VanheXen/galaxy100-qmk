// Copyright 2024 Epomaker (@Epomaker)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"

// single definition of the layer order, shared by galaxy100.c and every keymap
enum layers {
    _BL = 0,
    _FL,
    _MBL,
    _MFL,
    _FBL,
    GALAXY100_LAYER_COUNT,
};

#ifdef DYNAMIC_KEYMAP_LAYER_COUNT
_Static_assert(GALAXY100_LAYER_COUNT == DYNAMIC_KEYMAP_LAYER_COUNT, "layer enum and keyboard.json layer_count differ");
#endif
