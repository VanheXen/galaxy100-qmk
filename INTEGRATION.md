# Integration guide for run 2026101802

How the fix tasks of this run plug into the repo. Version: `2026101802`, branch `fix/2026101802`
(created from `a1408ec`, the head of `fix/2026101801`). `VIA_FIRMWARE_VERSION` in
`keyboards/epomaker/galaxy100/config.h` and the hard-coded `firmwareVersion` in
`via/galaxy100_via.json` are both `2026101802` (bumped together, once; do not touch again).
The VIA JSON `showIf` gates use `{id_firmware_version}`. Ledger: `docs/triage/2026101802-ledger.md`.
Previous version `2026101801` (ledger `docs/triage/2026101801-ledger.md`) stays as recorded.

## Chosen build path

**2026101801 used GitHub Actions on branch `fix/2026101801`** because no local toolchain existed then
(a local toolchain is now installed: see "Local toolchain" below). `.github/workflows/build.yml` (which also triggers on `fix/**` pushes) compiles both
keymaps. CI run 39130103928 on `fix/2026101801` succeeded for `via` and `default`
(commit 3f9c298). Later commits (including comment-only edits) need their own CI run read before
being called compiled. For 2026101801 no tag and no release were made (it was never board-tested); 2026101802 was tagged and released without a board test (see "Status for 2026101802").

## Storage-layout flag rule

A change that adds a persisted setting or alters the EEPROM layout must be written up as
"storage layout changed" in its ledger note, and the CHANGELOG heading becomes
`## 2026101802 — storage layout changed` (users coming from an older build then need an Esc-hold
flash / full settings reset; Fn+Right Shift+Esc bootloader keeps settings via `SKIP_VERSION = yes`).
Persisted settings are allowed in 2026101802 (fixes can be board-tested). If no fix changes the
layout, the heading stays `## 2026101802`.

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

### Local toolchain (installed 2026-10-18, run 2026101802) — supersedes the CI-only path above

Local compile is now the primary check; CI is the second check.

- Installed: QMK MSYS 1.12.0 (`QMK_MSYS.exe`, SHA256 `21df7d55f80870b22121d99867a541cc2a2ed834a28b908d379006a1f7c24c1c`)
  into `C:\QMK_MSYS` (silent install `/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP-`).
  It supplies `arm-none-eabi-gcc` 15.2.0 (crosstool-NG, qmk/qmk_toolchains) at `/opt/qmk/bin` and GNU Make 4.4.1.
  MSYS's own `qmk` post-install failed (no python/uv/jq in the bundle), so the CLI is the Windows pip one:
  `py -3 -m pip install --user qmk` → qmk CLI 1.2.0 (Python 3.14 on Windows; `python` inside MINGW64 is 3.12.12).
- QMK checkout: `E:\...\Epomaker\qmk_firmware` at `7a1bbf37` (mainline, "Add support for mini_mighty/kbd (#25878)").
  `keyboards/epomaker/galaxy100` and `keyboards/epomaker/linker` are links to this repo.
- Run commands from a MINGW64 shell (QMK refuses `MSYSTEM=MSYS`). From cmd.exe, with cwd `C:\QMK_MSYS\usr\bin`:

```bat
set MSYSTEM=MINGW64&& bash.exe -l -c "export PATH=$PATH:/c/Users/Davide/Python/Python314/Scripts:/c/Users/Davide/AppData/Local/Programs/Python/Python314; export QMK_HOME=/e/Users/Davide/Documents/1-Projects/Epomaker/qmk_firmware; cd $QMK_HOME; qmk compile -kb epomaker/galaxy100 -km via; qmk compile -kb epomaker/galaxy100 -km default"
```

  The same two commands inside a QMK MSYS MinGW 64-bit terminal need only the `qmk compile` lines (set `QMK_HOME` if `qmk` is not on PATH).
- Output files (QMK copies them into the qmk_firmware root; build tree in `qmk_firmware/.build/`):
  `E:\...\Epomaker\qmk_firmware\epomaker_galaxy100_via.bin` (flash this one) and `epomaker_galaxy100_default.bin`
  (also `.hex`/`.elf` under `.build/`). The older `epomaker_galaxy100_ledtest.bin` and `..._probe.bin` in that folder are unrelated.
- **Baseline result, commit `a1408ec` on `fix/2026101801` (2026101801, nothing changed since CI):**
  `via` compiled, exit 0, 0 warnings, `.bin` 71600 bytes; `default` compiled, exit 0, 0 warnings, `.bin` 64616 bytes.
  The compile was local only; the firmware is still not flashed or board-tested.
- **Rule:** every push to `fix/2026101802` is preceded by both compiles passing locally (exit 0, no new warnings).
  Paste any failing output verbatim into the task result. CI on the pushed commit must also be green before merge or tag.

## Status for 2026101802 (checked 2026-10-18)

- **Version:** `2026101802` in `config.h` (`VIA_FIRMWARE_VERSION`) and `via/galaxy100_via.json` (`firmwareVersion`). Branch `fix/2026101802`.
- **Local compile:** both keymaps built with the commands in "Local toolchain" (`qmk compile -kb epomaker/galaxy100 -km via` and `-km default`); `via` exit 0, `default` exit 0, no warning/error lines, on the pushed code.
- **CI:** run **39147350401** ("Build firmware", push) at commit **08e7f2c176d583d97ae2f2d2aa19120e865cc476**: success; build job green, Release skipped (no tag). The P07, P10, P45 workflow steps passed in that run. Artifact `galaxy100-firmware` (54114 B).
- **Read (2026-10-18, public GitHub API):** the docs-only ledger commit `3e7c970` ran as CI run 39147575461: success. Later runs also success: 966171b (39147805837), main 15d42b3 (39147815671), tag v2026101802 (39147815456). These are compile checks only; no board test exists.
- **Board test: NOT DONE / unverified.** No board result exists for any row; nothing was flashed. The owner notification was queued (Telegram not paired) and may not have been delivered. All FIXED firmware rows are compile- and CI-verified only. P19 stays OPEN; P43 and P44 stay OPEN (owner actions).
- **Tag/release (verified 2026-10-18 via `git ls-remote` and the public GitHub API):** `fix/2026101802` was merged into `main` with merge commit `15d42b3`; annotated tag `v2026101802` exists on origin and points at `15d42b3`. GitHub release `v2026101802` was published (not draft) by the tag workflow as a normal release and was marked pre-release afterwards (see "Release gate scope"), with assets `epomaker_galaxy100_via.bin` (72452 B) and `galaxy100_via.json`. `main` is now at `83ffdc2` (docs after the merge). It was published **without any board test**: CI-compiled only, never flashed.
- **Release gate scope (fixed 2026-10-18):** the `prerelease: true` / NOT BOARD-TESTED gate in `.github/workflows/build.yml` (commit `ce8b39c`) applies to tags created after it. Release `v2026101802` (tag at `15d42b3`) predated it and was a normal release; `gh` is not authenticated here, so `.github/workflows/mark-prerelease.yml` (commit `d2d2f64`, Actions run 39152442361) ran `gh release edit v2026101802 --prerelease --notes-file docs/triage/2026101802-release-body-draft.md` with the repo's own Actions token. Read back via the public API: `prerelease: true`, body starts "NOT BOARD-TESTED". Promote to a normal release only after board results are recorded.
- **Deviation (recorded 2026-10-18):** the settled end state (DESIGN.md "End state", brief item 9) required a board test before merge and tag. Merge `15d42b3` and tag `v2026101802` happened without one. No owner approval of this deviation is recorded; it is unapproved until the owner ratifies it or asks for withdrawal. The release is now marked pre-release (see "Release gate scope"); the deviation itself stays unapproved. Local compile on `main` re-run: `via` 72452 B and `default` 65464 B, both built OK. QMK has no unit-test suite here; compile is the only check.
- `fix/2026101801` and the 2026101801 records (CI run 39130103928, commit 3f9c298) are unchanged.

## Ledger

`docs/triage/2026101802-ledger.md` is the single list of work for this run (the 2026101801 ledger is history). Row ids: `Pnn` = problem
(`G100-0nn`), `Tnn` = unfinished task of the previous run.

Problems table columns: `id | source file | description | category | sev | prior status | record-complete | note for next task`.
A fix task finds its rows by the **FIX** marker in the note column and the matching
classification section (power/sleep, wireless transport, keyboard/indicator/Custom Paint).

Population: every OPEN row P15–P67 (P15, P16, P17, P19, P22, P24, P25, P28, P29, P31, P34, P39, P41, CI-only P07, P10, P45,
unitemised P46–P67). P43 and P44 are owner actions and stay OPEN. Rows fixed in 2026101801
(P04, P11, P12, P13, P14, P23, P26, P27, P32, P33, P38) are not reworked.

## Rules for every fix task

1. Change only the files named in the row's classification section.
2. When done, edit the row in the 2026101802 ledger: status to `FIXED (2026101802)`, and say in the note what changed.
   Rows with no distinct defect become `closed by rescan` with the rescan recorded; rows that cannot be fixed stay `OPEN` with the reason.
3. Do **not** edit `CHANGELOG.md` directly (parallel tasks would conflict). Instead add one line per fixed row
   to the ledger note, written as a user-facing sentence in the style of earlier entries. The last task moves
   those lines into the `Fixed` section and the OPEN rows into `Left open` of the `2026101802` entry.
4. A fix that adds a persisted setting or changes the EEPROM layout is reported in its ledger note as
   "storage layout changed". The last task then renames the heading to `## 2026101802 — storage layout changed`.
5. Do not touch the version number again; it is `2026101802`.
6. Compile check: both keymaps locally first (see Local toolchain), then CI. Paste failing output verbatim.

## Final assembly

- CHANGELOG `2026101802` entry complete (Fixed, Left open incl. P43/P44, layout note decided).
- README only needs a change if a user-visible key or menu changes (P04: default keymap knob).
- Push to a branch and confirm CI is green before any tag (`v*` tags trigger the release).
