# KSM Enhanced Vertical Mod (Windows x64 only)

**An unofficial mod that brings vertical play to K-Shoot Mania 2.1.0.**
Features include vertical play, skin presets, viewport customization, EX scoring, HARD+APS, judgment timing settings and partial ASIO support.

[Download](https://github.com/yhc308/ksm-enhanced-vertical-mod/releases/tag/v0.5.1-beta1) · [Source structure and build guide](https://github.com/yhc308/ksm-enhanced-vertical-mod/blob/release/0.5.1-beta1/overlay/REVIEW.md) · [Full source](https://github.com/yhc308/ksm-enhanced-vertical/tree/fe5afb4ca7c375f98cbce7591b70bae477d1004b) · [Report an issue](https://github.com/yhc308/ksm-enhanced-vertical-mod/issues)

## Features

### Vertical play

Play with lane and HUD layouts adapted for portrait displays. Landscape play is also supported, and display settings can be adjusted in Vertical Option.
The UI assets and additional features designed for vertical play also support landscape mode.

### Judgment counters

Judgment indicators are available in both landscape and portrait modes, with layouts suited to each orientation.

### P-CRITICAL and EX scoring

Adds the P-CRITICAL judgment, along with EX scoring, result displays and additional record saving. EX records are stored separately from the original game records, and include the chart's theoretical maximum EX score.

### Viewport customization / user skins

Adjust background brightness, lane and note width, judgment indicators, skins and more while viewing an in-game preview.

Default images are loaded from `imgs/`, and user skins from `skins/<skin name>/`. Select a skin in Viewport Custom and check it in the preview.
Missing skin images can fall back to the default images in `imgs/`, so you do not need to replace every image.

### HARD+APS

APS switches to NORMAL when the HARD gauge fails. The gauge and percentage display transition together, with a transition effect on the APS banner.

### Judgment timing settings

Adjust judgment and display settings while playing a dedicated test chart on repeat.

## Reporting issues

Please [open an issue](https://github.com/yhc308/ksm-enhanced-vertical-mod/issues) with your Windows version, game and mod versions, and steps to reproduce the problem. Screenshots or error logs are also helpful.

- Game logs: `<game folder>/log/session-<number>-pid<PID>/events-0.log`
- Installer report: `%TEMP%/KsmEnhancedVerticalSetup-last-result.json`

## Credits and license

Developed in collaboration with Codex. All images and audio used in the project were created by hand.

Original game: [K-Shoot Mania](https://www.kshootmania.com/) / [Official source](https://github.com/kshootmania/ksm-v2)

- Special thanks: [KSM's developer](https://x.com/masaka_k)
- UI skin design and mod contributions: [chesia_game](https://x.com/chesia_game)
- Music for the preview and judgment timing settings: [Hyung](https://open.spotify.com/artist/2soZF7u1mCALXnpwCbyGvh?si=7gvRzWqdTeqV2lEYtfmo5w)

This is an unofficial mod. Original copyright notices and licenses are retained.
