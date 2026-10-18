# CONTEXT — Ubiquitous Language

<!-- Maintained by Power Mode phase-0. Glossary of canonical project terms only —
     no implementation detail. Each term: a definition and the synonyms to avoid. -->

## Glossary

### correctness or power bug
A recorded problem that makes the keyboard behave wrongly (lost or stuck keys, memory corruption, corrupted wireless traffic, wrong lock or indicator state, unfair randomness) or waste battery (lights or radio active during sleep, missed deep sleep). Code-quality findings such as dead code, namin…

_Avoid:_ real bug, serious problem, important issue

### leftover
A problem still unresolved after the previous run: either one of the 53 problems recorded on run 2026101801 or any ledger row still OPEN in that run's ledger. Every leftover must end this run as FIXED, OPEN with a stated reason, or closed by a recorded rescan.

_Avoid:_ remaining issue, open bug, residual

### ledger
The single list of all triage work for one firmware version, where every problem or unfinished task has one row with its status and the reason for that status.

_Avoid:_ task list, bug list, tracker

### board-tested
A firmware build whose CI artifact was flashed onto the Galaxy100 and whose behaviour was checked and reported by the owner. A green CI run alone is compiled, not board-tested.

_Avoid:_ tested, verified, works on hardware, validated

### baseline flash
Flashing the already-built 2026101801 artifact to the board before any new change, so the eleven fixes recorded in that version get their first hardware test and their results are written into the new ledger.

_Avoid:_ regression flash, sanity flash
