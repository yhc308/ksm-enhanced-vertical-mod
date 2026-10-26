$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$app=Join-Path $root 'kshootmania/App'
$out=Join-Path $root 'test-output/wall-v10'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$hash=(Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash
function Capture([string]$dir,[string]$theme,[string]$name,[string]$extra,[string]$mode='hud') {
 $flags="--vertical-preview-$mode --vertical-preview-zero --vertical-preview-bg night --vertical-preview-layer $theme $extra"
 if($name -ne 'motion' -and $mode -ne 'settings'){$flags+=' --vertical-preview-fast'}
 $p=Start-Process (Join-Path $app 'kshootmania.exe') -WorkingDirectory $app -ArgumentList $flags -WindowStyle Hidden -PassThru
 if(-not $p.WaitForExit(30000)){ $p.Kill();throw "Preview timed out: $theme $name" }
 if($p.ExitCode -ne 0){throw "Preview failed: $theme $name"}
 $file=if($mode -eq 'options'){'vertical-options-preview.png'}elseif($mode -eq 'settings'){'vertical-settings-preview.png'}elseif($mode -eq 'title'){'vertical-title-preview.png'}else{'vertical-hud-preview.png'}
 Copy-Item -LiteralPath (Join-Path $app "Screenshot/$file") -Destination (Join-Path $dir "$name.png")
 if($mode -eq 'hud'){Copy-Item -LiteralPath (Join-Path $app 'vertical-warp-preview.csv') -Destination (Join-Path $dir "$name.csv")}
 Write-Output "Captured $theme $name"
}
foreach($theme in @('arrow','smoke','sakura','techno')) {
 $dir=Join-Path $out $theme
 foreach($v in @(-100,0,100)){Capture $dir $theme "knob-$v" "--vertical-preview-tilt $($v/10.0)"}
 foreach($group in @('top','bottom')){foreach($v in @(-100,0,100)){
  if($v -eq 0){Copy-Item (Join-Path $dir 'knob-0.png') (Join-Path $dir "$group-0.png");Copy-Item (Join-Path $dir 'knob-0.csv') (Join-Path $dir "$group-0.csv")}
  else{Capture $dir $theme "$group-$v" "--vertical-preview-zoom-$group $v"}
 }}
 & 'python' (Join-Path $root 'scripts/BuildWallSampleGallery.py') $dir
}
$dir=Join-Path $out 'diagnostics'
foreach($v in @(-100,0,100)){Capture $dir 'arrow' "uv-bottom-$v" "--vertical-preview-zoom-bottom $v --vertical-preview-background-only --vertical-preview-warp-mesh --vertical-preview-warp-uv"}
Capture $dir 'arrow' 'gpu' '--vertical-preview-zoom-top 50 --vertical-preview-zoom-bottom 50 --vertical-preview-tilt 10 --vertical-preview-layer-static'
Capture $dir 'arrow' 'cpu' '--vertical-preview-zoom-top 50 --vertical-preview-zoom-bottom 50 --vertical-preview-tilt 10 --vertical-preview-layer-static --vertical-preview-warp-cpu'
Capture $dir 'arrow' 'off' '--vertical-preview-zoom-top 50 --vertical-preview-zoom-bottom 50 --vertical-preview-tilt 10 --vertical-preview-layer-static --vertical-preview-warp-off'
foreach($theme in @('wave','snow','hidden')){Capture $dir $theme "$theme-thin" '--vertical-preview-zoom-top 50 --vertical-preview-zoom-bottom -100 --vertical-preview-tilt 10'}
foreach($theme in @('arrow','techno')){Capture $dir $theme "$theme-alternate" '--vertical-preview-full --vertical-preview-zoom-bottom -100 --vertical-preview-tilt 10'}
Capture $dir '../../test-output/tunnel-v4/custom-wall.gif' 'custom' '--vertical-preview-zoom-bottom -100 --vertical-preview-tilt 10'
Capture $dir 'arrow' 'legacy' '--vertical-preview-motion --vertical-preview-legacy --vertical-preview-zoom-top 1.333333333 --vertical-preview-zoom-bottom -100 --vertical-preview-wide --vertical-preview-effects'
Capture $dir 'arrow' 'spin' '--vertical-preview-zoom-top 50 --vertical-preview-zoom-bottom 50 --vertical-preview-spin 90 --vertical-preview-tilt 10'
Capture $dir 'arrow' 'wide' '--vertical-preview-wide --vertical-preview-effects'
Capture $dir 'arrow' 'shift' '--vertical-preview-shift 20 --vertical-preview-tilt 10'
if((Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash -ne $hash){throw 'Config changed'}
(Get-FileHash -LiteralPath (Join-Path $app 'kshootmania.exe')).Hash | Set-Content -LiteralPath (Join-Path $out 'executable-sha256.txt')
Write-Output 'PASS final screenshots and supplemental cases'
