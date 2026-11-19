KSM Enhanced Vertical Mod 0.5.5-Beta5
Supported game: K-Shoot Mania 2.2.0, Windows x64

# Supported version
This setup supports original KSM 2.2.0 only. It cannot update or remove a mod installed for 2.1.0.
To migrate a 2.1.0 installation, back up the original first, remove the mod with its own setup, then apply the official KSM 2.2.0 update. Do not replace only the original EXE while the mod is installed.
No separate PowerShell, Python or .NET installation is required to run this setup.

# Installation and updates
1. Close any game running from the target folder.
2. Run KsmEnhancedVertical_0.5.5-Beta5_Setup.exe.
3. Choose the KSM 2.2.0 folder containing kshootmania.exe. Placing setup in that folder enables automatic detection.
4. Select Install mod. Update mod is enabled for a supported existing 2.2.0 mod installation.
5. After completion, run ksm-enhanced-vertical.exe in the same folder. You can still run the original kshootmania.exe.

The setup defaults to Japanese and supports EN / JP / KR / SC / TC. Existing game language, key bindings and offsets are preserved.
Setup checks for a newer installer at startup. Use Check for updates to check again, then Open release page when a compatible release is available. Downloads and installation are manual. Local installation and removal work without an internet connection.

# Removal and preserved files
Keep this setup. To remove the mod, select the same game folder and choose Remove mod. No separate uninstaller is installed. Manual installations without an installation record cannot be removed automatically.
Normal, EX and course records, newly earned records, score backups, settings, songs and user skins are preserved. Images you edited or deleted remain in that state.
Unchanged mod files are removed or restored to their first-install originals. Edited language and other managed files are retained in .ksm-enhanced-recovery/<transaction>/.
Empty mod folders created by setup are cleaned up safely. skins/, user files, backups and recovery folders remain.
Do not edit or delete .ksm-enhanced-install.json or .ksm-enhanced-backup/. After an interruption, run the same setup and select the same folder to recover.

# Mod features
- Landscape and portrait gameplay, with portrait UI
- Viewport customization and user skins
- P-CRITICAL, EX scoring, additional records and result displays
- HARD+APS and an option to remember the selected EFF.RATE
- Judgment timing settings, ASIO output and per-device additional judgment calibration
- Laser, laser slam and note thickness, plus key-beam length and opacity

# Using skins
Installation and updates create skins/ in the game folder if it is missing. Create a separate folder inside it for each skin.
Example: <game folder>/skins/MySkin/button.png
Match the relative paths and filenames used under imgs/. You do not need to replace every image: missing images fall back to the defaults in imgs/.
Choose your skin in Viewport Custom. The Yellow missing-image setting helps identify images you have not replaced. Mod images are also included under imgs/.

# Reporting problems
Include the installer version, build ID, Windows version, original KSM version, steps to reproduce and error details.
https://github.com/yhc308/ksm-enhanced-vertical-mod/issues

Game log: <game folder>/log/session-<number>-pid<number>/events-0.log
Session information: session.txt in the same folder
Last setup operation: %TEMP%/KsmEnhancedVerticalSetup-last-result.json
Licenses: <game folder>/mods/enhanced/ and mods/enhanced/licenses/

Check write permissions, free space and whether the game is closed. Setup offers elevation when needed and does not overwrite an unsupported installation or damaged backups.
This is an unsigned test-distribution installer.
