$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$app=Join-Path $root 'kshootmania/App'
$out=Join-Path $root 'test-output/wall-v11'
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
 foreach($v in @(-100,-50,0,50,100)){Capture $dir $theme "bottom-$v" "--vertical-preview-zoom-bottom $v"}
 foreach($angle in @(-10,10)){foreach($v in @(-100,100)){Capture $dir $theme "tilt-$angle-bottom-$v" "--vertical-preview-tilt $angle --vertical-preview-zoom-bottom $v"}}
}
$dir=Join-Path $out 'diagnostics';New-Item -ItemType Directory -Force -Path $dir | Out-Null
foreach($v in @(-100,0,100)){Capture $dir 'arrow' "uv-bottom-$v" "--vertical-preview-zoom-bottom $v --vertical-preview-background-only --vertical-preview-warp-mesh --vertical-preview-warp-uv"}
Capture $dir 'arrow' 'gpu' '--vertical-preview-zoom-bottom 50 --vertical-preview-tilt 10 --vertical-preview-layer-static'
Capture $dir 'arrow' 'cpu' '--vertical-preview-zoom-bottom 50 --vertical-preview-tilt 10 --vertical-preview-layer-static --vertical-preview-warp-cpu'
Capture $dir 'arrow' 'off' '--vertical-preview-zoom-bottom 50 --vertical-preview-tilt 10 --vertical-preview-layer-static --vertical-preview-warp-off'
if((Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash -ne $hash){throw 'Configuration changed'}
(Get-FileHash -LiteralPath (Join-Path $app 'kshootmania.exe')).Hash | Set-Content -LiteralPath (Join-Path $out 'executable-sha256.txt')
Write-Output 'PASS lower-response capture; config preserved'
