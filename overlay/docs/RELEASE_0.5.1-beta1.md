# 0.5.1-beta1 release validation

The native setup uses the unchanged normal game Release build 20261023-234835-524ce9c3.
The six compiled installer inputs in RELEASE_0.5.1-beta1.json match the locally tested
production package. Game source remains at the frozen compiler-token/runtime-resource
baseline; this branch adds installer code and supporting documentation, not new judgment behavior.

255 native assertions passed: preservation of 9/10/11-field records, settings and skins;
image/language policies; malformed metadata and protected paths; locks and permissions;
rollback/restart; multilevel empty-folder ownership and legacy cleanup; hidden/user
files and folders; NTFS junctions; concurrent file addition/rename; cleanup interruption.
Four landscape/portrait title/chart runtime cases and install/remove/reinstall passed.
An actual earlier production installer was installed, then removed with this setup.
A real directory sharing lock produced a visible warning at 100 percent completion;
protected records/settings remained byte-identical.

These are local Windows results. CI verifies source reconstruction/integrity, frozen
game parity and portable/native timer tests. Tests requiring original runtime assets,
SDK or devices are separate. UAC consent and physical ASIO hardware were not re-tested.
No SDK, normal song collection, user settings, records, credentials, execution logs or
prebuilt runtime is included in Git. Approved Preview_Piano resources credit Hyung.

Release assets are only the installer ZIP and its SHA256 sidecar; file lists and local
verification reports are not bundled as extra release assets. Installation/removal
policies and error log locations are described in the KO/EN/JA guides inside the ZIP.
