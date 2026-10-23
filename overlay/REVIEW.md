# Review guide

Upstream: https://github.com/kshootmania/ksm-v2
Base: `41bfdaa7331808aff7239a9ec283cdba01b3b406` (KSM 2.1.0)
Repository: independent private `ksm-enhanced-vertical-mod`
Refactoring branch: `refactor/upstream-style-parity` (main/public fork remain unchanged)

The behavior baseline is development build 20261023-171521-0937f555, including EX maximum persistence and the separated skin layout. The refactoring branch changes source organization and naming while preserving that baseline; current evidence and remaining gates are in `docs/REFACTOR_VALIDATION.md`. Runtime
executables and release ZIPs are deliberately excluded from the commit.

## Main areas

- `kshootmania/src/Vertical`: portrait camera/viewport geometry and configuration.
- `MusicGame`: APS, detailed judgments, EX SCORE and presentation state.
- `Scenes/Title`: Judge Timing Setting and skin/viewport preview overlays.
- `App/mods`, `App/ui`, `App/imgs`, `App/skins`: mod UI definitions and image resources.
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
