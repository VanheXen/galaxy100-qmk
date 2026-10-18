# Galaxy100 (WB32) hardware notes

Facts verified on the board or extracted from the stock firmware. Epomaker's published
source gets several of these wrong.

## Revisions

| Revision | MCU | USB ID | This port |
|---|---|---|---|
| WB32 | WB32FQ95 | `342D:E489` | yes |
| ES32 | ES32 | `36B0:xxxx` | no (different MCU) |

Check the USB ID before flashing anything.

## Stock firmware backup

Bootloader: back switch on **USB**, hold **Esc** while plugging in the cable.

```sh
wb32-dfu-updater_cli -U stock.bin -Z 262144   # read all 256 KB
sha256sum stock.bin                            # keep it with the file
wb32-dfu-updater_cli -D stock.bin              # restore
```

Never pass `-t`: it disables read protection, which erases the flash.

## Stock firmware layout (256 KB dump)

| Offset | Content |
|---|---|
| `0x1049A` | keymap, 5 layers |
| `0x114DC` | LED map: matrix table, then LED points, then flags |

The LED map has **104** entries. LEDs 0–2 are not under keys (status lights under the right
arrow); key LEDs start at 3. Epomaker's published map has 101 entries, so every key is off by 3.

## Pins

| Function | Pin | Notes |
|---|---|---|
| Rows | A0 A1 A2 A3 A4 C13 | ROW2COL |
| Cols | C0 C1 C2 C3 A6 B10 B11 B12 B13 B14 A10 C6 C7 C8 C9 A8 C4 C5 B0 | |
| Encoder | A = B7, B = B6 | B6 also wakes from sleep (B7 shares EXTI line 7 with A7) |
| WS2812 data | B15 | SPI (SPIM2) |
| LED power enable | A5 | plus A9, D2 (boost) |
| USB power enable | B1 | active low |
| Cable detect | A7 | high = cable present |
| Battery full | A15 | high = full |
| Wireless module UART | TX C10, RX C11 | UART3, 115200 |
| External flash (settings) | SPI SCK B3, MOSI B5, MISO B4, CS C12 | QSPI, wear leveling, 8 KB backing |
| Mode switch | C14, C15 | see below |

### Back switch (BT | USB | 2.4G)

The switch ties a pin to VCC; read both with pull-downs.

| Position | C14 | C15 |
|---|---|---|
| USB | 1 | 1 |
| BT | 1 | 0 |
| 2.4G | anything else (expected 0, 1) |

In the BT and 2.4G positions the switch also cuts the USB data lines, so USB console output
only works in the USB position.

## Wireless module

Westberry/Epomaker "linker" module over UART. Driven by `keyboards/epomaker/linker/wireless`
(carlosedp fork lineage). Battery percentage is polled by the module only when not on USB;
the firmware polls it on the cable too (every 3 s, `WLS_INQUIRY_BAT_TIME`) for the VIA readout.

## Settings storage

Wear-levelled external flash, 8 KB backing (4 KB logical). Layout:

- QMK core eeconfig
- keyboard datablock (`EECONFIG_KB_DATA_SIZE`): `confinfo` (4 B: mode, last BT host, Custom
  Paint slot) + `power_ui` settings (magic + fields)
- user datablock: Custom Paint, 3 slots × 6 × 19 × 2 B
- VIA dynamic keymap / macros

Any change to these sizes moves everything after it: an **Esc-hold** flash (full reset) is
required and is noted in the changelog.
