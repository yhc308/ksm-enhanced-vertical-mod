# KSM Enhanced Vertical Mod

Unofficial Windows mod for **K-Shoot MANIA 2.1.0**, based on upstream commit
`41bfdaa7331808aff7239a9ec283cdba01b3b406`. This branch is intended for code review.
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

Compare `enhanced-vertical` with the upstream commit above. Main game changes are
ordinary source diffs. The original dependency submodule pointers are retained;
local changes to ksmaudio and NocoUI are in `patches/dependencies/` and are applied
by the script below. Generated executables, packages, local paths/reports, user
settings, credentials, scores, and ordinary/TEST song collections are excluded.

## Build (Windows x64)

1. Install Git, Python 3, Visual Studio 2022 C++ build tools, and OpenSiv3D SDK 0.6.16.
2. Clone this fork and check out the `enhanced-vertical` branch.
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
