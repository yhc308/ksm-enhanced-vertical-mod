# Development guide

This repository stores KSM Enhanced Vertical Mod as a patch plus overlay against **K-Shoot Mania 2.1.0**. It does not contain a complete copy of the original game source or its history. Original copyright notices and licenses are retained.

For playing the mod, use the [installer release](https://github.com/yhc308/ksm-enhanced-vertical-mod/releases/tag/v0.5.1-beta1). The tools below are for development only.

## Choose the source version

The source for the published **0.5.1-beta1** build is on [`release/0.5.1-beta1`](https://github.com/yhc308/ksm-enhanced-vertical-mod/tree/release/0.5.1-beta1). The `main` branch retains an earlier source snapshot; its updated project README does not mean the release code has been merged into it.

For an upstream-based view with the original Git history, see the [equivalent full-source branch](https://github.com/yhc308/ksm-enhanced-vertical/tree/enhanced-vertical).

## Reconstruct the source

Install Git and Python 3, then clone the release branch:

```sh
git clone --branch release/0.5.1-beta1 https://github.com/yhc308/ksm-enhanced-vertical-mod.git
cd ksm-enhanced-vertical-mod
python scripts/VerifySource.py
python scripts/PrepareSource.py
```

The preparation script verifies the supplied hashes, obtains the pinned official upstream commit `41bfdaa7331808aff7239a9ec283cdba01b3b406`, applies the main patch, copies the overlay, initializes original submodules and applies dependency patches.

The generated `upstream-worktree/` is ignored by Git. An existing destination is never overwritten.

## Repository layout

| Path | Purpose |
| --- | --- |
| `SOURCE_MANIFEST.json` | Pinned upstream revision and source integrity hashes |
| `patches/ksm-v2.patch` | Changes to existing upstream files |
| `overlay/` | Added mod source, UI resources, dependency patches and build helpers |
| `scripts/VerifySource.py` | Release source package integrity checks |
| `scripts/PrepareSource.py` | Reproducible source reconstruction |
| `upstream-worktree/` | Generated complete source tree, excluded from Git |

## Build and implementation details

Game development requires Windows x64, Visual Studio 2022 C++ build tools, OpenSiv3D 0.6.16, Python 3 and PowerShell 7. Set `SIV3D_0_6_16` before using the build script. Obtain runtime dependencies and an original KSM installation separately.

- [Game build and implementation guide](https://github.com/yhc308/ksm-enhanced-vertical-mod/blob/release/0.5.1-beta1/overlay/REVIEW.md)
- [Source and release overview](https://github.com/yhc308/ksm-enhanced-vertical-mod/blob/release/0.5.1-beta1/overlay/README_ENHANCED.md)
- [Installer architecture and build instructions](https://github.com/yhc308/ksm-enhanced-vertical-mod/blob/release/0.5.1-beta1/overlay/installer/README.md)
- [Validation results and limitations](https://github.com/yhc308/ksm-enhanced-vertical-mod/blob/release/0.5.1-beta1/overlay/docs/RELEASE_0.5.1-beta1.md)

These documents are also available in the reconstructed tree. The native installer needs Windows C++ tools to build, but does not require Siv3D.

**End users do not need to install Python, PowerShell or .NET to use the installer.**

## Source contents and compatibility

SDKs, compiled executables and DLLs, build outputs, ordinary/TEST song collections, user settings, records, execution logs and credentials are excluded from Git. The included Preview_Piano charts and audio credit Hyung in their chart metadata.

The mod loads base images from `imgs/` and user skins from `skins/<skin name>/`. Additional EX records include the chart's theoretical maximum EX score and retain compatibility with older ten-field records. The original `.ksc` record format remains unchanged.

Source reconstruction and development work are kept separate from existing user game data. Binary releases are published in this repository; the public fork provides the upstream-based source view.
