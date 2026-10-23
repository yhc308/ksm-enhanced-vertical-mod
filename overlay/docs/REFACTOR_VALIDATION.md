# Refactor validation record

Branch: `refactor/upstream-style-parity`.
Mod base: `bcb8c095158466ae9f710185528610746657ef9e`.
Upstream base: `41bfdaa7331808aff7239a9ec283cdba01b3b406` (KSM 2.1.0).

## Scope and preservation

Mod source, upstream connection points and modified dependency files were audited. The formatting scope contains 203 C++ files. Untouched upstream/vendor implementation and all license notices remain. `REMOVAL_AUDIT.md` lists the few deleted redundancies; moved implementations are not counted as removed functionality. No main/public-fork merge or release is part of this work.

The original development tree, play-test, settings, score records, authoring resources and releases remain the comparison baseline until the remaining gates below pass. Runtime resources are excluded from the eventual source-only synchronization so checkout newline normalization cannot rewrite the user's asset bytes.

## Recorded results

| Gate | Evidence | Result |
| --- | --- | --- |
| Windows Catch2, Release | `build/unit-tests/Release/results.xml` | 37 cases, 3,966 assertions passed |
| Windows Catch2, Debug | `build/unit-tests/Debug/results.xml` | 37 cases, 3,966 assertions passed |
| Standalone production rolling window | CMake/CTest and CI | 4 cases, 40,013 assertions passed |
| Source-package and runner tests | Private repository `tests/` | 15 Python tests passed |
| Full functional suite | `build/refactor-functional-verification.json` | All 27 required stages passed |
| EX, APS and judge setting | Actual executable fixtures | 125 / 142 / 1,617 checks passed |
| APS appearance | `build/refactor-aps-baseline-pixels.json` | 81 screenshots pixel-identical, no excluded pixels |
| Judge setting appearance | Baseline comparison report | 110 screenshots identical outside the exact runtime FPS label rectangle |
| Judge setting input replay | Baseline/candidate trace comparison | Both 2,197-row traces identical |
| Fresh reconstruction | `build/refactor-reconstructed-b7057fc-mirror` in the preserved development tree | Patch, overlay and dependency reconstruction succeeded |
| Fresh normal Release build | `build/normal-release.json` in reconstructed tree | Diagnostics disabled; build passed |
| Fresh normal Debug build | `build/normal-debug.json` in reconstructed tree | Diagnostics disabled; build passed |
| Final-payload installation/removal | Reconstructed normal Release, isolated original-game copy | Startup/shutdown passed; 346 original/user files restored or preserved |
| Installer/environment isolation | `build/environment-tests/ae947a2389ae4fdd918ed6295b08fb0b/results.json` | 54 checks passed |
| Fresh normal startup/shutdown | `build/normal-smoke-{Release,Debug}.json` in reconstructed tree | Both orientations, title and original chart CLI: four cases per configuration passed |

The full functional reports cover five display sizes (640x480, 1280x720, 1920x1080, 720x1280 and 1080x1920), five supported languages, default/complete/partial/broken skins, EX records, APS, viewport geometry, preview speed 450, timing display choices, menu/save/cancel/BACK and audio integration/EOF. Native fixture assertions and UI render fixtures are distinguished from real GameMain/input replays. A screenshot fixture alone is not presented as proof of human-input gameplay.

Normal Debug builds intentionally append the Siv3D diagnostic title suffix. The normal smoke test checks the unchanged application title plus that existing suffix, rather than requiring Release's exact full title. A failed initial smoke assertion for this suffix is retained in the run history. It was a test expectation error, not a game-title change.

## Failed setup/baseline findings retained

- The initial generated unit target omitted the upstream engine Resource.rc, causing renderer initialization failure. The target now includes the original resource script; subsequent unit and normal-game startup runs pass. The failed setup is not counted as a passed game test.
- The external authoring piano chart differs by one existing FX-B event from the pinned mod's runtime chart. Runtime bytes match the pinned mod baseline and are preserved. See `BASELINE_TEST_FINDINGS.md`.
- Of 299 runtime resources, 271 are byte-identical; 28 known text files differ only by checkout CRLF/LF. Raw hashes remain recorded. Explicit text-only normalization was used only for the checkout comparison; images/audio remain exact and no production assets were rewritten.
- Missing local ordinary-song inputs were prepared in ignored test fixtures using collision/hash checks. They are not exported or committed.
- The first new payload-smoke assertion mixed Windows backslash snapshot keys with slash lookup keys. The harness now normalizes relative keys; the repeated install/run/remove verifies all 346 original/user file hashes. The failed fixture was retained; no original/user runtime was modified.
- Short capped performance captures produced unstable p99 results on both binaries. Failed comparisons are retained. A discarded argument-wrapper experiment copied the SDK argument array; the final wrapper returns the original const reference. GaugePresentation's small hot calculations remain inline with unchanged arithmetic.

## Performance protocol and remaining gates

`RunRefactorPerformance.ps1` declares three batches, five 960-frame subruns per scene/build, with alternating build/scene order and warm-up. Every raw subrun is retained. Each batch reports the arithmetic mean of its five mean times and the median of its five measured subrun p99 values; that median is not a pooled-frame p99. The comparison uses medians across three batches and retains the original gate: a regression requires both more than 5% and more than 0.1 ms. Process-memory sampling is disabled during frame measurements, and child console output is buffered to logs.

The final 180-subrun capped APS comparison passed five of six scenes. Portrait-hidden p99 was 6.337 ms baseline versus 6.708 ms candidate (+0.371 ms, +5.85%); its mean changed by +0.0197 ms. This FAIL is retained under `build/refactor-performance-batched/20261023-212224/comparison.json`. A predeclared same-executable control and high-frame-limit control investigate scheduler/limiter effects; they do not replace this gate.

The follow-up frame-limit control retained the failure: capped candidate median p99 7.077 ms versus original-A 6.333 ms and original-B (same original EXE) 6.055 ms. At a 10,000fps limit, candidate mean/p99 were 1.2935/3.186 ms versus original-A 1.2992/3.159 ms: within the unchanged gate. This indicates a frame-limit/wakeup interaction, but does not certify normal gameplay performance. The unchanged limiter implementation uses std::this_thread::sleep_until; it has not been replaced.

External native-play DXGI capture was prepared using PresentMon 2.6.0, verified against the GitHub release SHA256. Its access probe failed with Windows ETW access denied; the user explicitly approved an elevated run. `scripts/RunNormalFramePerformance.ps1` then completed 12 native chart-CLI autoplay captures (three runs per build/orientation), filtering only exact owned test PIDs and making no driver/service/settings installation. This is actual GameMain with the ordinary executable and no fixture flags; PresentMon measures DXGI submission intervals, not physical display latency.

The native comparison also retains a FAIL: landscape mean improved from 3.4605 to 3.3286 ms, but p99 increased from 6.5990 to 7.0337 ms (+0.4347 ms, +6.59%). Portrait mean improved from 3.4251 to 3.3284 ms and p99 changed from 6.3606 to 6.4981 ms (+2.16%, within gate). Raw captures and comparison remain in `build/normal-frame-performance/20261023-215836`. No failed performance result is relabeled as a pass.

Pending: separate frame processing from sleep/wakeup cost to resolve the finding, final package export/CI and source-only production synchronization. Test-only instrumented copies of FrameRateLimit are built outside production sources; they are not shipping changes or a substitute for unmodified-binary captures. This record does not declare the refactor complete before those gates pass.

## Environment limitations

Windows x64 / Siv3D 0.6.16 / D3D11 was exercised locally. Audio tests cover mock ASIO 44.1/48 kHz transport, failure/fallback, concurrent resource lifetime and EOF. A physical ASIO device and other operating systems were not certified. CI verifies reconstruction, hashes, style, runner/package checks and headless production logic; it does not replace local GPU/audio tests.

After the final unused-local cleanup, Windows Release and Debug units again passed, and the final normal Release build `20261023-215326-e25a486d` passed the four normal startup/CLI/shutdown cases. Final installation evidence is retained under the working source build directory.

## Frame-pacing investigation

Two isolated timer probes were built from the preserved original source/project/libraries and the candidate source/project/libraries. Only the disposable copies of FrameRateLimit.cpp record work, wait and total intervals; production sources were not instrumented. Three native-play runs per binary retained all samples after a declared 300-frame warmup. Both binaries showed variable work-time tails and substantial sleep/wakeup tails. The unchanged `std::this_thread::sleep_until` pacing therefore needed investigation independently of the refactor's rendering cost; instrumented timings are not substituted for the unmodified DXGI captures.

An isolated Windows high-resolution waitable-timer prototype, retaining the original deadline, target FPS and 10ms drift calculation, was then compared against the preserved unmodified executable using the same 12 native DXGI captures. Both orientations passed the unchanged gate: landscape p99 6.6035 -> 5.8183 ms (-11.89%), portrait 6.5779 -> 6.0882 ms (-7.44%); average submission intervals remained around 3.33ms. This prototype is not a shipping or production-source change. Integrating a regression fix requires explicit source/resource-lifetime tests and repeated full validation. Earlier capped/native FAIL reports remain retained.
