# Baseline test fixture findings

These issues were reproduced before refactoring game code. Corrections affect tests only.

1. `MakeSRandomNoteLanes generates valid assignment`: the synthetic chart had no BPM at pulse zero. The production conversion requires a valid BPM map; the other randomization fixtures already supplied one. Added 120 BPM and retained all lane validity/overlap assertions.
2. `HighwayScroll integrates partial scroll_speed interpolation across zero`: for velocity `-1 + pulse/480`, the signed area from the 480 origin to either 240 or 720 is +60 pulse units. Both notes must have the same screen displacement above the origin. The old test expected an asymmetric ordering. The corrected test retains the original 33-pixel displacement and verifies the independently derived symmetry.

The original game implementations were not changed to make these tests pass. Windows test setup now embeds the original Siv3D engine resources and returns from the engine entrypoint normally; JUnit assertion failures are checked independently of the process exit code.

## Pre-existing authoring/runtime chart difference

The immutable mod base bcb8c09 contains an additional FX-B chip at pulse 7440 in `mods/enhanced/judge_setting/Preview_Piano.kson`. The current external authoring folder does not contain that chip. Runtime chart SHA256 is `484186980c4f9e522f4d885f1e4059adfa8b86186e54b2f8ffaefdf05a54e3bd` (841 bytes); the external authoring chart is `09cf692996c18b0255d9c450d3e02cb0b2c873640048e49510302f6112b90680` (836 bytes). The preserved development runtime and the refactored runtime both match the pinned 841-byte mod chart.

The original authoring-parity assertion fails on this unchanged baseline too. The failed validation is retained in `build/validation/20261023-194504`. No note, song offset, audio or supplied original was edited to silence it. Refactoring parity is checked against a separately hashed runtime-resource baseline; external authoring parity remains a known discrepancy outside this refactoring. UI/replay pixel checks run separately and keep their original expectations.

## Resource checkout line endings

The preserved runtime snapshot has 299 managed resources. Reconstructed source differs on 28 text resources only by CRLF versus LF (JSON/NocoUI scene data and license texts); all 271 remaining resources are byte-identical. The difference existed in the original private source export and is reproduced without this refactor. The exact-hash failure is retained. Validation keeps every original raw hash and separately derives an LF hash for known text extensions; an explicit checkout-line-ending mode accepts only that exact CRLF/LF transformation. Binary images/audio and file names must still match byte-for-byte. No authoring or runtime resource is rewritten. Final source synchronization excludes runtime resource files, retaining the development folder's original bytes.
