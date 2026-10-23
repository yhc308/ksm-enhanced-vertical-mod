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

Dependency style/lifecycle audit, full Debug/Release validation, complete regression/replay and visual comparisons, performance comparison, clean source reconstruction, final production synchronization.

## Style gate

Formatted 168 mod/test C++ files using clang-format 19.1.5 and the upstream tab/BOM/bracing conventions. Every file retained exactly the same C++ tokens (including literals and operation order). The complete upstream-plus-mod Catch2 suite passed after formatting: 36 cases, 3950 assertions.

## Responsibility extraction gate

- Moved JudgeSettingOverlay layout, navigation, persistence and drawing implementations into its registered `.cpp` file. The public surface, coordinates and event order remain unchanged.
- Moved JudgeSettingSession measurement/state transitions, GaugePresentation interpolation and NormalGaugeState updates into registered `.cpp` files.
- Extracted AdditionalScoreIO from KscIO. Existing 10-field and new 11-field supplemental records, atomic replacement and shared-score rules are retained. Main KSC serialization remains in KscIO.
- RecentTimingWindow owns the incremental time queue/sum and is used by the real setting session. Its standalone Catch2 target passes 4 cases / 40,013 assertions, including comparison against independent filtering over 20,000 frames.
- DiagnosticRunner separates three startup phases without moving the dispatch points or changing cleanup order. Heavy test implementations no longer sit in Main.cpp.
- Renamed skin-cache and path helpers to describe their actual roles; kept resource keys, config values, caches and fallback behavior.
- Release builds and upstream-plus-mod units passed (3,966 assertions); actual-executable EX 125, APS 142, judge-setting 1,617 and menu-flow checks passed after the moves. Broader skin/render/preview checks are in progress.
- Added a JUnit-aware CMake wrapper because a `void Main()` engine entry point cannot propagate Catch2's assertion status as a process return value.

These are stage results, not completion of the full plan. The original development tree and play-test executable have not been synchronized.
