# Upstream-style refactor checkpoint

Branch: `refactor/upstream-style-parity`; mod base bcb8c09; upstream KSM 2.1.0 base 41bfdaa.

The source refactor and required final validation are complete. See REFACTOR_VALIDATION.md for the full verification matrix, original failures, performance protocols and environment limits.

- Enforced upstream-area naming/formatting over 207 C++ files, with 168-file and modified-dependency token-preserving format audits.
- Extracted judge-setting rendering/session logic, NORMAL gauge updates, supplemental score IO, recent timing window, mod-options implementations and startup diagnostic dispatch. Hot gauge math stays inline; operation/initialization/cleanup order and resource keys remain.
- Normal builds disable heavyweight diagnostic startup implementations; opt-in diagnostic builds retain fixtures. Original native chart CLI parsing remains.
- Removed only audited duplicate includes and pure unused locals; preserved dynamic resources, compatibility readers, fallbacks and diagnostic entry points.
- Isolated and tested the Windows waitable-timer performance correction, retaining deadline/FPS/drift behavior and the original sleep fallback.
- Passed Windows Release/Debug units, standalone rolling-window/timer tests, all 27 functional stages, exact images/replays, fresh ordinary builds/smokes, installer preservation, native-play and long diagnostic performance comparison. Earlier failures remain documented.
- Backed up and synchronized 272 managed source files to the original development tree; destination Release build and four runtime cases passed. All 299 managed runtime assets are byte-identical. User records/settings/skins, authoring resources, play-test and existing releases were excluded.
- Stable checkpoints were pushed only to the private refactor branch. No main/public-fork merge or release was performed.

Source backup: `build/refactor-source-backups/20261023-234818/manifest.json` in the development tree. Final evidence and source/hash manifests are retained under build/.
