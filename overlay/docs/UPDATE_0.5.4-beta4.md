# 0.5.4-beta4 installation and settings migration

Setup adds a single update transaction for supported schema 2/3 installations and writes schema 4. Original backups are copied into the new transaction inventory so eventual removal restores the first-install baseline. Update undo bytes describe the immediately preceding runtime. Journals identify their action so committed update cleanup cannot be mistaken for a completed removal.

CLI actions: `inspect`, `install`, `update`, `verify`, `remove`. Inspection includes `updateAvailable` and `updateBlockedReason`. Same-version and downgrade updates are refused. Untracked manual installs remain blocked.

Edited/deleted tracked images are retained, including retired image paths. Edited managed language/other files are copied into recovery before replacement. Records, configuration, songs, user skins and linked user song directories are never update targets.

ASIO correction is an optional integer `asio_judgment_correction_ms` in the existing version-1 device-profile objects. Missing values use +15ms; values are clamped to −100..100ms. The six legacy profile fields remain mandatory and unchanged. Native backends apply no correction. Actual driver latency and audio/render clocks are not rewritten. Malformed profile files are retained and saving is blocked with an error rather than overwriting them.

New display settings use existing default/migration readers. Legacy key-beam OFF becomes Normal with zero opacity. Nine-field original and ten/eleven-field additional scores remain readable without batch conversion.
