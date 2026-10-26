# Full-source review validation

Feature commit: `1c96d26c1477ade1efdfc70180b578e40b3f8c68`. Reference mod: `d7fa0f09a15a124609269d97f093eb2befa84a76`. Upstream: `41bfdaa7331808aff7239a9ec283cdba01b3b406`.

## Result

All 27 required local functional stages passed in the independent checkout.
Fresh Windows x64 Release and Debug builds each passed title and actual chart-CLI
startup/playback/graceful-exit checks in both orientations. The initial three
feature-history checkpoints were also built in both configurations and smoke tested.
Fresh builds avoided the intermediate NocoUI PDB warning; earlier failed historical
Debug attempts (object-section limit /bigobj) and build-cache warnings remain in
the local logs. Historical Debug flags are documented in the feature guide.

Both SDK unit configurations passed 43 cases / 3,982 assertions. Native headless
checks cover four rolling-window cases / 40,013 assertions and six Win32 timer
cases / 16 assertions. The private distribution checks passed 21 Python cases;
compiler-token safeguards passed 11 cases.

The first skin capture had an eight-second observation gap and missed required
numeric edits; it remains an incomplete failed capture. A second capture exceeded
the old raw first-frame loop timestamp bound by 9ms. An unchanged d7fa0f0 reference
executable with a finite 600ms pause of only its owned renderer independently
reproduced a false first-new-frame timing rejection. The trace checker now uses
actual observation intervals, preserving the nominal 18-second period and the
existing 0.12-second clock tolerance. Thirteen new cases reject early resets,
incorrect clock jumps and actual period drift. All required numeric-edit and
pixel checks remain; the incomplete first capture still fails those checks.
Fresh wide/classic/portrait skin reruns passed. The original failure records and
control are retained in SAMPLING_CONTROL.json and the local logs. No game code was
changed to make those captures pass; the cause of the initial host stall itself
is not claimed to have been fixed.

Game C++ inventory is 367; all game C++ and 21 changed library C++ files match
the independent reference compiler tokens and directive text. 431 managed runtime
assets match: binary assets use exact SHA256; text permits only CRLF/LF differences.
The lexer target is explicitly Windows MSVC on every CI host. No original game
source path was deleted or moved. The test branch changes no feature-branch game,
resource, project or dependency patch files.

APS: 81 RGBA captures match exactly, without exclusions. Judge setting: 110 captures
match outside only the existing FPS-label rectangle; both 2,197-row input replays
are identical. Functional tests include the five resolutions, five UI languages,
skin fallback, APS/EX, timing menus, preview, record compatibility and audio teardown.
Installer/environment tests and a real generated-payload install/run/remove test
preserved original and user sentinel files. These run only in owned test folders.

## Source statistics

The feature branch has 842 tracked files, including the
697 inherited upstream files, plus six unchanged submodule
gitlinks. Tests and development tooling are a separate comparison.

| Game C++ against upstream | Before review formatting restoration | Review |
|---|---:|---:|
| Added files | 73 | 73 |
| Modified files | 100 | 99 |
| Deleted files | 0 | 0 |
| Inserted lines | 17076 | 16619 |
| Deleted lines | 1789 | 1414 |

The conservative formatting reduction is modest. The main review improvement is
the correct shared upstream history and separating development tools, rather than
claiming game implementation disappeared. Serialized UI resources account for
much of the full diff. Dependency source diffs are measured separately from their
patch containers in STATISTICS.json. Its remainingFormatting list identifies
changed/uncertain files retained conservatively, not proven formatting-only changes.

## Performance

Comparison uses the current validated mod, not the pre-timer/pre-refactor binary.
Native actual-play ETW: three repetitions per build/orientation, three-second
warmup plus eight-second capture. Long diagnostic protocol: three predeclared
batches, five 960-frame subruns, six scenes and two builds (180 measured subruns).
No other local builds or test suites ran during these measurements. Both gates
passed the unchanged threshold: regress only when exceeding both 5% and 0.1ms.
See VALIDATION.json for per-case medians and deltas. ETW measures submitted-frame
intervals, not physical input-to-photon latency.

## Reproduce

Use the feature branch's Windows build instructions. A safe manual runtime is a
new copy of stock KSM 2.1.0: copy the tracked kshootmania/App resources into that
copy, then copy the generated mod EXE beside the original EXE and run the mod with
that directory as its working directory. Leave installed user folders untouched.

On this test branch install Python libclang==18.1.1, clang-format==19.1.5, Pillow
and NumPy. Run ApplyDependencyPatches.py, VerifyReviewSource.py and CheckCppStyle.py.
Run BuildUnitTests.ps1 for Release and Debug, then RunHeadlessTests.cmd.
The complete local runtime sequence is ValidateRefactor.ps1 -Tier Full with saved
baseline/resource inputs, followed by APS visual, environment and payload smoke
tests. RunAudioTests.ps1 prepares the mock audio fixture before judge-setting tests.
Ordinary original songs, SDK/runtime DLLs and the local authoring reference-image
snapshot must be supplied separately for the full pixel suite; they are not committed.
PrepareTestSongs.ps1 imports only local ignored test inputs without changing originals.

GitHub Actions runs direct full-source parity, added-code style, lexer safeguards,
portable rolling-window tests and native Windows timer tests. It does not reconstruct
the private patch package or pretend to exercise a GPU/SDK/physical ASIO device.
Every run records CORE_TARGET.json. Actual ASIO hardware and other-OS gameplay
remain unverified. The original private main/refactor branches, development folder,
play-test and old releases were preserved. No PR, merge, release or reviewer message
is part of this change.

The reference full restored snapshot contains 969 tracked files. The core review separates 127 existing development files. The companion branch contains 981 tracked files (139 beyond core, including the new review safeguards and evidence). Six original gitlinks are counted separately.
