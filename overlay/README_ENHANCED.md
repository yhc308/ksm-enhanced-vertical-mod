# KSM Enhanced Vertical Mod

Unofficial Windows mod for **K-Shoot MANIA 2.1.0**, based on upstream commit
`41bfdaa7331808aff7239a9ec283cdba01b3b406`. This source is maintained in the independent private mod repository.
The original K-Shoot MANIA project and its copyright notices remain intact.

## Features

- Portrait gameplay and configurable vertical display/viewport layouts.
- ASIO output and audio/input timing work.
- HARD + APS: parallel NORMAL calculation and one-time fallback without ending play.
- EX SCORE, P-CRITICAL feedback, timing-error text and color controls.
- Looping Judge Timing Setting mode, live offsets, hi-speed, and timing indicators.
- Skin/viewport previews using gameplay rendering; preview hi-speed is fixed at 450.
- English, Japanese, Korean, Simplified Chinese, and Traditional Chinese UI support.

## Reviewing the changes

Reconstruct the source using the private repository root scripts/PrepareSource.py. Compare the result with the upstream commit above. Main game changes are
ordinary source diffs. The original dependency submodule pointers are retained;
local changes to ksmaudio and NocoUI are in `patches/dependencies/` and are applied
by the script below. Generated executables, packages, local paths/reports, user
settings, credentials, scores, and ordinary/TEST song collections are excluded.

## Build (Windows x64)

1. Install Git, Python 3, Visual Studio 2022 C++ build tools, and OpenSiv3D SDK 0.6.16.
2. Clone the private mod repository and run `python scripts/PrepareSource.py`; work in the generated source directory.
3. Run `git submodule update --init --recursive`.
4. Run `python scripts/ApplyDependencyPatches.py` (safe to run again).
5. Set `SIV3D_0_6_16` to the SDK directory. Open `kshootmania.sln` and build Release/x64,
   or run `scripts/Build.ps1` in PowerShell 7 with the VS 2022 Build Tools installed.
6. Run the resulting mod executable from a prepared KSM 2.1.0 runtime directory.
   Obtain the original game and SDK runtime dependencies separately; the source
   repository does not provide a complete prebuilt game runtime.

For ASIO, obtain the official Windows x64 BASSASIO and BASSmix runtime DLLs from
Un4seen and place `bassasio.dll` and `bassmix.dll` beside the executable. Their
headers and notices accompany the dependency patches. Default audio remains usable
without an ASIO device. Original KSM settings and songs are not part of this repo.

## Test resources and attribution

The small Preview_Piano chart/audio set used by preview and judge settings is
included by the mod author's request. Track metadata credits **Hyung**. Ordinary
K-Shoot MANIA songs, TEST collections, third-party user skins, and authoring PSDs
are not added by this branch. See the upstream LICENSE and dependency notices.

A separate folder-selection installer for users who already have KSM 2.1.0 is
being prepared; `scripts/ModInstaller.ps1` currently provides the validated backend.
Do not overwrite the original `kshootmania.exe` with the mod executable.

## Current source changes

- Base images live in `imgs/`; custom skins live in `skins/<skin name>/`. The `default` menu entry refers to `imgs/` directly. Move old custom `imgs/<skin name>/` folders into `skins/`.
- Additional score rows append full-chart EX SCORE MAX after EX SCORE. Ten-field legacy additional records remain readable; the original nine-field `.ksc` format is unchanged. Older mod executables do not read the new eleven-field format.
- `scripts/RunSkinTests.ps1`, `RunExScoreTests.ps1` and `RunApsTests.ps1` provide isolated checks. Python image checks require Pillow (some also require NumPy). Tests involving original songs, SDK DLLs or authoring snapshots require those inputs separately; ordinary song collections and authoring originals are not stored here.

## Refactoring validation

The private branch `refactor/upstream-style-parity` retains the KSM 2.1.0 source base and the mod's file formats/resources. See `docs/REFACTOR_VALIDATION.md`, `REFACTOR_PROGRESS.md`, `BASELINE_TEST_FINDINGS.md`, and `REMOVAL_AUDIT.md` for stage results, known baseline discrepancies and deletion reasons.

General builds disable diagnostic fixture arguments. Use an opt-in diagnostic build for local validation:

```powershell
$env:SIV3D_0_6_16 = "C:/path/to/OpenSiv3D_0.6.16"
scripts/Build.ps1 -Configuration Release -Diagnostics
scripts/PrepareTestSongs.ps1 -SourceSongs "C:/path/to/K-Shoot MANIA/songs/K-Shoot MANIA"
scripts/ValidateRefactor.ps1 -Tier Fast -SkipBuild
scripts/ValidateRefactor.ps1 -Tier Full -SkipBuild
```

Supply the original standard songs locally for integration tests; they remain ignored by Git. BASSASIO/BASSmix runtime DLLs are obtained separately as described above. Do not point tests at a user's active runtime. The helper copies missing ordinary song inputs into an isolated snapshot and refuses conflicting files. Full validation logs each passing or failing stage under `build/validation/`; a resumed run explicitly requires the preceding report rather than treating skipped steps as passes.

Upstream-plus-mod Catch2 tests run with `scripts/BuildUnitTests.ps1 -Configuration Release` or `Debug`. The runner includes the original Siv3D engine resource script and rejects missing, empty, stale or failed JUnit reports. CMake/CTest uses the same JUnit-aware result check. The standalone `scripts/RunHeadlessTests.cmd` exercises the real incremental timing window without Siv3D. C++ style uses clang-format 19.1.5 with the audited scope in `docs/CPP_STYLE_SCOPE.json`.

UI/replay checks can run without external authoring snapshots; this is reported explicitly. Runtime resource preservation can be checked against an immutable snapshot with `VerifyResourceBaseline.py`. Its optional checkout-line-ending mode accepts only CRLF/LF differences in known text formats; image/audio bytes, paths and all other content remain exact. The known authoring/runtime last-note discrepancy is documented separately and is not changed by the refactor.

A fresh checkout is reconstructed with the private repository's `PrepareSource.py`. Source/package integrity and pure production-state tests run in GitHub Actions without uploading an SDK, user records, ordinary songs or binaries. GPU/audio-device checks run locally on Windows; physical ASIO-device validation is separate from the mock-ASIO suite.

The normal executable smoke is `tests/NormalRuntimeSmoke.ps1 -ProjectRoot <prepared source>` after a build without `-Diagnostics`. `tests/PayloadInstallationSmoke.ps1` tests the real payload against a disposable copy of an original compatible runtime. Installer preservation/error paths are covered by `tests/TestModEnvironment.ps1`. Both helpers leave the original runtime untouched. Performance scripts retain raw subruns and any failed comparisons; `RunFrameLimitControl.ps1` is a diagnostic control, not a substitute for the main performance gate.

The separate Windows frame-pacing correction and its ownership/fallback tests are described in `docs/FRAME_PACING_FIX.md`. `scripts/RunHeadlessTests.cmd` also runs the actual Win32 timer tests, and the private CI workflow includes an SDK-independent Windows timer job. Final-binary performance results remain separate from prototype experiments.
