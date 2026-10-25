# Enhanced Vertical: upstream review

This branch is a full-source review view of the existing mod, based directly on
K-Shoot MANIA 2.1.0 `41bfdaa7331808aff7239a9ec283cdba01b3b406`. Its game behavior is frozen at mod commit
`d7fa0f09a15a124609269d97f093eb2befa84a76`. The game implementation is unchanged; the release branch additionally includes the native installer. No newer upstream game implementation is merged.
The original upstream license, source paths and six submodule revisions remain.

## Comparisons and reading order

- [Complete game changes](https://github.com/yhc308/ksm-enhanced-vertical-mod/compare/review%2Fupstream-2.1.0...review%2Fcurrent-features)
- [Behavior-preserving implementation separation](https://github.com/yhc308/ksm-enhanced-vertical-mod/compare/e71cedb61ecfcd8f7296e4ba1bcfa1a2baecae0f...ff402e7e3b46b2122627104dc7f64489a93f0c4e)
- [Windows frame deadline timer only](https://github.com/yhc308/ksm-enhanced-vertical-mod/compare/ff402e7e3b46b2122627104dc7f64489a93f0c4e...414a091d05be9a8952f6db5b71f3d01c779bb1d2)
- [Companion tests and development tools](https://github.com/yhc308/ksm-enhanced-vertical-mod/compare/review%2Fcurrent-features...review%2Fcurrent-features-tests)

Start with `kshootmania/src/Main.cpp` and `Diagnostics/DiagnosticRunner.cpp`;
diagnostic implementation stays registered in the game project but is disabled
in normal builds. Then read `Vertical/VerticalLayout.hpp`, `Vertical/ModOptionsScene.cpp`,
`Scenes/Title/SkinOverlay.cpp` and the highway/HUD graphics for display behavior.

For HARD+APS read `MusicGame/Judgment/NormalGaugeState.*`, `ScoringStatus.*`,
`MusicGame/GameMain.cpp` and the gauge HUD. Actual state and displayed interpolation
remain distinct. For EX and record compatibility read `HighScore/AdditionalScoreIO.*`,
`HighScoreInfo.*` and `MusicGame/PlayResult.cpp`. Original nine-field `.ksc` rows
and ten/eleven-field additional records retain their existing interpretation.

For timing configuration read `MusicGame/JudgeSettingSession.*`,
`Judgment/RecentTimingWindow.hpp` and `Scenes/Title/JudgeSettingOverlay.*`.
Preview uses the gameplay renderer with hi-speed 450; judge setting loops at
16 seconds. Skin lookup uses base `imgs/` and custom `skins/<name>/`.
`Common/SkinPreset.*` owns selection/fallback. Existing settings keys, filenames,
UI languages, input mapping, score and clear policies have not been redesigned.

Audio changes remain explicit patches under `patches/dependencies/`; read their
manifest and then the ksmaudio ASIO/stream changes and NocoUI asset change.
Submodule pointers are not replaced by unpublished dependency commits.

## Build on Windows x64

1. Clone this branch, then `git submodule update --init --recursive`.
2. Install Visual Studio 2022 C++ tools, Python 3, PowerShell 7 and OpenSiv3D 0.6.16.
3. Run `python scripts/ApplyDependencyPatches.py`.
4. Set `SIV3D_0_6_16` to the SDK root and run `pwsh -File scripts/Build.ps1 -Configuration Release`
   (or Debug). Normal builds use `KsmEnableDiagnostics=0`.
5. Run the generated mod executable in a separately prepared KSM 2.1.0 runtime.
   Use a copy when testing. Keep the original executable, user settings and scores.
   The source checkout is not a complete standalone binary distribution.

The initial feature-import checkpoint has the old large diagnostic entry point;
its historical Debug build needs the MSVC `/bigobj` flag (for example append it to
`_CL_`). The later split removes that entry-point pressure. Local validation also
bounded compiler concurrency with `/MP2`; this is not a gameplay setting.
Official BASSASIO/BASSmix x64 DLLs are separate runtime inputs for ASIO.
Do not commit SDKs, runtime binaries, ordinary song collections or private data.
The included small Preview_Piano resources credit Hyung in their track metadata.

## Formatting and validation

The final game still contains 367 C++ source/header files. No original game source
was moved or deleted. Identical complete files and unambiguous complete function
bodies were conservatively restored to upstream spelling with the Clang 18 lexer.
All resulting game C++ and changed library C++ retain the reference compiler tokens;
preprocessor directive text is also checked. Comments and literals are retained.
Uncertain regions stay as they were. This is a token/directive comparison, not a
claim that executables or debug line tables are byte-identical.

Serialized `.noco` UI data is included and collapsed by GitHub's generated-file
display setting; expand it to inspect it. It is counted separately from C++ in the
review statistics. No data was removed to make the comparison appear smaller.

The companion `review/current-features-tests` branch carries reproducible source
expectations, added-code style checks, unit/headless/native Windows timer tests,
the existing complete runtime test harness, installer/package tools and local
validation evidence. See its `docs/review/VALIDATION.md`, `STATISTICS.json` and
`CORE_TARGET.json` for results and the exact feature commit tested. SDK/GPU/physical
ASIO coverage is explicitly distinguished from the portable CI checks.

Do not compare these full-source branches with the independent patch/overlay
`main`: use `review/upstream-2.1.0` as the base shown above.
