Skin folders
============

Base game images are read from ../imgs/.
"default" in VIEWPORT CUSTOM selects those base images; do not create skins/default/.

Install a custom skin in its own folder here, for example:
  skins/My Skin/base.gif
  skins/My Skin/bg.jpg
  skins/My Skin/bg/...
  skins/My Skin/skin.json   (optional author and description metadata)

Keep image names and subfolder structure unchanged. Select the skin in VIEWPORT CUSTOM.
The existing Default / Yellow missing-image option still applies.

For skins installed by an older mod build, move the entire custom skin folder from
imgs/<skin name>/ to skins/<skin name>/. Old imgs skin folders are no longer listed.
Do not move the base imgs/bg, bg2, editor, gate, icon or jacket resource folders.
Settings and score files do not need to be moved.
