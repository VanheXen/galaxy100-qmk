# DESIGN — Intent & Aesthetic Contract

This is a firmware project (QMK C for the Epomaker Galaxy100), so the "aesthetic" is the feel of the keyboard: honest behaviour, quiet power use, predictable lights. This file is the contract for run 2026101802.

## Goal

Triage and fix the leftovers of run 2026101801 and ship them as version `2026101802`. That run closed partial: 26 tasks ok, 0 failed, 53 problems recorded, and ledger `docs/triage/2026101801-ledger.md` still has OPEN rows. Its version (CI run 39130103928, commit 3f9c298, branch `fix/2026101801`) was compiled by CI only: it was never flashed, never board-tested, not tagged, not released.

## Scope (settled)

A **leftover** (CONTEXT.md) is BOTH populations:

- (a) all 53 problems recorded on run 2026101801, and
- (b) every ledger row still OPEN in `docs/triage/2026101801-ledger.md`: firmware P15, P16, P17, P19, P22, P24, P25, P28, P29, P31, P34, P39, P41; CI-only P07, P10, P45; owner-action P43, P44; unitemised P46–P57 and any row beyond P57 (read the full ledger and CHANGELOG.md to enumerate them).

Already FIXED in 2026101801 and left as recorded, not reworked: P04, P11, P12, P13, P14, P23, P26, P27, P32, P33, P38.

Every leftover ends the run as FIXED, OPEN with a stated reason, or closed by a recorded rescan.

## Rules

1. **Fix every OPEN ledger row regardless of category.** Cosmetic and code-quality rows are fixed too; they are not left OPEN because CONTEXT.md's definition of a "correctness or power bug" excludes them.
2. **Exception:** P43 and P44 are owner actions outside the repository. Carry them forward as OPEN (owner action pending) and list them under "Left open" in the changelog entry.
3. **P46–P57 and beyond** (no record, never itemised): rescan the code once more against the CONTEXT.md definition of a **correctness or power bug** (lost or stuck keys, memory corruption, corrupted wireless traffic, wrong lock or indicator state, unfair randomness, lights or radio active during sleep, missed deep sleep). Fix what the rescan finds; close rows with no distinct defect as "closed by rescan" with the rescan recorded in the ledger. Do not invent defects to fill the count.
4. **Persisted settings are allowed this run** because fixes can now be board-tested. Any fix that adds a persisted setting or alters the EEPROM layout is written up as "storage layout changed" in its ledger note, and the CHANGELOG heading becomes `## 2026101802 — storage layout changed` (Esc-hold flash / full settings reset when coming from an older build; Fn+Right Shift+Esc bootloader keeps settings via `SKIP_VERSION = yes`). Otherwise the heading stays `## 2026101802`.
5. **Version:** new version is `2026101802`; `VIA_FIRMWARE_VERSION` in `keyboards/epomaker/galaxy100/config.h` and `firmwareVersion` in `via/galaxy100_via.json` are bumped together, once, on branch `fix/2026101802`.

## Build and verification path

- Install the QMK toolchain locally (qmk CLI + arm-none-eabi-gcc + make); compile both keymaps (`via` and `default`) with `qmk compile -kb epomaker/galaxy100 -km <keymap>` before pushing. Both must compile.
- Then push `fix/2026101802` and read the GitHub Actions "Build firmware" run. Each later commit needs its own CI run read before being called compiled.
- **Baseline flash:** flash the 2026101801 CI artifact first, before any new change, and record the owner's board result per row in the new ledger `docs/triage/2026101802-ledger.md`.
- Use the canonical words: "compiled" means CI or local build passed; **board-tested** means flashed and checked by the owner. Never write "tested", "verified" or "works on hardware" for a build only compiled.
- Wording: use "correctness or power bug". Do not use "real bug", "serious problem" or "important issue".

## Deliverables

- `docs/triage/2026101802-ledger.md`: single list of work; each leftover row with status (FIXED / OPEN with reason / closed by rescan) and a user-facing note line.
- `CHANGELOG.md` entry `2026101802` with `Fixed` and `Left open` (P43, P44, and any row with a stated reason).
- README changed only if a user-visible key or menu changes.
- INTEGRATION.md updated for the new version, local build path and storage-layout rule.

## End state

Once the fixes compile locally, pass CI on `fix/2026101802` and are board-tested: merge into `main` and create tag `v2026101802` (a `v*` tag triggers the GitHub release, so this is a publishing step taken only after those three conditions hold).

## Taste non-negotiables

- Correct and quiet over clever: no lost or stuck keys, no lights or radio during sleep, indicators that tell the truth.
- Minimal, row-scoped changes: touch only the files a row's classification names; do not rework FIXED rows.
- Every claim of status in the ledger is backed by what actually ran (compile, CI, board report), and says so when a check could not run.
- No commit trailers such as Co-Authored-By or "Generated with" lines.
