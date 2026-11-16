# KSM 2.2.0 migration investigation

Local research only. No production integration or remote push.

- Upstream base: `41bfdaa7331808aff7239a9ec283cdba01b3b406` (2.1.0).
- Target: `63d6546808fe3afad4469f3e4d12ca85145650d8` (2.2.0).
- Current mod: `7533cf48fd73a73976294fa2d9294477eb35345b`.
- 94 upstream commits, 131 changed files, 67 overlap files, 29 merge conflicts.
- Trial merge was aborted after preserving conflict evidence. This branch does not contain a complete migrated game.
- Original 2.2.0 Release build and 48 Catch2 test cases / 6,884 JUnit checks passed.
- Research audio library candidate built; all 11 mock-ASIO scenarios passed.
- Research NocoUI/skin-loader candidate built.
- Existing 0.5.4 installer rejects official 2.2.0 as expected.
- Full mod gameplay, device timing, performance, UI and installer migration remain unverified.

The complete Korean report, 94-commit index and local evidence are preserved at:

`F:\ksh_vertical_project\research\ksm-2.2.0-migration-20261117\REPORT_KO.md`

Dependency candidates live in separate local worktrees (not production gitlinks):

- ksmaudio: `b4599f141706df54cdde841b2e5437cd26cf7aee`
- NocoUI: `0bdf1796f75ce3a7ca2b92d74ff3ce0adb26404e`

Continue in isolated branches: dependencies → selection/cache/paths → judgment/APS/EX → rendering → UI/skins → installer contracts → full QA.

Do not overwrite production/play-test/release artifacts. Do not push until separately requested.

Audio safety: no further game launches using real output devices after the user reported unexpected loud audio. Do not change system volume, default device or user calibration. Any future runtime test requires an isolated silent output path.
