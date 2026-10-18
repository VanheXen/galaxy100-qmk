# Integration guide for run 2026101801

How the fix tasks of this run plug into the repo. Version: `2026101801`
(`VIA_FIRMWARE_VERSION` in `keyboards/epomaker/galaxy100/config.h`, already bumped; the VIA JSON
has no hard-coded version, it uses `{id_firmware_version}`).

## Build path

Mainline QMK plus this repo's two board folders linked into it. Same as README "Build" and
`.github/workflows/build.yml`:

```sh
git clone --depth 1 https://github.com/qmk/qmk_firmware
cd qmk_firmware && git submodule update --init --recursive --depth 1
# link keyboards/epomaker/galaxy100 and keyboards/epomaker/linker into qmk_firmware/keyboards/epomaker/
qmk compile -kb epomaker/galaxy100 -km via
qmk compile -kb epomaker/galaxy100 -km default
```

Both keymaps must compile. CI runs exactly these two commands, then publishes `epomaker_galaxy100_via.bin`.

### Status on this machine (checked 2026-10-18)

- `E:\...\Epomaker\qmk_firmware\keyboards\epomaker` already has `galaxy100` and `linker` as links
  to this repo, so the checkout side is ready.
- The `qmk` CLI, `arm-none-eabi-gcc` and `make` are **not installed** (`qmk --version` and
  `where` fail). A local compile was **not run**; nothing is compile-verified yet.
- Therefore verify by pushing the work to a branch and reading the GitHub Actions "Build firmware"
  result (runs on push to `main`, pull requests and manual dispatch). Nothing has been flashed or tested on the board.

## Ledger

`docs/triage/2026101801-ledger.md` is the single list of work. Row ids: `Pnn` = problem
(`G100-0nn`), `Tnn` = unfinished task of the previous run.

Problems table columns: `id | source file | description | category | sev | prior status | record-complete | note for next task`.
A fix task finds its rows by the **FIX** marker in the note column and the matching
classification section (power/sleep, wireless transport, keyboard/indicator/Custom Paint).

Planned FIX rows: P04, P11, P12, P13, P14, P23, P26, P27, P32, P33, P38. Fix order for wireless: P12, P13, P11.
All other rows stay OPEN with a reason.

## Rules for every fix task

1. Change only the files named in the row's classification section.
2. When done, edit the row in the ledger: prior status to `FIXED (2026101801)`, and say in the note what changed.
   Rows that turn out not to be fixable become `OPEN` with the reason.
3. Do **not** edit `CHANGELOG.md` directly (parallel tasks would conflict). Instead add one line per fixed row
   to the ledger note, written as a user-facing sentence in the style of earlier entries. The last task moves
   those lines into the `Fixed` section and the OPEN rows into `Left open` of the `2026101801` entry.
4. A fix that adds a persisted setting or changes the EEPROM layout is reported in its ledger note as
   "storage layout changed". The last task then renames the heading to `## 2026101801 — storage layout changed`.
   Planned fixes add no persisted setting so far.
5. Do not touch the version number again; it is `2026101801`.
6. Compile check: see Build path. If no local toolchain, say so in the task result rather than claiming it passed.

## Final assembly

- CHANGELOG `2026101801` entry complete (Fixed, Left open, layout note decided).
- README only needs a change if a user-visible key or menu changes (P04: default keymap knob).
- Push to a branch and confirm CI is green before any tag (`v*` tags trigger the release).
