$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$app=Join-Path $root 'kshootmania/App'
$out=Join-Path $root 'test-output/lane-anchor-v2/after'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$before=(Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash
foreach($percent in @(-100,-75,-50,-25,0,25,50,75,100)) {
 $degrees=$percent/10.0
 $flags="--vertical-preview-hud --vertical-preview-zero --vertical-preview-bg night --vertical-preview-layer arrow --vertical-preview-tilt $degrees --vertical-preview-geometry"
 $p=Start-Process (Join-Path $app 'kshootmania.exe') -WorkingDirectory $app -ArgumentList $flags -WindowStyle Hidden -PassThru
 if(-not $p.WaitForExit(20000)){throw 'Preview timed out'}
 if($p.ExitCode -ne 0){throw 'Preview failed'}
 Copy-Item -LiteralPath (Join-Path $app 'Screenshot/vertical-hud-preview.png') -Destination (Join-Path $out "laser-tilt-$percent.png")
 Copy-Item -LiteralPath (Join-Path $app 'vertical-lane-preview-geometry.csv') -Destination (Join-Path $out "geometry-$percent.csv")
 Write-Output "Captured $percent% ($degrees degrees)"
}
foreach($case in @('effects','legacy','wide','static','zoom-top','zoom-bottom','spin')) {
 $flags='--vertical-preview-hud --vertical-preview-zero --vertical-preview-bg night --vertical-preview-layer techno --vertical-preview-tilt 10 --vertical-preview-geometry'
 if($case -eq 'static'){$flags+=' --vertical-preview-layer-static'}
 if($case -in @('effects','legacy')){$flags+=' --vertical-preview-motion --vertical-preview-effects --vertical-preview-wide'}
 if($case -eq 'wide'){$flags+=' --vertical-preview-wide'}
 if($case -eq 'zoom-top'){$flags+=' --vertical-preview-zoom-top 100 --vertical-preview-effects'}
 if($case -eq 'zoom-bottom'){$flags+=' --vertical-preview-zoom-bottom 100 --vertical-preview-effects'}
 if($case -eq 'spin'){$flags+=' --vertical-preview-spin 90 --vertical-preview-wide --vertical-preview-effects'}
 if($case -eq 'legacy'){$flags+=' --vertical-preview-legacy --vertical-preview-zoom-top 1.333333333 --vertical-preview-zoom-bottom -100'}
 $p=Start-Process (Join-Path $app 'kshootmania.exe') -WorkingDirectory $app -ArgumentList $flags -WindowStyle Hidden -PassThru
 if(-not $p.WaitForExit(20000)){throw 'Preview timed out'}
 if($p.ExitCode -ne 0){throw 'Preview failed'}
 Copy-Item -LiteralPath (Join-Path $app 'Screenshot/vertical-hud-preview.png') -Destination (Join-Path $out "$case.png")
 Copy-Item -LiteralPath (Join-Path $app 'vertical-lane-preview-geometry.csv') -Destination (Join-Path $out "geometry-$case.csv")
 Write-Output "Captured $case"
}
if((Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash -ne $before){throw 'Preview changed config'}
Write-Output 'PASS: 16 previews, configuration preserved'
