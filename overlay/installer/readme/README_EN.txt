KSM Enhanced Vertical Mod 0.5.1-beta1 Installation Guide
Supported version: K-Shoot Mania 2.1.0 Windows x64

# Installation
1. Close kshootmania.exe if it is running.
2. Run KsmEnhancedVerticalSetup.exe.
3. Select the folder containing kshootmania.exe, then click [Install mod]. (If the setup EXE is in the KSM folder, that folder is selected automatically.)
4. After installation, run ksm-enhanced-vertical.exe in the same folder to launch the mod.
5. The original kshootmania.exe is kept.

# Removal and reinstallation
* Manual installations that were not installed through this setup EXE cannot be removed by the installer.
1. Run the same setup EXE again, select the KSM folder, then click [Remove mod].
2. Keep this EXE: no separate uninstaller or Start menu entry is installed.
3. To switch to a different mod version, remove the current version before reinstalling.

# Important notes when removing the mod

Do not edit the installation record (.ksm-enhanced-install.json) or its backups. Doing so can make recovery impossible.
Edited language files and other mod files (such as images added by the mod) are backed up under .ksm-enhanced-recovery/<transaction ID>/.
Backups and recovery files are retained for safety. Do not delete them while using the installer.

Normal, EX and course records, records earned after installation, score backups, settings, songs and user skins are preserved.
Images you edited or deleted remain as you left them.

Unmodified installed images are restored to their original versions, or removed if they were added by the mod.

# Errors and help
Check that the game is closed, the folder is correct and enough disk space is available. If interrupted, run the same EXE and select the same folder to recover.
The installer stops without overwriting files if backups are damaged or the original version is unsupported. It requests administrator rights if write permission is insufficient.
This is an unsigned test installer. Automatic updates are not provided.

# Additional mod content
- Vertical play
- Viewport customization and skin support (using the added skins folder)
- EX score and result screen improvements
- HARD+APS
- Judge timing settings mode

# About skins
- Mod images are also added to imgs.
- Add skin images with matching filenames under the skins folder to apply them.

When reporting a problem on GitHub, include the build ID, Windows version, original KSM version, reproduction steps and error details.

Error log: [game folder]\log\session-<number>-pid<PID>\events-0.log
Last GUI operation result: %TEMP%/KsmEnhancedVerticalSetup-last-result.json
Licenses: [game folder]\mods\enhanced\ and [game folder]\mods\enhanced\licenses\

# Empty-folder cleanup after removal
After mod files have been removed successfully, folders recorded as newly added by this installation are removed only if empty, deepest first.
For older installations without directory ownership records, cleanup is limited to registered paths in mods/enhanced, mods/vertical and skin_preview. The shared parent mods is kept.
Folders containing files (including hidden files) or other folders are kept. Contents are never recursively deleted.
Pre-existing shared folders, songs, user skins, records, settings, logs, installation backups and recovery data are preserved.
If permissions or locks prevent cleanup, the folder is kept and a warning appears in the completion screen and last-operation JSON report.
If folder cleanup is interrupted, run the same installer and select the same folder to resume recovery.
