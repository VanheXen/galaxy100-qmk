# 2026101801 baseline board test

Status: **AWAITING OWNER REPORT. Nothing has been flashed or tested; every row below is "no report".**

An AI agent cannot flash the board. The owner flashes the build and reports pass or fail per row. Until a report arrives, nothing here is claimed as tested.

## Flash attempt, 2026-10-18 (run 2026101802, remedy)

The board (USB VID 342D, PID E489) is attached and running normal firmware. Everything possible short of a keypress was done:

- CI artifact download: HTTP 401 (artifact zips need a GitHub login; `gh` is not authenticated). Artifact sha256 `8af2e39b...b6bb` could therefore NOT be checked against a download.
- Substitute: built commit a97e3e4 (the commit run 39130103928 compiled, `VIA_FIRMWARE_VERSION 2026101801`) locally from a git worktree: `epomaker_galaxy100_via.bin`, 71600 B, sha256 `2f2d6d8cc513d77d25e45e1365bfb35f6bd8c8b496d21b017cbe57fe26c9d5f8`, at `exports/runs/55f165e4-1ddb-49ff-a8f5-872ecb6202ee/baseline-2026101801/`. This is a local rebuild, not the CI binary (different toolchain, so a different hash is expected). The version is stored as a number, not text, so it could not be confirmed inside the .bin; it comes from config.h at a97e3e4.
- Flash: `wb32-dfu-updater_cli` (QMK MSYS) found, and `flash.bat` (backup of the current image, then flash) written next to the .bin. Run result: "No DFU capable USB device available". The board is not in the bootloader, and entering it needs Fn+Right Shift+Esc (or Esc held while plugging in), which an AI agent cannot press.
- Result: NOT FLASHED. All 11 rows stay "no report".

To finish: put the board in the bootloader, run `flash.bat` from that folder, then fill the Outcome cells below.

## What to flash

- CI run 39130103928: https://github.com/VanheXen/galaxy100-qmk/actions/runs/39130103928 (branch `fix/2026101801`, success).
- Artifact `galaxy100-firmware` (contains `epomaker_galaxy100_via.bin` and `via/galaxy100_via.json`), 53467 bytes, sha256 `8af2e39b9954b1caebf61371b9679e19cb9412e4636402c1587471bba42cb6bb`, expires 2027-01-16.
- Commit note: run 39130103928 built commit **a97e3e4**, not 3f9c298 (see the ledger section "Compiled-code coverage after 3f9c298"). The later commits changed only docs and comments, so a97e3e4 holds the same compiled code. The artifact is the `via` keymap only. The `default` keymap (P04) is compiled by CI but not published as an artifact; build it locally to test P04 (`qmk compile -kb epomaker/galaxy100 -km default`, see INTEGRATION.md).
- Flash with the Fn+Right Shift+Esc bootloader (keeps settings) or an Esc-hold flash. No storage layout change in 2026101801.
- The `gh` CLI is not logged in on this machine, so the artifact has to be downloaded from the run page in a browser (GitHub login needed for artifact download).

Flash date: not yet flashed (no owner report as of 2026-10-18). The owner was asked on 2026-10-18 to flash this artifact and report per row. The run did not wait: rows stay "no report" and the 2026101802 work proceeds. A later report only needs the Outcome cells and the flash date edited here; any **fail** is then added to the 2026101802 ledger as a leftover.

## Results

Outcomes: pass / fail / not reproducible / no report. Report the cells marked "no report".

| row | what to observe on the board | outcome |
|---|---|---|
| P04 | `default` keymap build only: turn the volume knob; volume changes. (`via` build already had it.) | no report |
| P11 | Switch to BT or 2.4G with no host in range, type for several seconds, then stop. The board falls asleep normally and, after a host returns, reconnects and types without long delays. | no report |
| P12 | Switch mode (USB to BT1 to BT2 to 2.4G) while a link is down or pairing: the switch takes effect at once (about 1 s), and a long-press PAIR starts pairing instead of being lost. | no report |
| P13 | Hold a key and a media key (or mouse button), switch from BT1 to BT2 (wireless to wireless): the old host sees no stuck key or button. Repeat for USB to wireless. | no report |
| P14 | Boot with a valid saved mode and BT host: no mode-switch loop, backlight and connection stable. (Corrupt-value case needs an older-build EEPROM; mark "not reproducible" if you cannot make one.) | no report |
| P23 | On battery, wait for the idle lights-off: all LEDs are off and battery drain is lower than with lights on (a USB power meter or the battery % trend). Pressing a key restores the lights fully. | no report |
| P26 | Critical battery (3 %) forces deep sleep about 10 s after the module reports it. Likely "not reproducible" unless the battery is run down. | no report |
| P27 | Press Fn-layer indicator keys repeatedly (e.g. a lock or hue key): indicators restart, they do not pile up. Switch Win/Mac layout: the other OS light goes off. | no report |
| P32 | Boot with the normal saved default layer: Windows layout, correct Win/Mac light. (Corrupt-value case: mark "not reproducible" unless forced.) | no report |
| P33 | Fn+End to start Custom Paint recording and again to end it: the paint buffer is kept and the blink matches the recording state. | no report |
| P38 | With SOCD on for a pair (e.g. A/D), set Turbo on one of those keys: Turbo arms and repeats, and SOCD still resolves opposite presses on keys without Turbo. | no report |

## Rows reported failing

None reported yet. Any row reported as **fail** is copied into the 2026101802 ledger as a leftover to re-fix in 2026101802.
