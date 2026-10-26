# Native installer implementation report

Date: 2026-10-26
Version: 0.4.0-beta.1
Game build: 20261023-234835-524ce9c3 (normal Release; unchanged)

## Delivered

- Independent x64 Win32 setup EXE; install/remove share a native transaction engine.
- Module-relative automatic detection and native folder picker; five languages, Japanese default.
- Side-by-side mod executable; protected original executable, records, settings, songs and skins.
- File-level image/language removal policies, original backups and durable rollback/recovery journal.
- CLI inspect/install/verify/remove with English JSON reports; legacy schema 2 state reading.
- Same-version reinstall prevention, running-game/lock/permission/space checks and elevation retry path.
- 263 compressed payload files, including required internal piano resources and licenses.
- ZIP root: setup EXE, README_KO/EN/JA.txt and English BUILD_INFO.json only.
- External file hashes, verification report and ZIP SHA256. No Git upload or release publication.

## Verification

- Native engine: 127 assertions passed using the actual compiled engine.
- Cancellation: real cancellation flag after first write; original inventory restored.
- Crash/failure recovery: injected separate test build; restart restored the transaction.
- Permission failure, hardlinks, duplicate/traversing/protected paths, corrupted backups/package/journal rejected.
- Modified and deleted images retained; original language restored and edits recovered.
- Both old/new additional score formats preserved byte-for-byte, including new post-install records.
- Real original-runtime copies: production install / verify / remove / reinstall passed.
- Four normal game startup/shutdown cases: landscape/portrait, title/chart CLI.
- Running game removal rejected; original 354-file/sentinel inventory preserved/restored.
- Production fault-injection environment variables did not alter execution.
- Automatic target recognition worked independently of the process working directory.
- Japanese default and five language changes inspected through computer-use; native folder dialog opened.
- ZIP CRC and all archive entry hashes passed; extracted setup inspected the original runtime without installation.
- PE dependency inspection found only built-in Windows libraries, no .NET/Python/PowerShell/VC runtime dependency.

## Explicit limits

Actual UAC approval/elevated retry was not executed. Interactive folder selection and
GUI install/remove clicks were not fully automated; the shared engine was tested via CLI.
Low disk space and interrupted writes were injected; no physical disk was filled.
ASIO hardware and other Windows installations were not re-tested. The game binary
is identical to the fixed previously verified Release build. These limits are also
included in the English verification report; they are not counted as passed tests.

## Artifacts

Directory: `releases/ksm-enhanced-vertical-0.4.0-beta.1-installer/`
ZIP bytes: 77119733
ZIP SHA256: `a57d4625b90d50d159541a70aef624df530c4cf15da8207ab0ba9eb8bf1a0c0e`
Setup SHA256: `3429f3f3ed683534650f0427dd3705c08fecb8d1a04ec499a8d449e4659fa7f2`

The original development binary, original KSM runtime and play-test were not deployment
targets. New work is confined to installer/, docs/, build/ and the new release directory.
