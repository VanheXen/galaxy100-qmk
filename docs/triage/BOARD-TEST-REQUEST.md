# Board-test request BT-2026101802-01

Target board: Epomaker Galaxy100 (VID_342D PID_E489), firmware v2026101802 (tag on main 15d42b3).

## Hops (requester to owner)
1. Run task calls `notify-owner` (host notification channel, Telegram).
2. Channel delivers to the paired owner chat. **Stops here: Telegram is not paired.**
3. Owner flashes and reports pass/fail per ledger row in `docs/triage/2026101802-ledger.md`.

## Dispatch log
| time (2026-10-18) | request id | target | result |
|---|---|---|---|
| earlier runs | BT-2026101802-01 | Galaxy100 | delivered=false, queued=true, reason=telegram-not-paired |
| this task | BT-2026101802-01 | Galaxy100 | delivered=false, queued=true, reason=telegram-not-paired |
| remedy task (2026-10-18) | BT-2026101802-01 | Galaxy100 | delivered=false, queued=true, reason=telegram-not-paired; fallback pointer added to README.md |

Failing hop: 2 (channel not paired). Not fixable from the repo; the owner must pair Telegram.
This file is the fallback channel: it lives in the repo, so the request is visible without Telegram.

## What to do
1. Back switch on USB; Fn+Right Shift+Esc to enter the bootloader.
2. Run `exports/runs/55f165e4-1ddb-49ff-a8f5-872ecb6202ee/baseline-2026101801/flash.bat` (backs up current firmware, flashes the 2026101801 baseline), test, then flash v2026101802 (release asset or CI artifact).
3. P34/P41 changed the storage layout: coming from an older build, do an Esc-hold flash / settings reset.
4. Report pass/fail per FIXED firmware row. Receipt = your reply; until then every row reads "no report".

**Result (2026-10-21):** owner flashed v2026101802 (Esc-hold, from 2026101301) and reported P34, P41 and the VIA menus as pass. Recorded in the ledger. Request closed.

Delivery receipt (2026-10-18): NOT received. Dry-run to a flashing target was not done; nothing was flashed.

## Release status of v2026101802 (checked 2026-10-18)

- GitHub release v2026101802 (tag target main, merge 15d42b3) now reads `prerelease:true` via the public API and its body starts "NOT BOARD-TESTED". It was marked by `.github/workflows/mark-prerelease.yml` (commit d2d2f64, Actions run 39152442361), because `gh` is not authenticated on this machine. Body source: `docs/triage/2026101802-release-body-draft.md`.
- The build.yml Release step gives future `v*` tags the same pre-release flag and body.
- Promote to a normal release only after the board results below are recorded as pass.
- Merging and releasing without a board test is an UNAPPROVED deviation from the settled end state; the owner must ratify or withdraw it.
- Git note: this folder is a git repo (origin VanheXen/galaxy100-qmk). Commits 0da7072 and 587d394 are in main's history and were pushed with 868ab74 (HEAD at check time).
