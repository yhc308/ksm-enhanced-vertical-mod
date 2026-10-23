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
 $dir=Join-Path $out $theme;New-Item -ItemType Directory -Force -Path $dir | Out-Null
 foreach($v in @(-100,-75,-50,-25,0,25,50,75,100)){Capture $dir $theme "knob-$v" "--vertical-preview-tilt $($v/10.0)"}
 foreach($group in @('top','bottom')) {foreach($v in @(-100,-50,0,50,100)) {
  if($v -eq 0){Copy-Item -LiteralPath (Join-Path $dir 'knob-0.png') -Destination (Join-Path $dir "$group-0.png");Copy-Item -LiteralPath (Join-Path $dir 'knob-0.csv') -Destination (Join-Path $dir "$group-0.csv")}
  else {Capture $dir $theme "$group-$v" "--vertical-preview-zoom-$group $v"}
 }}
 foreach($top in @(-100,100)){foreach($bottom in @(-100,100)) {Capture $dir $theme "combined-$top-$bottom" "--vertical-preview-zoom-top $top --vertical-preview-zoom-bottom $bottom --vertical-preview-tilt 10"}}
 Capture $dir $theme 'hud-approach' '--vertical-preview-zoom-top 300'
 & 'python' (Join-Path $root 'scripts/BuildWallSampleGallery.py') $dir
 if($LASTEXITCODE -ne 0){throw 'Gallery failed'}
 Write-Output "THEME COMPLETE: $theme"
}
$dir=Join-Path $out 'diagnostics';New-Item -ItemType Directory -Force -Path $dir | Out-Null
foreach($v in @(-100,0,100)) {Capture $dir 'arrow' "mesh-top-$v" "--vertical-preview-zoom-top $v --vertical-preview-background-only --vertical-preview-warp-mesh";Capture $dir 'arrow' "uv-bottom-$v" "--vertical-preview-zoom-bottom $v --vertical-preview-background-only --vertical-preview-warp-mesh --vertical-preview-warp-uv"}
Capture $dir 'arrow' 'gpu' '--vertical-preview-zoom-top 50 --vertical-preview-zoom-bottom 50 --vertical-preview-tilt 10 --vertical-preview-layer-static'
Capture $dir 'arrow' 'cpu' '--vertical-preview-zoom-top 50 --vertical-preview-zoom-bottom 50 --vertical-preview-tilt 10 --vertical-preview-layer-static --vertical-preview-warp-cpu'
Capture $dir 'arrow' 'off' '--vertical-preview-zoom-top 50 --vertical-preview-zoom-bottom 50 --vertical-preview-tilt 10 --vertical-preview-layer-static --vertical-preview-warp-off'
if((Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash -ne $hash){throw 'Preview changed configuration'}
(Get-FileHash -LiteralPath (Join-Path $app 'kshootmania.exe')).Hash | Set-Content -LiteralPath (Join-Path $out 'executable-sha256.txt')
Write-Output 'PASS: four themes and mesh diagnostics; configuration preserved'
