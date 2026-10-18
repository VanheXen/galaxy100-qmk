// Copyright 2024 Epomaker (@Epomaker)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define USB_POWER_EN_PIN B1    // USB ENABLE pin
#define LED_POWER_EN_PIN A5    // LED ENABLE pin
#define HS_BAT_CABLE_PIN A7    // USB insertion detection pin
#define HS_LED_BOOSTING_PIN D2 // LED BOOSTING
#define HS_LED_AUX_POWER_PIN A9 // second LED supply enable, switched with LED_POWER_EN_PIN

#define BAT_FULL_PIN A15
#define BAT_FULL_STATE 1

#define HS_RGB_INDICATOR_COUNT 8 // concurrent blink jobs
#define HS_RGB_BAT_COUNT 1

/* Status Indicator Lamp */

#define HS_RGB_INDEX_CAPS 47
#define HS_RGB_INDEX_NUM_LOCK 70
#define HS_RGB_INDEX_WIN_LOCK 12
#define HS_RGB_BLINK_INDEX_VAI 26
#define HS_RGB_BLINK_INDEX_VAD 6
#define HS_RGB_BLINK_INDEX_SPI 5
#define HS_RGB_BLINK_INDEX_SPD 7

#define HS_RGB_BLINK_INDEX_WIN 46
#define HS_RGB_BLINK_INDEX_MAC 45

/* Wireless */
#define MD_BT1_NAME "Galaxy100-1"
#define MD_BT2_NAME "Galaxy100-2"
#define MD_BT3_NAME "Galaxy100-3"
#define MD_DONGLE_PRODUCT "Galaxy100-2.4G"
// Idle time before deep sleep on battery. Longer = fewer slow wake-ups, more drain.
#define LPWR_TIMEOUT (30 * 60000)

/* Mode switch on the back: BT | USB | 2.4G. Pins verified with a probe:
 * the switch ties a pin to VCC, so read with pull-downs.
 * USB = C14 1, C15 1 | BT = C14 1, C15 0 | 2.4G = anything else (expected C14 0, C15 1) */
#define HS_BT_DEF_PIN C14
#define HS_2G4_DEF_PIN C15
#define HS_USB_PIN_STATE 1, 1
#define HS_BT_PIN_STATE 1, 0
#define HS_2G4_PIN_STATE x, x

/* Mode key LEDs: Fn+Q/W/E/R like stock; USB has no key, blinks the 5 key */
#define HS_RGB_BLINK_INDEX_BT1 49
#define HS_RGB_BLINK_INDEX_BT2 50
#define HS_RGB_BLINK_INDEX_BT3 51
#define HS_RGB_BLINK_INDEX_2G4 52
#define HS_RGB_BLINK_INDEX_USB 80

/* Keyboard datablock: confinfo (4) + power_ui settings (15) + spare.
 * The spare bytes let new settings be added without moving the Custom Paint
 * slots and VIA keymaps that follow in EEPROM (that would need an EEPROM reset). */
#define EECONFIG_KB_DATA_SIZE 32

/* Shown to VIA; must match "firmwareVersion" in the VIA JSON (YYYYMMDDNN) */
#define VIA_FIRMWARE_VERSION 2026101802

/* UART for wireless module */
#define UART_DRIVER SD3
#define UART_RX_PIN C11
#define UART_RX_PAL_MODE 7
#define UART_TX_PIN C10
#define UART_TX_PAL_MODE 7

/* Caps Word: tap both Shifts to type one WORD in caps */
#define BOTH_SHIFTS_TURNS_ON_CAPS_WORD

/* Encoder */
#define ENCODER_MAP_KEY_DELAY 1
// ponytail: only pin B (B6) wakes from sleep; pin A (B7) shares EXTI line 7 with the cable pin A7
#define ENCODER_WAKE_PIN B6

/* SPI */
#define SPI_DRIVER SPIDQ
#define SPI_SCK_PIN B3
#define SPI_MOSI_PIN B5
#define SPI_MISO_PIN B4

/* Flash */
#define EXTERNAL_FLASH_SPI_SLAVE_SELECT_PIN C12
#define WEAR_LEVELING_LOGICAL_SIZE (WEAR_LEVELING_BACKING_SIZE / 2)

/* RGB Matrix */
#define RGB_MATRIX_FRAMEBUFFER_EFFECTS
#define RGB_MATRIX_KEYPRESSES

/* WS2812 */
#define WS2812_SPI_DRIVER SPIDM2
#define WS2812_SPI_DIVISOR 32

/* Matrix size: comes from "matrix_pins" in keyboard.json (single source) */

/* rgb_record */
#define RGBREC_CHANNEL_NUM 3
#define ENABLE_RGB_MATRIX_RGBR_PLAY
#define EECONFIG_USER_DATA_SIZE (RGBREC_CHANNEL_NUM * MATRIX_ROWS * MATRIX_COLS * 2)
// paint slots sit right after the kb datablock: resizing it blanks them instead of garbling
#define EECONFIG_USER_DATA_VERSION (EECONFIG_USER_DATA_SIZE | ((uint32_t)EECONFIG_KB_DATA_SIZE << 16))
