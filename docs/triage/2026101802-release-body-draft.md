**NOT BOARD-TESTED.** This binary is CI-compiled only; it was never flashed.

The settled end state for 2026101802 made a board test a precondition for merging and tagging. v2026101802 was merged to main (15d42b3) and tagged without one. The owner has not approved this as a deliberate deviation; it is recorded as an unapproved deviation in CHANGELOG.md, INTEGRATION.md and docs/triage/2026101802-ledger.md.

Rows P34/P41 change the storage layout (additive): coming from an older build, do an Esc-hold flash / settings reset. No board results are recorded, and delivery of the board-test request is unconfirmed (Telegram not paired).

See CHANGELOG.md and docs/triage/2026101802-ledger.md.

**Full Changelog**: https://github.com/VanheXen/galaxy100-qmk/compare/v2026101301...v2026101802

<!-- Apply (needs `gh auth login`):
gh release edit v2026101802 --prerelease --notes-file docs/triage/2026101802-release-body-draft.md
-->
