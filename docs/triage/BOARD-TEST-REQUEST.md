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

Failing hop: 2 (channel not paired). Not fixable from the repo; the owner must pair Telegram.
This file is the fallback channel: it lives in the repo, so the request is visible without Telegram.

## What to do
1. Back switch on USB; Fn+Right Shift+Esc to enter the bootloader.
2. Run `exports/runs/55f165e4-1ddb-49ff-a8f5-872ecb6202ee/baseline-2026101801/flash.bat` (backs up current firmware, flashes the 2026101801 baseline), test, then flash v2026101802 (release asset or CI artifact).
3. P34/P41 changed the storage layout: coming from an older build, do an Esc-hold flash / settings reset.
4. Report pass/fail per FIXED firmware row. Receipt = your reply; until then every row reads "no report".

Delivery receipt: NOT received. Dry-run to a flashing target was not done; nothing was flashed.
