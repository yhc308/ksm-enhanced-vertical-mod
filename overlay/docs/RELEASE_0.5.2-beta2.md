# Release 0.5.2-beta2

The release fixes frame rate drops during overlapping laser rendering and installation rejection caused by modified images/UI. Errors opening certain song folders remain under investigation; Unicode/path hardening and clearer diagnostics are included, but the reported original failure has not been reproduced.

The normal Release build is `20261027-194127-779e4d6d` (diagnostics disabled), with SHA256 `ef68345d4e8126d35ac7ecf6c552b3e51b5cfce22419f0db2c7a69d999bb73e3`. The window title includes 0.5.2-beta2.

Validation completed on isolated runtime copies:

- 296 native installer engine checks, including rollback, interrupted operations, protected paths, DLL PE/export checks and empty-directory cleanup.
- 8 normal game startup cases across landscape/portrait and unmodified/customized or missing original resources.
- Actual previous 0.5.1-beta1 production installation removed by the new installer; existing and newly earned EX records preserved through reinstall.
- Modified and missing user resource acceptance, exact pre-install restoration and different-hash compatible DLL acceptance.
- GUI install/remove completion at 100%, five-language dependency errors and preflight JSON diagnostic saving.
- Installed judgment-setting and viewport-preview entry; preview hispeed 450 displayed.
- Installer ZIP CRC, file hashes and extracted installer inspection.

DLL inspection reads metadata without executing the DLL. Matching exports cannot guarantee the behavior of every third-party DLL version. Interactive UAC elevation and physical ASIO hardware were not re-tested. Existing compiler warnings are recorded in local build evidence; the normal Release build and runtime checks passed.

The package contains no original game EXE, ordinary songs, SDK, personal records, settings or runtime logs. Users remove the prior mod before installing the new version. Prior releases and tags are retained.
