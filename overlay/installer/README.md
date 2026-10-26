# Native mod installer

The installer is an independent Win32 C++17 application. No game source is modified.
`src/InstallerCore.cpp` owns target validation, protected paths, SHA256, staging,
transaction journals, recovery, legacy schema 2 migration and removal policies.
`src/Main.cpp` provides the localized GUI and non-interactive JSON command interface.
Both use the same `Engine::Run` implementation. Production and fault-test builds are separate.

## Development build

Requirements on the build machine: Visual Studio 2022 C++ Build Tools / Windows SDK,
PowerShell and Python 3. These are not end-user dependencies.

From the project root:

```powershell
./installer/Build.ps1
./installer/Build.ps1 -TestEngine
python installer/tests/TestEngine.py
python installer/BuildPackage.py --output build/native-installer/cleanup-revision-3/KsmEnhancedVerticalSetup.exe
./installer/tests/RealRuntime.ps1 -ProjectRoot (Get-Location).Path -OriginalGameDirectory <original-KSM-folder> -BuildInformation build/latest.json -SetupExecutable <assembled-setup.exe>
python installer/PackageRelease.py
```

Assembly and release scripts refuse to overwrite existing output files/directories.
Use a new output path for another build; intentionally select that path in the release
script. The fixed normal game build and checksum are enforced by BuildPackage.py.
Do not replace build/latest.json to point at a diagnostic executable.

## CLI

```text
KsmEnhancedVerticalSetup.exe --quiet --action inspect --target "C:\Games\KSM" --report "C:\Temp\inspect.json"
```

Actions: inspect / install / verify / remove. The optional language index is
`--language 0..4` (EN / JP / KR / SC / TC). Results use English JSON and exit codes
0 (success) or 2 (failure). Wait for the process when invoking this GUI-subsystem EXE
from scripts. Without --target, only the installer's directory is considered;
the process working directory is not used for automatic target selection.

## Data safety

Installation ownership schema 3 is stored in `.ksm-enhanced-install.json`.
Original, undo and staged bytes live below `.ksm-enhanced-backup/<transaction>/`.
The durable journal is written before target mutations; interrupted transactions
are rolled back before a subsequent operation. Invalid metadata/backups stop safely.
All score/config/song/skin paths are denied independently of installation metadata.
Edited/deleted images remain untouched during removal. Edited language/other owned
files are recovered, then originals restored. Identical pre-existing files are not owned.
Backup and recovery files are deliberately retained rather than recursively deleted.
Do not hand-edit installation state. The same EXE performs removal; no manager is installed.

## Test evidence

`tests/TestEngine.py` invokes the real native test executable with isolated synthetic
payloads. It checks state compatibility, data preservation, malformed metadata,
file locks, permissions, low-space injection, cancellation and process termination.
Hardlink rejection runs on an NTFS temporary volume because the development drive
cannot create hardlinks. `CancelEngine.cpp` requests cancellation through the real
atomic cancellation interface after the first applied operation.
`tests/RealRuntime.ps1` copies only original runtime assets and then exercises the
production package, real normal game startup/exit in both orientations, running-game
rejection and reinstall preservation. No original user runtime is installed into.

Release VERIFICATION.json lists actual evidence and remaining environment/UI limits.
Fault-injection hooks are compiled out of the production stub and checked absent.
Dependency inspection confirmed only built-in Windows DLLs. Third-party provenance
and licenses are recorded in third_party/ and installed with the payload.

## 2026-10-26 UI revision

The setup now embeds an original package/download icon in seven sizes. `progressValue`
is separate from status text; prepare/apply phases use actual file/operation counts,
remain below 100 until success, and the Win32 UI removes marquee mode before setting
a full 100-percent completion bar. Failures retain incomplete progress and use the
error state. A new target resets progress. Actual GUI install/remove/failure states
were inspected at 100/100/0 respectively. Translation templates now preserve the
user-edited Korean guide and its English/Japanese counterparts, including log paths.
Use --setup and --output-directory to build a candidate without replacing an existing
release. Only publish after ZIP verification and a hash-verified backup.

## 2026-10-26 wording revision

Missing kshootmania.exe is classified as missing_executable with the raw path detail.
The UI explains that the selected folder lacks the original EXE, separately from
rollback/error messages. All five locales explain original-game availability and
preservation of EX records and skin settings. The footer was expanded to 82 pixels;
measured word-wrapped text height is 69 pixels in each supported locale. Final Korean
wording and the full footer were visually inspected in the production GUI.

## Empty-directory cleanup revision (20261026-cleanup1)

Ownership schema 3 has optional createdDirectories. Absence uses conservative legacy
cleanup of registered mods/enhanced, mods/vertical and skin_preview parent paths;
an explicit empty array means no cleanup. Existing/common/protected directories are
preserved. New installs can remove a newly recorded empty mods root; legacy installs
keep that shared parent. New directory results are removedDirectories,
preservedDirectories (path/reason) and cleanupWarnings (path/reason/systemCode).

All directory metadata is validated before file mutation against valid source-file
parents and an independent protected-path denylist. Cleanup is post-commit. Its
candidate/source inventory is durable in the journal, so a committed removal can
resume without restoring removed files. Empty-only handle disposition is used with
ancestor/leaf handles that deny delete sharing and do not follow reparse points.
Concurrent new files prevent deletion. Folder cleanup failures do not undo a
completed file removal; the GUI and JSON report warn about the remaining paths.

Verification: 255 native assertions; real runtime install/play/remove/reinstall;
real prior-production installation removal; NTFS junction, sharing lock, hidden
files, user directories, metadata tampering, concurrent file addition/rename and
crash/restart cases. Real production GUI warning at 100 percent was inspected.
The latest local package is under releases/ksm-enhanced-vertical-0.5.1-beta1-installer-cleanup1/.
The previous release folder was restored and verified after its open ZIP blocked replacement.
