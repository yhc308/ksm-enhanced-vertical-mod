# Installer UI revision — 2026-10-26

Version remains 0.5.1-beta1; installer revision 20261026-ui1.

- Original package/download icon embedded: 16/24/32/48/64/128/256px, window and shell icons verified.
- Progress reports file preparation and transaction operations. Only successful UI completion sets 100.
- Actual GUI install/remove/failure results inspected: 100 / 100 / 0; correct button states and icon.
- Native engine 129 checks passed; progress monotonicity/cancellation/data preservation included.
- Four real landscape/portrait title/chart cases, install/remove/reinstall and running-game rejection passed.
- User-edited Korean guide retained byte-for-byte; EN/JA translations include all sections and error log locations.
- ZIP CRC, entry hashes, extracted setup execution and shell icon extraction passed.
- Old release (including user README) backed up and hash-verified before replacing current local release.
- No game binary, active user installation, record/config or Git branch changed.

ZIP bytes: 77164386
ZIP SHA256: 5a0adc1c12e6bc9f053a3dda8e0489871d1197f27478cc5cbda25f0abe923700
Setup SHA256: 734d990e9bf848cebb048cda709fb4ea1f717e232bd8aba0a327c9222ccacacc

Artifact: releases/ksm-enhanced-vertical-0.5.1-beta1-installer/ksm-enhanced-vertical-0.5.1-beta1-installer.zip
Backup location is recorded in build/native-installer/ui-release-promotion.json.
Interactive UAC consent and physical install/remove button clicks remain unautomated;
the same real GUI actions were initiated using --gui-action in isolated test copies.
