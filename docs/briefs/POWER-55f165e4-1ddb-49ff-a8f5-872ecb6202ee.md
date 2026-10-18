# Power Run Brief — 55f165e4-1ddb-49ff-a8f5-872ecb6202ee

## Goal

Triage and fix Galaxy100 run leftovers

## Shared understanding

GOAL: Triage and fix the Galaxy100 run leftovers, in project galaxy100-qmk (QMK C firmware at E:\Users\Davide\Documents\1-Projects\Epomaker\galaxy100-qmk, sub-project of Epomaker; key modules keyboards/epomaker/galaxy100/power_ui.c, galaxy100.c, config.h, keyboards/epomaker/linker/wireless/module.c, lowpower.c, wireless.c, via/galaxy100_via.json, docs/triage/2026101801-ledger.md). The previous run "Triage and fix Galaxy100 run leftovers" shipped version 2026101801 (CI run 39130103928 at commit 3f9c298 on branch fix/2026101801, both via and default keymaps compiled by GitHub Actions, never flashed or board-tested, no tag, no release). It closed as partial: 26 tasks ok, 0 failed, 53 problems recorded on the run itself, and the ledger docs/triage/2026101801-ledger.md still has OPEN rows. Fixed in 2026101801 (leave as recorded, do not rework): P04, P11, P12, P13, P14, P23, P26, P27, P32, P33, P38.

SCOPE (settled): the leftover population is BOTH (a) all 53 problems recorded on the previous run AND (b) every ledger row still OPEN in docs/triage/2026101801-ledger.md: firmware rows P15, P16, P17, P19, P22, P24, P25, P28, P29, P31, P34, P39, P41 (previously left as needs-hardware-test or cosmetic); CI-only rows P07, P10, P45; owner-action rows P43, P44; and the unitemised rows P46 through P57 and any beyond P57 (the changelog excerpt was truncated, so read the full ledger and CHANGELOG.md to enumerate them). Fix EVERY OPEN ledger row regardless of category: cosmetic and code-quality rows are fixed too, not left OPEN on the grounds that CONTEXT.md's "correctness or power bug" definition excludes them. Exceptions: P43 and P44 are owner actions outside the repository; carry them forward as OPEN (owner action pending) and list them under "left open" in the changelog entry. For P46-P57+ (no record, counted by an earlier run, never itemised): rescan the code once more against the CONTEXT.md definition of a correctness or power bug (wrong keyboard behaviour: lost or stuck keys, memory corruption, corrupted wireless traffic, wrong lock or indicator state, unfair randomness; or battery waste: lights or radio active during sleep, missed deep sleep); close only those rows still unmatched, and itemise any distinct defect found as a NEW OPEN row in the ledger (which then falls under the fix-everything rule).

DELIVERABLES AND ORDER:
1. First task: write the intent/contract in DESIGN.md (currently a seed with the goal line only; the contract slice 1 of the previous run was supposed to write is absent). Base it on CONTEXT.md's bug definition and INTEGRATION.md's rules (build path, ledger rules, version/storage-layout rules). Use CONTEXT.md's canonical term "correctness or power bug"; avoid the synonyms real bug, serious problem, important issue.
2. Local toolchain: install the QMK toolchain locally (qmk CLI + arm-none-eabi-gcc + make; none is installed as of 2026-10-18) and compile BEFORE EVERY PUSH. The qmk_firmware checkout side is ready: E:\Users\Davide\Documents\1-Projects\Epomaker\qmk_firmware\keyboards\epomaker has the galaxy100 and linker links. Compile both the via and default keymaps. If a compile fails or could not be run, say so with the output; never claim a passing compile that did not happen.
3. Baseline board test: flash the existing 2026101801 CI artifact (CI run 39130103928, commit 3f9c298) to the Galaxy100 board FIRST, as a baseline, so the 11 recorded fixes get a board test; record the results per row. The owner can flash CI artifacts and report results when a task asks; tasks that need board results must ask explicitly and record the reported outcome. Leave 2026101801 as recorded: do not reopen or edit its changelog entry or ledger beyond noting baseline test results.
4. New version 2026101802 on branch fix/2026101802: bump VIA_FIRMWARE_VERSION in keyboards/epomaker/galaxy100/config.h AND the hard-coded firmwareVersion in via/galaxy100_via.json together (version scheme YYYYMMDDNN, shown to VIA as id_firmware_version). Create its own ledger (docs/triage/2026101802-ledger.md, same format: single list of work; row ids Pnn = problem (G100-0nn), Tnn = unfinished task of previous run; columns id | source file | description | category | sev | prior status | record-complete | note for next task; FIX marker in the note column; classification sections power/sleep, wireless transport, keyboard/indicator/Custom Paint; wireless fix order P12, P13, P11 where relevant) and its own CHANGELOG.md entry. Per-fix rules from INTEGRATION.md still apply: change only files in the row's classification section; update the ledger row to FIXED (2026101802) or OPEN with reason; do not edit CHANGELOG.md directly from fix tasks, the last task assembles it; fix tasks do not touch the version number (the bump is its own task); if no local toolchain, say so rather than claim a passing compile.
5. Persisted settings / EEPROM: the previous rule "no fix adds a persisted setting" is relaxed. A fix may add a persisted setting or change the EEPROM layout only when it genuinely needs it; if any fix does, the changelog heading becomes "## 2026101802 — storage layout changed", the entry gets the Esc-hold flash note, and that row must be board-tested before the run closes. If no fix needs it, the heading stays plain "## 2026101802".
6. CI: the workflow .github/workflows/build.yml triggers on fix/** pushes and compiles via and default keymaps; every later commit needs its own CI run before it is called compiled. Push to fix/2026101802 only after a successful local compile.
7. Board test of 2026101802: once the fixes compile locally and pass CI on fix/2026101802, the owner flashes the CI artifact and reports; record per-row board results in the ledger (rows previously left as needs-hardware-test now get their test).
8. Final assembly: last task assembles the CHANGELOG.md entry for 2026101802 from the ledger (fixed rows, left-open rows with reasons including P43/P44 as owner action pending, closed unitemised rows with the rescan result, and the board-test status replacing the old "Not compiled locally (CI only) and not tested on the board" note), and updates INTEGRATION.md for the new build path (local compile + CI) and version.
9. End state (settled): after local compile, CI pass on fix/2026101802, and board test, merge fix/2026101802 into main and create tag v2026101802 so GitHub Actions publishes the release (v* tags trigger release; this is the one publishing step and happens only after the board test is reported good).

STANDING PREFERENCES: owner language en, autonomy just-do-it (do not re-ask settled decisions), working hours 09:00-18:00. Use brain_query / project_code_map for structure questions; open the filesystem only for files being edited. Report outcomes faithfully: failed compiles, failed CI, failed board tests, or skipped steps are stated plainly with output.

## Interview

1. **Q:** What counts as a "leftover" for this run? The previous run (2026101801) closed with 26 tasks ok but 53 problems recorded on the run itself, and the ledger still has OPEN rows (P15-P41 firmware, P07/P10/P45 CI-only, P43/P44 owner actions, P46-P57+ unitemised). Which population should this run triage?
   **A:** Both: the 53 problems recorded on the previous run AND every ledger row still OPEN in docs/triage/2026101801-ledger.md

2. **Q:** Many OPEN firmware rows are marked "needs hardware test". Do you have the Galaxy100 board available and can you flash and test builds during this run, or does this run stay CI-only with nothing flashed?
   **A:** Yes: I can flash CI artifacts and report results when a task asks me to

3. **Q:** The local machine has no qmk CLI, arm-none-eabi-gcc or make. Should this run install the QMK toolchain locally so changes can be compiled before pushing, or keep the GitHub Actions build path on a fix/** branch as the only compile check?
   **A:** Install the QMK toolchain locally (qmk CLI + arm-none-eabi-gcc + make) and compile before every push

4. **Q:** You can flash CI artifacts now, but 2026101801 (CI run 39130103928 at commit 3f9c298) has never been on the board. Should this run first flash that artifact so the 11 recorded fixes get a board test, and then put the new fixes into a separate version 2026101802 (bump config.h and galaxy100_via.js…
   **A:** Flash the 2026101801 artifact first as a baseline test, then start 2026101802 on branch fix/2026101802 with its own ledger and changelog entry; leave 2026101801 as recorded

5. **Q:** The previous run's rule was "no fix adds a persisted setting" because a storage-layout change forces the "storage layout changed" heading and an Esc-hold flash that nobody could verify. Now that you can flash and test, should a fix be allowed to add a persisted setting or change the EEPROM layout…
   **A:** Allow it only when a fix genuinely needs it: mark the entry "storage layout changed", add the Esc-hold flash note, and treat it as a row that must be board-tested before the run closes

6. **Q:** With the OPEN ledger rows in scope, several of them (P15-P41) were left as "cosmetic" or code-quality. CONTEXT.md defines a "correctness or power bug" and excludes code-quality findings. Should this run fix only the OPEN rows that meet that definition and leave the cosmetic/code-quality rows OPEN…
   **A:** Fix every OPEN ledger row regardless of category

7. **Q:** Ledger rows P46 through P57 (and possibly beyond; the changelog excerpt is truncated) have no record: counted by an earlier run but never itemised, and a rescan found no distinct defect. Now that OPEN ledger rows are in scope, how should this run close them?
   **A:** Rescan the code once more against the CONTEXT.md definition and only close those still unmatched; itemise any distinct defect found as a new OPEN row

8. **Q:** Ledger rows P43 and P44 are recorded as owner actions outside the repository. Since OPEN ledger rows are in scope, have you completed them, or should this run carry them forward as OPEN?
   **A:** Carry them forward as OPEN (owner action pending) and list them under "left open" in the changelog entry

9. **Q:** DESIGN.md is still a seed (goal line only; the intent/contract that slice 1 of the previous run was supposed to write is absent). Should this run write that contract as part of its first task, or leave DESIGN.md as is?
   **A:** Write the intent/contract in DESIGN.md as the first task, using CONTEXT.md's bug definition and INTEGRATION.md's rules as its basis

10. **Q:** End state of the run: once the 2026101802 fixes compile locally, pass CI on branch fix/2026101802 and you have board-tested them, how far should the run take the branch? INTEGRATION.md says a v* tag triggers a GitHub release, so this is a publishing step.
   **A:** Merge into main and create tag v2026101802 so GitHub Actions publishes the release

## Glossary terms

- leftover
- ledger
- board-tested
- leftover
- baseline flash

## Decision records

- docs\adr\0001-each-fix-run-ships-as-its-own-version-with-a-frozen-predeces.md
