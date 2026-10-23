# KSM Enhanced Vertical Mod — private source repository

This is an independent private repository, not a GitHub fork. It stores the mod's
new files and changes against **K-Shoot MANIA 2.1.0** without including a complete
copy of upstream source or history. The original public review fork is separate.

## Prepare the complete source

Install Git and Python 3, then run:

```sh
python scripts/PrepareSource.py
```

The script checks the supplied file hashes, obtains the pinned official upstream
commit, applies the main patch, copies `overlay/`, initializes the original
submodules and applies the dependency patches. The generated `upstream-worktree/`
is ignored by Git. An existing destination is never overwritten.

Build instructions are in `upstream-worktree/README_ENHANCED.md`. Windows x64,
Visual Studio 2022 C++ build tools and OpenSiv3D 0.6.16 are required. Set
`SIV3D_0_6_16` before using the build script. Runtime dependencies and an original
KSM installation are obtained separately; this source repository is not a game ZIP.

## Layout

- `SOURCE_MANIFEST.json`: pinned upstream revision and source integrity hashes.
- `patches/ksm-v2.patch`: changes to existing upstream files.
- `overlay/`: new mod source, UI resources, dependency patches and build helpers.
- `scripts/PrepareSource.py`: reproducible source reconstruction.

Ordinary/TEST song collections, prebuilt executables, packages, user settings,
records, logs and credentials are excluded. Preview_Piano test charts/audio are
included as requested; chart metadata credits Hyung. Original copyright and
license notices are retained.

Repository access must remain private unless the owner explicitly chooses to
publish it. No binary releases are published by this repository setup.

## Latest source update

Development build `20261023-171521-0937f555` adds EX maximum persistence to additional score records and restores base images to `imgs/`, with user skins in `skins/<skin name>/`. Legacy ten-field additional records are readable. Original `.ksc` files retain their format. Source reconstruction and existing user data remain separate.
