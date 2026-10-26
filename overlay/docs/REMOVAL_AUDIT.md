# Removed code and dependency audit

Only the following proven redundancies were removed in this pass. No resources, compatibility parsers, fallback branches or diagnostic entry points were deleted.

| Location | Removed item | Evidence |
| --- | --- | --- |
| Main.cpp | Duplicate OptionScene, SelectScene and ResultScene includes | Identical paths already included earlier; headers use guards. Scene registrations and implementations remain. |
| Common/SkinPreset.cpp | `<filesystem>` include | No std::filesystem names are used. File operations remain on the existing Siv3D APIs and the retained `<cstdio>` path. |
| JudgeSettingOverlay::controlsRect | Unused `title` local | Pure position calculation; neither the result nor any side effect is consumed. The control rectangle expression is unchanged. |
| JudgeSettingOverlay::draw | Unused `frameUnit` local | Pure width/height arithmetic; all title, banner and HUD drawing expressions are unchanged. |
| JudgeSettingSession | Inline sample queue/sum implementation | Replaced by RecentTimingWindow with the same append, ordered expiration and sum operation order; checked against an independent 20,000-frame history. |
| Main.cpp / overlay headers | Moved implementations | These are relocations into registered `.cpp` files, not deletions. Initialization stages and cleanup order are retained. |

TimingMenuCursor retains its unused delta argument for call-site compatibility; its unnamed parameter documents that current navigation already applies the direction before calling it. Modified external-library code is audited separately; vendor headers and license notices remain intact.

GaugePresentation.cpp was consolidated back into its small header after the frame-path review. Its calculation and fixture accessors remain present and tested; no gauge behavior was deleted. The hot interpolation calculator keeps the original inline structure, with explicit unit names.

The final Debug warning audit also removed two unused pure coordinate locals (`x`/`w`) from SkinOverlay mouse-hit processing and an unused copied `initial` hispeed fixture value. Existing hit rectangles, Unit evaluation, input replay calls and hispeed checks remain. The hispeed accessor only returns the current setting and has no mutation.
