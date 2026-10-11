# Changelog

Firmware versions are `YYYYMMDDNN` (shown to VIA as `id_firmware_version`). Entries marked
**storage layout changed** need an Esc-hold flash (settings reset) when coming from an older
build; the others keep keymap, macros and settings.

## 2026101104 — storage layout changed

New:
- VIA *Battery and power → Max brightness on battery* (no limit / 75 / 50 / 25 %). Brightness
  set from the keyboard still works; the cap applies only while on battery.
- Low battery (≤ 15 %) dims the lights after 10 s idle regardless of the dim setting.
- While Fn is held, the mode key of the active wireless connection (Q/W/E = BT 1–3, R = 2.4G)
  lights in its color (blue / green).
- VIA *Typing → Turbo rate* (5–50 presses per second).
- Caps Word: tap both Shifts to type one word in capitals (Caps LED lights while active).
- Repeat Key available (`QK_REP`, assign it in VIA via *Any*).
- Knob turn wakes the keyboard from deep sleep.
- Fn+Backspace held **10 s** = "Reset everything to defaults" (same as the VIA button), for when
  VIA is not reachable. Backspace blinks red while held, faster near the end. Was 3 s before.

Fixed:
- SOCD: turning it off (or changing the key set) while a key was held could leave that key
  pressed on the host.
- `housekeeping_task_user` is now free for keymaps (the keyboard uses `_kb`).
- Blink-job table shrunk from 104 to 8 entries (2.3 KB RAM).
- Dropped 20 bytes of unused Custom Paint storage; dead code in `rgb_record.c`.

## JSON update (no reflash)

Menus regrouped into Backlight / Indicators / Battery and power / Typing / Reset; resets renamed
so they can't be mistaken for a stock-firmware restore.

## 2026101103

VIA "Reset everything to defaults" behind an unlock toggle; VIA live refresh waits for changes to
settle (fixes the effect name lagging one step); lock indicators at full brightness.

## 2026101102 — storage layout changed

Turbo (Fn+Right Alt), Fn-layer highlight, Fn+knob brightness, battery readout and
restore-defaults button in VIA, VIA live refresh.

## 20261011

First release of the port.
