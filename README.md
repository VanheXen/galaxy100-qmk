# Epomaker Galaxy100 on mainline QMK

Open firmware for the **WB32 revision** of the Epomaker Galaxy100 (USB `342D:E489`), built on
mainline [QMK](https://github.com/qmk/qmk_firmware) with full wired, 2.4 GHz and Bluetooth support.

> Not for the ES32 revision (USB `36B0:xxxx`), which uses a different MCU.

## What's different from stock

- **Fixed wake-up delay**: no forced 1-minute sleep while typing, no 1 s USB resume delay,
  sleep timing configurable.
- **Correct LED map** (104 LEDs, including the 3 status LEDs under the right arrow), extracted
  from the stock firmware; Epomaker's published map was off by 3.
- **Back mode switch** (BT / USB / 2.4G) supported; Epomaker's published source left its pins out.
- **VIA menus** (Lighting / Keyboard tabs):
  - backlight with all 44 QMK effects + Custom Paint, Fn-layer highlight
  - status light: what it shows (smart events, connection, battery, lock keys…), where
    (hidden LEDs / Esc / Left Ctrl), brightness
  - power: dim, lights off on battery, deep sleep timeouts; auto Win/Mac on cable
  - typing: debounce mode (defer / eager) and time, SOCD (last / first wins, neutral)
  - battery readout, restore Keyboard-tab defaults, factory reset of all settings (behind an
    unlock toggle; the firmware itself is not changed)
  - VIA follows changes made from the keyboard (live refresh of sliders / effect)
- **Battery**: Fn+Space shows the level on the number row; low / critical battery handling;
  charging indication; smoothed readings.
- **Turbo**: Fn+Right Alt, then press a key: it auto-fires until Fn+Right Alt again.
- Pairing feedback on the mode keys, hold-to-confirm Win/Mac switch, Fn+knob brightness.
- Caps / Num / Win lock indicators at full brightness, visible even with the backlight off.
- Firmware updates keep your VIA keymap, macros and settings (see Flash).

## Build

```sh
git clone --depth 1 https://github.com/qmk/qmk_firmware
cd qmk_firmware && git submodule update --init --recursive --depth 1
# copy (or link) this repo's keyboards/epomaker/{galaxy100,linker} into qmk_firmware/keyboards/epomaker/
qmk compile -kb epomaker/galaxy100 -km via
```

## Flash

**Back up your stock firmware first.** Enter the bootloader (hold **Esc** while plugging in the
cable, back switch on USB), then run `wb32-dfu-updater_cli -U stock.bin -Z 262144` and keep
`stock.bin` safe; restore it any time with `wb32-dfu-updater_cli -D stock.bin`. Never use `-t`
(it disables read protection and erases the flash).

1. Back switch on **USB**, cable connected.
2. Enter the bootloader:
   - **Fn + Right Shift + Esc**: keeps keymap, macros and settings (`SKIP_VERSION = yes` keeps
     VIA's data valid across builds);
   - or hold **Esc** while plugging in the cable: resets everything to the firmware defaults.
     Needed when an update changes the settings storage layout (noted in the changelog).
3. `wb32-dfu-updater_cli -w -D epomaker_galaxy100_via.bin` (`-w` waits for the bootloader),
   or `qmk flash`.

## VIA

Load [`via/galaxy100_via.json`](via/galaxy100_via.json) in usevia.app → Settings → Show Design tab →
Design → Load. New controls are hidden on older firmware via `{id_firmware_version}`.

**Factory reset** (Keyboard → Info and reset → Unlock factory reset): resets keymap, macros,
lighting and settings to this firmware's defaults. It does **not** change the firmware; to go
back to Epomaker's stock firmware, flash your `stock.bin` backup (see Flash).

## Fn layer (Windows)

| Combo | Action |
|---|---|
| Fn + F1…F12 | media / volume |
| Fn + Q / W / E | Bluetooth 1 / 2 / 3 (hold 3 s to pair) |
| Fn + R | 2.4G (hold 3 s to pair) |
| Fn + T | backlight on/off |
| Fn + Enter / \\ | next effect / color |
| Fn + ↑ ↓ ← → | brightness / speed |
| Fn + Space | battery level |
| Fn + Right Alt | turbo |
| Fn + Ins / Home / End / PrtSc | Custom Paint record / slots 1-3 |
| Fn + Win | Win key lock |
| Fn + S / A (hold 3 s) | Mac / Windows mode |
| Fn + B | NKRO |
| Fn + knob | brightness |
| Fn + Right Shift + Esc | bootloader |

## Credits and license

GPL-2.0-or-later. Board files from Epomaker's published Galaxy100 source; wireless stack
(`keyboards/epomaker/linker`) from Epomaker / isuua as maintained in
[carlosedp/qmk_firmware](https://github.com/carlosedp/qmk_firmware). Ideas borrowed from
[Keychron's QMK fork](https://github.com/Keychron/qmk_firmware).

## Changelog

Firmware versions are `YYYYMMDDNN` (shown to VIA as `id_firmware_version`).

- **2026101103**: VIA factory reset behind an unlock toggle; VIA live refresh waits for changes
  to settle (fixes the effect name lagging one step); lock indicators at full brightness.
- **2026101102**: turbo (Fn+Right Alt), Fn-layer highlight, Fn+knob brightness, battery
  readout and restore-defaults button in VIA, VIA live refresh. *Storage layout changed:
  Esc-hold reset needed when coming from older builds.*
- **20261011**: first release of the port.
