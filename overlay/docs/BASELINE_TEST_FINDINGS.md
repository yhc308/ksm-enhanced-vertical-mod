# Baseline test fixture findings

These issues were reproduced before refactoring game code. Corrections affect tests only.

1. `MakeSRandomNoteLanes generates valid assignment`: the synthetic chart had no BPM at pulse zero. The production conversion requires a valid BPM map; the other randomization fixtures already supplied one. Added 120 BPM and retained all lane validity/overlap assertions.
2. `HighwayScroll integrates partial scroll_speed interpolation across zero`: for velocity `-1 + pulse/480`, the signed area from the 480 origin to either 240 or 720 is +60 pulse units. Both notes must have the same screen displacement above the origin. The old test expected an asymmetric ordering. The corrected test retains the original 33-pixel displacement and verifies the independently derived symmetry.

The original game implementations were not changed to make these tests pass. Windows test setup now embeds the original Siv3D engine resources and returns from the engine entrypoint normally; JUnit assertion failures are checked independently of the process exit code.
