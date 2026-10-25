# Empty-directory removal verification — 2026-10-26

Version: 0.5.1-beta1; installer revision: 20261026-cleanup1.

## Behavior

- New optional createdDirectories retains schema 3 / legacy schema 2 compatibility.
- Newly added missing payload parents are recorded; explicit [] never invokes legacy inference.
- Legacy cleanup uses registered exclusive areas only and preserves the common mods root.
- File removals and original restoration commit before any directory removal.
- Candidates are validated against sources, duplicates, lexical bounds and independent protected paths.
- Root/shared/user/record/config/log/backup/recovery paths remain protected.
- Ancestor and leaf directory handles reject reparse points and pin names against replacement.
- SetFileInformationByHandle(FileDispositionInfo) deletes only empty directories; no recursive deletion.
- A committed journal stores candidates/sources for idempotent restart; cleanup failures do not roll files back.
- JSON reports list removed/preserved directories and warnings; GUI reports warnings without hiding successful file removal.
- Korean/English/Japanese guides retain the previous text and add cleanup rules.

## Evidence

- Native engine: 255 assertions PASS, including newly tracked multilevel folders, pre-existing
  empty directories, explicit empty inventory, old metadata, protected path/type/case tampering,
  hidden/user files, user subdirectories, forced interruption during cleanup and restart by inspect/remove.
- Both a file added during deletion and a rename attempt were preserved/blocked respectively.
- Real NTFS junction targets stayed unchanged and removal stopped before any payload mutation.
- Real directory sharing lock produced a warning and kept the directory, with all original data unchanged.
- Real original-runtime copies: four landscape/portrait title/chart starts, graceful exit,
  install/remove/reinstall and tracked empty-folder checks passed; 354 baseline/sentinel files preserved/restored.
- Real older production installer created an unmodified legacy record; current removal cleaned exclusive
  areas, kept common mods and preserved 13 user-data sentinel files.
- Real current production GUI: 100 percent completion and a retained-directory warning visually inspected;
  protected records/settings remained byte-identical.
- ZIP entries, CRC, SHA256, extracted executable and original-runtime inspection passed.
- Fixed normal game executable remains F5E014B9E6DE87037403A49F1652C9B6DE308A61DE49BEE4B66100882D232F70.

## Delivery

Latest package directory: releases/ksm-enhanced-vertical-0.5.1-beta1-installer-cleanup1
ZIP bytes: 77190843
ZIP SHA256: 9dca1f2daae642fe381da849bd7177929ad7c6e8d1fd1ce36a7556226e101a14
Setup SHA256: fe63a77a082875ee159365351de5e8fc5246cfd5382cd7730bba5eaf4ad40224

The previous ZIP was open in another process. Its entire release directory was restored
and hash verified against the pre-change backup. The new verified package was published
to a separate revision directory instead of disturbing that open archive. Backup and
promotion paths are recorded in build/native-installer/cleanup-release-promotion.json.
No actual user installation, game binary, play-test, Git branch or remote was changed.

Limits: physical GUI button clicks / UAC approval round trip and other Windows/ASIO devices
were not re-tested. Fault injection uses a separate synthetic test package; the GUI lock
and older-installer integration tests use actual production executables and isolated runtimes.
