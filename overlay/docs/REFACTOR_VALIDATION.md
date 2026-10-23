# Refactor validation record

Branch: `refactor/upstream-style-parity`.
Mod base: `bcb8c095158466ae9f710185528610746657ef9e`.
Upstream style/base: `41bfdaa7331808aff7239a9ec283cdba01b3b406` (KSM 2.1.0).

## Outcome and source preservation

The final functional, exact-appearance, normal-runtime and performance gates passed. Source-only synchronization was applied to `F:/ksh_vertical_project/ksm-enhanced-vertical` after those gates, with 272 file hashes checked. The destination then built a normal Release executable and passed four title/chart-CLI startup/shutdown cases. No main/public-fork merge or release was performed.

Original source backup: `build/refactor-source-backups/20261023-234818/manifest.json` in the development tree. Its manifest retains original bytes/hashes and identifies newly added paths. Runtime assets, authoring inputs, user settings/records/skins, play-test and existing releases were excluded. All 299 original managed runtime resources are still byte-identical; no checkout-line-ending allowance was needed for this destination check.

The audit covers all mod source and upstream connection points. The enforced formatting scope is 207 C++ files; untouched upstream/vendor implementation was retained. Responsibility extraction, names and header/implementation splits follow the corresponding upstream area. `REMOVAL_AUDIT.md` records the few proven redundant includes and pure unused locals. The Windows frame-pacing correction is a separate performance fix, documented in `FRAME_PACING_FIX.md`.

## Verification matrix

| Gate | Result / evidence |
| --- | --- |
| Windows Catch2 Release and Debug | Each 43 cases / 3,982 assertions passed |
| Standalone production rolling window | 4 cases / 40,013 assertions passed |
| Actual Win32 timer, headless | 6 cases / 16 assertions passed; same cases also run in SDK suite |
| Source-package / harness Python tests | 21 passed; package hashes verified before and after tests |
| Full functional suite | All 27 required stages have actual PASS reports in `build/refactor-paced-functional-verification.json` |
| Executable EX / APS / judge-setting checks | 125 / 142 / 1,617 passed |
| APS screenshots | 81 pixel-identical RGBA comparisons, no exclusions |
| Judge-setting screenshots | 110 identical outside the exact runtime FPS-label rectangle |
| Judge-setting input replays | Both 2,197-row traces identical |
| Fresh source reconstruction | Patch, overlay and dependency patches restored at 37312a1; all 367 current C++ source files match after the documented CRLF/LF checkout transformation |
| Fresh ordinary Release | `20261023-231723-1b61bf64`, diagnostics disabled; four normal startup/CLI/shutdown cases passed |
| Fresh ordinary Debug | `20261023-232024-a8a7d0d2`, diagnostics disabled; four normal startup/CLI/shutdown cases passed |
| Final normal payload | Install/run/verify/remove passed; all 346 original/user sentinel files restored or preserved |
| Installer/environment isolation | 54 checks passed |
| Native normal-play frame performance | 12 captures / 27,330 frames; both orientations passed |
| APS diagnostic frame performance | 180 measured subruns / six scenes; all passed |
| Destination source/build | 272 source files backed up/applied/hashed; normal Release `20261023-234835-524ce9c3` and four runtime cases passed |
| Destination runtime resources | 299 byte-identical files |
| GitHub Actions | Both source-and-headless and windows-timer passed at 5a4811c, run 39968751962; final documentation/package verification is tracked at branch head |

The functional reports cover 640x480, 1280x720, 1920x1080, 720x1280 and 1080x1920; five supported languages; default/complete/partial/broken skins; EX records; APS; viewport geometry and preview speed 450; timing choices; menus/save/cancel/BACK; and audio integration/EOF. Render fixtures, actual GameMain input replays and normal executable startup/CLI tests are distinguished. A screenshot fixture is not treated as proof of human controller input.

## Final performance protocol

A regression requires an increase exceeding **both 5% and 0.1ms**. That gate was retained throughout. No failed result was relabeled as a pass.

Normal executable: actual original chart-CLI autoplay through GameMain, no diagnostic flags; three repetitions per build/orientation. PresentMon 2.6.0 was verified against its official release digest and used under the separately approved elevation, filtering exact owned test PIDs. Each capture has 3 seconds warmup and 8 seconds measurement. These are DXGI submission intervals, not physical display latency. No driver/service/settings installation was performed.

Baseline / candidate values in milliseconds:

| Native resolution | Mean | p99 | Gate |
| --- | --- | --- | --- |
| 1280x720 | 3.4443 / 3.4420 | 7.0581 / 6.0563 | PASS |
| 720x1280 | 3.4372 / 3.4369 | 6.7748 / 5.9464 | PASS |

Native evidence: `build/normal-frame-performance/20261023-232402/` (all CSVs, 12 captures, comparison).

Diagnostic APS: the pre-existing long protocol uses three declared batches, five 960-frame subruns per scene/build, alternating build/scene order and warmup. Each batch uses the arithmetic mean of the five means and the median of five measured p99 values; the latter is explicitly **not** a pooled-frame p99. Three-batch medians are compared with the same gate. Process-memory sampling is disabled and console output buffered during timing. Raw subruns remain available.

| Diagnostic scene | Mean baseline / candidate | p99 baseline / candidate | Gate |
| --- | --- | --- | --- |
| landscape / hidden | 3.3471 / 3.3328 | 6.6230 / 5.7530 | PASS |
| landscape / idle | 3.3359 / 3.3355 | 6.5460 / 6.0720 | PASS |
| landscape / overlap | 3.3427 / 3.3467 | 6.4910 / 5.9000 | PASS |
| portrait / hidden | 3.3336 / 3.3393 | 6.5610 / 5.7320 | PASS |
| portrait / idle | 3.3413 / 3.3338 | 6.5340 / 6.0260 | PASS |
| portrait / overlap | 3.3583 / 3.3466 | 6.5630 / 5.9000 | PASS |

Long-protocol evidence: `build/refactor-performance-batched/20261023-232755/`. All 180 measured subruns completed. The earlier one-subrun/three-direct-repetition result is retained as FAIL: portrait-hidden p99 5.074 -> 5.818ms (+14.66%), despite its essentially unchanged mean. It was not counted as passing. Because that short diagnostic result conflicted with passing native-play captures and varied baseline tails, the already-existing longer protocol was run once as an explicit confirmatory measurement. Its threshold and aggregation were declared before collection; all six scenes passed. Earlier 180-subrun and native-play failures from before the timer correction also remain retained.

Separate three-run memory observations before the timer correction showed approximately 1% peak-private-memory difference. The final correction adds one lazily owned timer handle per pacing thread, no per-frame allocation, and native ownership/fallback tests. Those observations are not a long-duration or physical-ASIO memory-leak certification.

## Retained failures and corrections

See `BASELINE_TEST_FINDINGS.md` for independent derivations/reproductions and original failed artifacts.

- Generated SDK unit target initially omitted upstream Resource.rc, causing renderer initialization failure. Including the original resource script fixed the test setup; failed setup is not counted as a passed game test.
- Original randomization fixture lacked required BPM; signed-area fixture had an incorrect symmetry expectation. Game calculations were retained and independently derived expectations used.
- External authoring piano chart differs by one existing FX-B event. Runtime matches pinned bcb8c09. Original resources were preserved, not edited to make the authoring assertion pass.
- Reconstructed checkout has 28 known CRLF/LF-only text-resource differences; binary assets are exact. Original raw hashes remain. Destination assets were not copied and all 299 remain byte-identical.
- Original executable reproduced a preview sample-window assertion under a forced, finite owned-worker hitch. The harness now bounds the prior sample by the observed gap while retaining the actual 18s wrap and all other drift/reset checks. Six rejection/acceptance tests cover it.
- One asynchronous result PNG was lost after a multi-second frame stall. Its failure remains; the unchanged resolution-specific sweep passed subsequently.
- Fixture/live EX capture modes previously reused a runtime folder, allowing missed samples to read preceding sentinel JSON/PNGs. Failed outputs were archived. Separate owned runtimes now make missing captures fail and the original live-score assertions pass. HUD/score calculations were unchanged.
- A Debug smoke expectation initially omitted the existing Siv3D debug-title suffix. The corrected harness checks the original title plus that suffix.
- Payload sentinel verification initially mixed slash/backslash keys. Relative-key normalization corrected the harness; all 346 hashes pass.
- First Windows CI timer job used a slash path that cmd parsed as a command. Backslash launcher correction passed; the failed email describes the earlier run.
- Destination incremental linking emitted LNK4020 for cached NocoUI debugger type records. The executable passed all four normal smokes. Fresh restored Release/Debug builds passed without this cached-symbol warning. Existing baseline build artifacts were preserved; no broad clean/delete was performed.

## Environment limits

Windows x64 / Siv3D 0.6.16 / D3D11 was exercised locally. Mock ASIO tests cover 44.1/48kHz, failure/fallback, concurrent resource lifetime, real-chart EOF and shutdown. A physical ASIO device and other operating systems were not certified. CI verifies reconstruction, hashes, style, source/harness tests and headless production logic; it does not replace local GPU/audio tests. Normal-gameplay results do not certify physical input-to-display/audio latency.
