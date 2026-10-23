$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$app=Join-Path $root 'kshootmania/App'
$out=Join-Path $root 'test-output/tunnel-v4'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$hash=(Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash
function Capture([string]$name,[string]$extra) {
 $flags="--vertical-preview-hud --vertical-preview-zero --vertical-preview-bg night --vertical-preview-layer arrow $extra"
 $p=Start-Process (Join-Path $app 'kshootmania.exe') -WorkingDirectory $app -ArgumentList $flags -WindowStyle Hidden -PassThru
 if(-not $p.WaitForExit(30000)){ $p.Kill();throw "Preview timed out: $name" }
 if($p.ExitCode -ne 0){throw "Preview failed: $name"}
 Copy-Item -LiteralPath (Join-Path $app 'Screenshot/vertical-hud-preview.png') -Destination (Join-Path $out "$name.png")
 Copy-Item -LiteralPath (Join-Path $app 'vertical-warp-preview.csv') -Destination (Join-Path $out "$name.csv")
 Write-Output "Captured $name"
}
foreach($zoom in @(-100,0,100)) {
 Capture "background-$zoom" "--vertical-preview-background-only --vertical-preview-zoom-top $zoom"
 Capture "mesh-$zoom" "--vertical-preview-background-only --vertical-preview-warp-mesh --vertical-preview-zoom-top $zoom"
 Capture "uv-$zoom" "--vertical-preview-background-only --vertical-preview-warp-uv --vertical-preview-warp-mesh --vertical-preview-zoom-top $zoom"
}
Capture 'gpu' '--vertical-preview-zoom-top 100 --vertical-preview-layer-static'
Capture 'cpu' '--vertical-preview-zoom-top 100 --vertical-preview-layer-static --vertical-preview-warp-cpu'
Capture 'off' '--vertical-preview-zoom-top 100 --vertical-preview-layer-static --vertical-preview-warp-off'
foreach($percent in @(-100,-75,-50,-25,0,25,50,75,100)) {
 $degrees=$percent/10.0
 Capture "laser-tilt-$percent" "--vertical-preview-tilt $degrees"
}
foreach($theme in @('cloudy','sky','cyber','smoke')) {
 Capture "theme-$theme-zero" "--vertical-preview-bg $theme --vertical-preview-layer techno --vertical-preview-zoom-top 100"
}
foreach($layer in @('arrow','techno','hidden','sakura','smoke','snow','wave')) {
 Capture "layer-$layer" "--vertical-preview-layer $layer --vertical-preview-background-only --vertical-preview-zoom-top 100"
}
foreach($extra in @('--vertical-preview-hundred','--vertical-preview-hard','--vertical-preview-course','--vertical-preview-wide','--vertical-preview-effects','--vertical-preview-legacy --vertical-preview-motion --vertical-preview-zoom-top 1.333333333 --vertical-preview-zoom-bottom -100','--vertical-preview-spin 90 --vertical-preview-tilt 10 --vertical-preview-zoom-top 100','--vertical-preview-zoom-top -800')) {
 $name='case-'+($extra -replace '[^a-zA-Z0-9]','_')
 Capture $name $extra
}
Capture 'motion' '--vertical-preview-background-only --vertical-preview-warp-cycle --vertical-preview-sequence'
Get-ChildItem -LiteralPath (Join-Path $app 'Screenshot') -Filter 'warp-sequence-*.png' | Copy-Item -Destination $out
if((Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash -ne $hash){throw 'Preview changed configuration'}
Write-Output 'PASS: configuration preserved'
