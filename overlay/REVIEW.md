# Review guide

Upstream: https://github.com/kshootmania/ksm-v2
Base: `41bfdaa7331808aff7239a9ec283cdba01b3b406` (KSM 2.1.0)
Branch: `enhanced-vertical`

The game code on this branch matches the local 0.4.0-beta.1 mod source. Runtime
executables and release ZIPs are deliberately excluded from the commit.

## Main areas

- `kshootmania/src/Vertical`: portrait camera/viewport geometry and configuration.
- `MusicGame`: APS, detailed judgments, EX SCORE and presentation state.
- `Scenes/Title`: Judge Timing Setting and skin/viewport preview overlays.
- `App/mods`, `App/ui`, `App/imgs/default`: mod UI definitions and image resources.
- `patches/dependencies`: ksmaudio audio changes and the NocoUI texture-loader fix.
- `scripts/ModInstaller.ps1`: compatibility-checked install/verify/remove backend.

## Validation before the first push

- Cloned the pinned upstream tree and initialized its original submodules.
- Applied dependency patches successfully, then verified a second application is a no-op.
- Compared the patched dependency source with the production mod source.
- Built this review checkout successfully with VS 2022 and OpenSiv3D 0.6.16 (Release/x64).
- Audited staged files for credentials, personal runtime data and compiled output.

The review checkout uses patches for modified submodules; their working trees are
expected to appear dirty after preparation. Do not commit updated gitlink hashes
unless those dependencies are intentionally published as separate forks.
