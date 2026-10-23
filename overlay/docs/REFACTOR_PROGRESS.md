# Upstream-style refactoring

Base mod commit: bcb8c095158466ae9f710185528610746657ef9e.
Upstream style/behavior base: KSM 2.1.0, 41bfdaa7331808aff7239a9ec283cdba01b3b406.
Branch: refactor/upstream-style-parity. Main and the public fork are unchanged.

## Completed baseline gates

- Captured the production source hashes, screenshots and existing functional reports.
- Measured APS frame-time cases three times using the unchanged production binary.
- Recovered the upstream Catch2 v2.13.6 tests on Windows and added real production-logic tests for EX, NORMAL gauge, APS presentation, input statistics and timing display boundaries.
- Original test fixture repairs: provide a required BPM in the S-RANDOM fixture; use the independent signed-area calculation for symmetric scroll-speed zero crossing. The game implementation is unchanged by these fixture repairs.
- Test runner includes the original engine resource script and parses JUnit errors/failures. A renderer startup error during test-target setup was diagnosed and fixed; failed setup runs are not counted as passing tests.

## Pending gates

Final performance comparison, final-payload installation/removal preservation, final package export/CI and source-only production synchronization. See REFACTOR_VALIDATION.md for the completed gates and retained failures.

## Style gate

Formatted 168 mod/test C++ files using clang-format 19.1.5 and the upstream tab/BOM/bracing conventions. Every file retained exactly the same C++ tokens (including literals and operation order). The complete upstream-plus-mod Catch2 suite passed after formatting: 36 cases, 3950 assertions.

## Responsibility extraction gate

- Moved JudgeSettingOverlay layout, navigation, persistence and drawing implementations into its registered `.cpp` file. The public surface, coordinates and event order remain unchanged.
- Moved JudgeSettingSession measurement/state transitions and NormalGaugeState updates into registered `.cpp` files. GaugePresentation was reviewed separately; its small hot math remains inline.
- Extracted AdditionalScoreIO from KscIO. Existing 10-field and new 11-field supplemental records, atomic replacement and shared-score rules are retained. Main KSC serialization remains in KscIO.
- RecentTimingWindow owns the incremental time queue/sum and is used by the real setting session. Its standalone Catch2 target passes 4 cases / 40,013 assertions, including comparison against independent filtering over 20,000 frames.
- DiagnosticRunner separates three startup phases without moving the dispatch points or changing cleanup order. Heavy test implementations no longer sit in Main.cpp.
- Renamed skin-cache and path helpers to describe their actual roles; kept resource keys, config values, caches and fallback behavior.
- Release builds and upstream-plus-mod units passed (3,966 assertions); actual-executable EX 125, APS 142, judge-setting 1,617 and menu-flow checks passed after the moves. Broader skin/render/preview checks are in progress.
- Added a JUnit-aware CMake wrapper because a `void Main()` engine entry point cannot propagate Catch2's assertion status as a process return value.

These are stage results, not completion of the full plan. The original development tree and play-test executable have not been synchronized.

## Diagnostic and compatibility gates

- GitHub Actions run 39935851846 passed: package hashes, malformed-package checks, clean source/dependency reconstruction, C++ style and the standalone production rolling-window tests.
- A saved original-binary timing-setting baseline was compared with the candidate: all 110 screenshots were pixel-identical outside the exact runtime FPS label rectangle. Both 2,197-row input replays were identical. No general pixel tolerance was used.
- Release audio engine and integration tests passed, including failure/fallback, resource churn and concurrent transport. Real-chart EOF cases passed in both orientations and 44.1/48 kHz mock ASIO modes. This does not certify a physical ASIO device.
- The upstream-plus-mod Debug unit target passed 37 cases / 3,966 assertions before the later token-preserving lambda style pass.
- General builds default to KSM_ENABLE_DIAGNOSTICS=0; opt-in fixture builds use `scripts/Build.ps1 -Diagnostics`. Original TestPlayArgs parsing is unchanged. Heavyweight startup fixtures are compiled only for diagnostic builds. The final diagnostic argument wrapper returns the original SDK const reference; original test-play parsing remains separate.
- Moved 21 complex ModOptionsScene implementations out of its header while retaining small inline helpers and the original member initializer order. Added the implementation to both project and filter files.
- Expanded compressed lambda bodies and formatted all mod/test/dependency C++ in the audited scope. Automatic using-declaration sorting is disabled so a formatting pass cannot reorder C++ tokens.

## Full functional gate

All 27 required functional validation steps passed across the retained initial and resumed reports. Windows Release and Debug each passed 37 Catch2 cases / 3,966 assertions. Standalone production rolling-window tests passed 4 cases / 40,013 assertions. Five resolutions, five languages, skin fallback/switching, viewport rendering, title/select/result/in-game EX, menu navigation, save/cancel and audio integration/EOF were exercised.

The original external-authoring assertion failure, exact-resource checkout newline failure and missing local song-input preparation failure remain in the run history. They were investigated rather than relabeled as passing: runtime chart matches pinned bcb8c09; raw resource hashes are retained with explicit text-only checkout normalization; missing local song inputs are copied without overwriting existing fixtures. The original development runtime is unchanged.

81 APS screenshots are pixel-identical to the preserved original; 110 timing-setting screenshots and both input replays also match (only the runtime FPS label is excluded in the latter comparison).

Final performance verification is still pending. Short capped runs have unstable p99 values even on the preserved binary; all failures are retained. A by-value argument-wrapper experiment was discarded because the SDK actually returns a const reference. The final diagnostic wrapper delegates that reference directly. Small hot GaugePresentation calculations/accessors are retained inline, matching the original structure and preserving arithmetic. Main now reports actual collected benchmark samples so an interrupted capture cannot appear complete.

Fresh patch/overlay reconstruction also passed normal Release and Debug builds, with diagnostics disabled. Both configurations passed title and original chart-CLI startup/shutdown in both orientations (four cases each). The normal Debug title expectation includes the existing Siv3D suffix.

## Isolated frame-pacing correction

The high-resolution Windows wait prototype improved native-play p99 in both orientations. The production correction is separated from style/responsibility changes and documented in FRAME_PACING_FIX.md. Deadline/FPS/drift math is retained; unsupported/failed native timers fall back to the previous sleep path. Six actual Win32 cases / 16 assertions passed headlessly; the combined Release and Debug suites now pass 43 cases / 3,982 assertions. Full functional and final-binary performance checks are running before any production source synchronization.

## Latest checkpoint: CI fixed, production still preserved

Windows workflow launcher correction passed both jobs in Actions run 39964915296. The integrated timer build passes all 27 functional stages, 43 Catch2 cases / 3,982 assertions in each Windows configuration, 81 exact APS images, 110 judge-setting images (exact FPS-label exclusion) and both 2,197-row replays. The final EX capture harness now separates sentinel fixtures and live runtime files, preserving the earlier failures.

The three-direct-repetition diagnostic performance comparison still fails portrait-hidden p99 (5.074 -> 5.818 ms), although the other five scenes improve. No final completion or production-source synchronization is declared. A fresh reconstruction's ordinary Release/Debug builds and normal native-play capture are being checked next.
