**Board-tested 2026-10-21** for the storage-layout rows: the owner flashed this binary over 2026101301 with an Esc-hold flash. Pass: VIA settings kept across a power cycle (P34), hue preset position kept across a power cycle (P41), all VIA custom menus load. Not tested: the Fn+Right Shift+Esc migration from 2026101801, and the other fixed rows one by one.

The tag was merged (15d42b3) and published before that test; this is recorded as an unapproved deviation in CHANGELOG.md and docs/triage/2026101802-ledger.md.

Rows P34/P41 change the storage layout (additive): coming from 2026101801, Fn+Right Shift+Esc keeps settings; coming from an older build, do an Esc-hold flash (full settings reset).

See CHANGELOG.md and docs/triage/2026101802-ledger.md.

**Full Changelog**: https://github.com/VanheXen/galaxy100-qmk/compare/v2026101301...v2026101802
