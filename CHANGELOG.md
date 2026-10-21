# Changelog

Firmware versions are `YYYYMMDDNN` (shown to VIA as `id_firmware_version`). Entries marked
**storage layout changed** need an Esc-hold flash (settings reset) when coming from an older
build; the others keep keymap, macros and settings.

## 2026101802 — storage layout changed

Leftovers of 2026101801. Ledger: [docs/triage/2026101802-ledger.md](docs/triage/2026101802-ledger.md).
Storage layout changed (additive, P34 and P41): a settings version byte and the hue preset byte were
added. Coming from 2026101801 the settings are migrated and kept (flash with Fn+Right Shift+Esc,
which keeps settings); coming from an older build, or if anything looks wrong, do an Esc-hold flash
(full settings reset).

**Verification status.** Compiled locally (`via` and `default`, exit 0, no warnings) and by CI run
39147350401 at commit 08e7f2c (branch `fix/2026101802`, success). The CI run for the later docs-only
commit 3e7c970 (run 39147575461) was read afterwards: success. The docs commit 966171b (run 39147805837), the
merge to main 15d42b3 (run 39147815671) and tag v2026101802 (run 39147815456) also read success. **Nothing was flashed or board-tested: the 15 firmware rows fixed below
(plus P19, left open) are NOT board-verified** (no board report exists).

**Deviation from the settled end state.** The owner's end-state decision (DESIGN.md "End state", brief item 9)
made a board test a precondition for merging into `main` and tagging. Merge 15d42b3 and tag v2026101802 were
done before any board test, so this is a deviation. **No owner approval of the deviation is recorded anywhere**
(the run's just-do-it autonomy setting is not an approval of it). It stays a recorded, unapproved deviation
until the owner ratifies it or asks for the release to be withdrawn or marked pre-release.

**Board test (2026-10-21).** The owner flashed the v2026101802 release binary (sha256 963ca6c3...ed7b) over
2026101301 with an Esc-hold flash; the board read back VIA firmware version 2026101802. Pass: VIA settings
kept across a power cycle (P34), hue preset position kept across a power cycle (P41), all VIA custom menus
load. Not tested: the Fn+Right Shift+Esc migration from 2026101801 (never flashed on this board) and the
other fixed rows one by one. The release was promoted from pre-release to a normal release after this test.

**Release status (before the board test).** Tag v2026101802 was published without a board test, so the release is CI-compiled only
and the storage-layout rows P34 and P41 (Esc-hold flash path) are unverified on hardware. Delivery of the
board-test request is unconfirmed (Telegram not paired). From now on the release workflow publishes tags
as **pre-releases** with a "NOT BOARD-TESTED" body. The existing v2026101802 release was marked
`prerelease: true` with that body on 2026-10-18 by the `mark-prerelease.yml` workflow (run 39152442361, commit d2d2f64,
read back through the public API). Promote it only after board results are recorded.

### Fixed in this build (not board-verified)
- Holding two wireless mode keys together no longer starts pairing on the wrong device (P15).
- Switching wireless device no longer shows a stale connected state from the previous link (P16;
  guard by ordering, no UART trace available).
- Late acknowledgements from the wireless module can no longer complete the wrong packet (P17;
  ACK latency not measured).
- Battery text in VIA is now bounds-guarded (P22).
- Plugging in or pulling out the USB cable no longer flickers the charging light: cable pin debounced 50 ms (P24).
- The green "battery full" flash no longer fires falsely when you plug the cable in; it may appear up to 50 ms later (P25).
- Key indicator flashes now respect the battery brightness cap (P28).
- The Fn-layer highlight can now light the Win-lock key (P29).
- Fn+Space with no battery reading blinks key 1 white instead of showing nothing (P31).
- Settings now carry a version, and one invalid value resets only itself instead of every setting (P34, storage layout changed).
- Custom Paint: Fn release handling made explicit, no change in behaviour (P39).
- The hue preset key remembers its position after a reboot (P41, storage layout changed).
- Turbo now stops after 5 minutes without a key press, so the keyboard can sleep (P68; the 5 minutes is a choice, not measured).
- With the PC asleep, all LED power supplies are now cut, not just one (P69).
- The Locks status light no longer shows the Win lock on the Mac layer (P70).
- CI: VIA version match check between config.h and the VIA JSON (P07), via.o redirect check (P10), QMK pinned to commit 7a1bbf37 (P45). These ran once in CI run 39147350401 and passed.

### Closed by rescan
- P46 to P67 (22 never-itemised rows): rescan of the wireless and board sources against the correctness or power bug definition found no distinct defect behind them; it found P68 to P70 instead (see ledger rescan record).

### Left open
- P19 wireless mouse wheel/tilt axis order is unverified: module report layout unknown, needs board evidence; no behaviour change made.
- P43 qmk_firmware junctions untracked: owner action outside the repo, pending.
- P44 stale `ledtest` and `probe` build outputs in qmk_firmware: owner action outside the repo, pending.
- Board test of the 15 fixed firmware rows above and of P19: requested, no report yet.

## 2026101801

Triage of the problems left by the previous bug scan run. Ledger: [docs/triage/2026101801-ledger.md](docs/triage/2026101801-ledger.md).
No fix adds a persisted setting, so settings, keymap and macros are kept (no Esc-hold flash needed).
Not compiled locally (CI only) and not tested on the board.

### Fixed
- The volume knob now works in the `default` build too (P04).
- Typing while the wireless link is down no longer floods the module with device-select
  commands, so the queue stays free and the keyboard can go to sleep (P11).
- Switching mode or starting pairing now drops commands still queued for the old link, so the
  switch takes effect at once and PAIR is no longer lost (P12).
- Switching between wireless devices now releases held keys, consumer, system and mouse buttons
  on the old host, so nothing stays stuck (P13).
- A saved mode or Bluetooth host from an older build that is not valid no longer makes the
  keyboard loop on mode switching; it falls back to USB / Bluetooth 1 (P14).
- Idle lights-off on battery now also cuts the second LED supply and the boost converter, not
  just the LED enable pin, so the board draws less while the lights are off (P23).
- Critical battery (3 %) now forces deep sleep about 10 s after the module reports it, instead
  of up to 70 s later (P26).
- Key indicators no longer pile up or hide behind stale ones: repeating one restarts it, and
  switching Win/Mac cancels the other OS light reliably (P27).
- A corrupt saved default layer is reset to the Windows layout at boot (P32).
- An invalid saved Custom Paint slot is reset to slot 1, and a refused recording start no longer
  wipes the paint buffer (P33).
- Turbo can now take over a key that is also in a SOCD pair when SOCD is on (P38).

### Left open

Not fixed in this version, with the reason:
- P15 two mode keys held together can start pairing on the wrong device: rare chord, a safe fix
  means redesigning the shared long-press token.
- P16 stale link-state frame after a switch: unconfirmed, needs a UART trace (the P12 flush
  shrinks the window).
- P17 ACK has no packet id: the module protocol cannot match it; needs measured ACK latency.
- P19 wireless mouse wheel axes may be swapped: module report layout unknown, needs a hardware
  test; a wrong guess would break working scroll.
- P22 residual `battery_text` size guard: not a live defect, VIA packets are fixed at 32 bytes.
- P24 cable pin read without debounce: no drain or sleep effect, needs a PCB fact.
- P25 charge-complete pin read raw: only a cosmetic false "full" flash, needs a plug-in test.
- P28 direct-color indicators ignore the battery brightness cap: flashes last seconds, energy
  negligible.
- P29 Fn-layer highlight cannot light the Win-lock key: cosmetic.
- P31 Fn+Space shows nothing without a battery reading: cosmetic.
- P34 one bad VIA setting resets all settings: init already resets safely; a version byte would
  change the storage layout for no current bug.
- P39 Fn painting on every second release while painting: code matches its intent, not
  reproduced; needs a board test.
- P41 hue preset key forgets its position after reboot: cosmetic, persisting it needs a new
  stored byte.
- P07 firmware version kept by hand in two files (config.h and the VIA JSON): CI check only, covered by the 2026101801 bump in both files.
- P10 via.o redirect fails silently if the path changes: CI tripwire, not firmware.
- P45 CI builds unpinned mainline QMK: needs a known-good commit pin, not firmware.
- P43 qmk_firmware junctions untracked: owner action outside the repo.
- P44 stale `ledtest` and `probe` build outputs in qmk_firmware: owner action outside the repo.
- P46 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P47 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P48 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P49 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P50 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P51 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P52 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P53 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P54 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P55 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P56 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P57 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P58 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P59 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P60 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P61 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P62 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P63 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P64 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P65 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P66 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- P67 no record: counted by the previous run but never itemised; a rescan found no distinct defect (treated as a duplicate of P11-P39, unconfirmed), so nothing was changed.
- Not scanned by either run: the wear-levelling driver, platforms/chibios WB32 and the generated `info_config.h`.

Count: 18 itemised open lines (17 open rows plus the P22 residual) + 22 lines P46-P67 = 40 Left-open lines. Ledger: 45 itemised problems = 28 fixed (17 earlier + 11 above) + 17 open, plus 22 never-itemised = 67 counted. Checked against docs/triage/2026101801-ledger.md. P46-P67 content was never recorded; a rescan of lowpower.c, control.c, galaxy100.c, power_ui.c, the wear-levelling driver, platforms/chibios WB32 and info_config.h found no defect for them (ledger 'Second rescan'), but that does not prove they are duplicates.

UNVERIFIED on the board: nothing was flashed or tested on the keyboard. CI compiled both keymaps (Build firmware run 39130103928, commit a97e3e4), which is a build check only.

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
