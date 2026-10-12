# Changelog

Firmware versions are `YYYYMMDDNN` (shown to VIA as `id_firmware_version`). Entries marked
**storage layout changed** need an Esc-hold flash (settings reset) when coming from an older
build; the others keep keymap, macros and settings.

## 2026101301 — storage layout changed

New:
- Boot sweep: a rainbow wave reveals the lighting at power-on and when waking from deep sleep.
- Konami code (↑ ↑ ↓ ↓ ← → ← → B A) sets off 3 s of rainbow rings. The keys still reach the
  computer as normal.
- Fn+D rolls a die, Fn+F flips a coin, shown on numpad 1–9 with the rest of the numpad dark
  (coin: ring = heads, cross = tails).
  New keycodes *Dice* and *Coin*; an existing VIA keymap keeps its old Fn+D / Fn+F until you
  assign them in VIA.

Changed:
- All indicators follow the backlight brightness (including dimming and the battery cap):
  Caps/Num/Win lock, battery bar, mode key, Fn layer white highlight, turbo, Fn+Backspace,
  dice/coin, boot sweep, Konami. Lock lights were fixed at full power (brighter than the
  backlight's own maximum); the status light keeps its own VIA brightness.

Fixed:
- Lock lights (e.g. Num Lock) stayed lit all night during deep sleep, keeping the LED power on
  and draining the battery: the light-off logic kept running in the short wake-ups between
  sleep cycles and switched the LED power back on.
- Deep sleep now also turns off the LED boost converter (A9 and D2, which waking up turns back
  on); before, only the LED enable pin was cut.
- With the switch on USB and no cable the keyboard never went to deep sleep (only the lights
  turned off), draining the battery. It now sleeps after the *Sleep after* time like on 2.4G/BT.
- PC asleep with the cable in: the lights now turn off like with stock QMK (they stayed on all
  night).
- Wireless: an unanswered packet was resent forever and a busy loop in the key report path
  could freeze the keyboard; now a packet is dropped after 40 tries and reports are only queued.
  Key releases are kept when the queue is full, the current keys are resent when the link comes
  back, system keys (sleep/power) are released properly, and corrupted module messages can no
  longer overwrite memory.
- Moving the switch from USB to BT/2.4G with the cable in no longer sends a garbage key report.
- Turning the knob while painting (Custom Paint) or with Turbo armed corrupted memory.
- Keys held when Custom Paint recording started stayed pressed; holding Fn+Backspace across it
  could trigger the 10 s full reset.
- Fn+Enter could get stuck in Custom Paint. It now cycles the effects only; Custom Paint has
  its own keys, and switching slots while painting keeps what was painted.
- Pairing that timed out after 3 min flooded the module with commands and kept the keyboard
  from sleeping.
- *Max brightness on battery* is applied while drawing: it no longer ends up saved as the
  normal brightness, and Fn+Up no longer lowers it.
- Turbo stops on sleep and on mode change.
- Typing while the keyboard is going to sleep keeps it awake; a modifier held to wake it is kept.
- Pairing (Fn+Q/W/E/R held 3 s) is no longer cancelled by touching another key or the knob.
- Battery level read on USB position too, so the critical-battery sleep works there.
- SOCD: changing the pairs in VIA while holding a key no longer leaves it stuck.
- Dice: all six faces equally likely.
- EEPROM reset now also clears the Custom Paint slots; 13 spare bytes reserved so future
  settings don't move the stored data again.

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
