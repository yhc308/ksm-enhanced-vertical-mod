# KSM Enhanced Vertical Mod

Unofficial Windows x64 mod for **K-Shoot Mania 2.1.0**. This independent public
repository stores the mod as a patch plus overlay, not a complete prebuilt game.
Original upstream copyright notices and the license remain intact.

## Install / remove

Download [0.5.1-beta1](https://github.com/yhc308/ksm-enhanced-vertical-mod/releases/tag/v0.5.1-beta1),
extract the ZIP, and run KsmEnhancedVerticalSetup.exe. Select the folder containing
kshootmania.exe. The original executable remains available; start the mod using
ksm-enhanced-vertical.exe. Use the same setup EXE to remove the mod.
Normal/EX/course records, songs, user skins and settings are preserved. Recorded
empty mod-created directories are cleaned only after removal commits. Folders
containing user files are kept; backups/recovery data and protected folders remain.
KO/EN/JA guides are in the ZIP. Setup defaults to JP and supports EN/JP/KR/SC/TC.

## Source / build

The release source is on **release/0.5.1-beta1**. The historical default main branch
is preserved; select the release branch before reconstructing or reviewing it.

```sh
git clone --branch release/0.5.1-beta1 https://github.com/yhc308/ksm-enhanced-vertical-mod.git
cd ksm-enhanced-vertical-mod
python scripts/VerifySource.py
python scripts/PrepareSource.py
```

PrepareSource.py verifies SOURCE_MANIFEST.json, obtains the pinned official upstream
commit 41bfdaa7331808aff7239a9ec283cdba01b3b406, applies the patch and overlay,
initializes original submodules and applies dependency patches. An existing
reconstruction directory is never overwritten. Full game build instructions are in
upstream-worktree/README_ENHANCED.md and REVIEW.md. Visual Studio 2022 C++ tools,
OpenSiv3D 0.6.16, Python 3 and PowerShell 7 are development prerequisites.
Installer engine builds require Windows C++ tools but not Siv3D; see installer/README.md
inside the reconstructed tree. End users need no Python, PowerShell or .NET installation.

The [public fork release branch](https://github.com/yhc308/ksm-enhanced-vertical/tree/release/0.5.1-beta1)
provides an equivalent full-source view with upstream history. Binary releases are
published only here. Existing review/refactor branches retain their historical roles.

SDKs, compiled executables/DLLs, build outputs, normal/TEST song collections,
user settings, records, execution logs and credentials are excluded from Git.
Approved Preview_Piano resources credit Hyung in their chart metadata.
See the reconstructed docs/RELEASE_0.5.1-beta1.md for validation and limitations.
