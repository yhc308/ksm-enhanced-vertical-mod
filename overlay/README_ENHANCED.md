# KSM Enhanced Vertical Mod 0.5.1-beta1

Unofficial Windows x64 mod for K-Shoot Mania 2.1.0. Keep your original game installation.

## Install or remove the mod

Download the native setup ZIP from the [mod release](https://github.com/yhc308/ksm-enhanced-vertical-mod/releases/tag/v0.5.1-beta1).
Run KsmEnhancedVerticalSetup.exe and select the folder containing kshootmania.exe.
Use the same setup EXE to remove the mod. Original gameplay remains available.
Normal/EX/course records, songs, user skins and settings are preserved. Only recorded
empty mod-created folders are cleaned; nonempty and protected folders are kept.
The installer defaults to Japanese and supports EN / JP / KR / SC / TC.
KO, EN and JA guides are included in the ZIP.

## Source layouts

- This full-source branch in the public fork preserves original upstream history.
- The independent [mod repository](https://github.com/yhc308/ksm-enhanced-vertical-mod/tree/release/0.5.1-beta1)
  stores the same source as a reproducible patch plus overlay. Run its scripts/PrepareSource.py.
- Binary releases are published only in the independent mod repository, not this fork.

See [REVIEW.md](REVIEW.md) for source comparisons and game build prerequisites.
The game remains based on the frozen, validated KSM 2.1.0 implementation. This release
adds the native installer, translations, progress/icon fixes, precise missing-EXE
messages and crash-safe empty-folder cleanup. See installer/README.md and
[release validation](docs/RELEASE_0.5.1-beta1.md) for installer builds/tests.
