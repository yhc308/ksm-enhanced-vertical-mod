$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$app=Join-Path $root 'kshootmania/App'
$out=Join-Path $root 'test-output/wall-v11/coverage'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$hash=(Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash
foreach($angle in @(-10,0,10)){foreach($top in @(-100,0,100)){foreach($bottom in @(-100,0,100)){
 $name="a$angle-t$top-b$bottom"
 $flags="--vertical-preview-hud --vertical-preview-fast --vertical-preview-zero --vertical-preview-bg night --vertical-preview-layer arrow --vertical-preview-tilt $angle --vertical-preview-zoom-top $top --vertical-preview-zoom-bottom $bottom"
 $p=Start-Process (Join-Path $app 'kshootmania.exe') -WorkingDirectory $app -ArgumentList $flags -WindowStyle Hidden -PassThru
 if(-not $p.WaitForExit(30000)){ $p.Kill();throw 'Coverage preview timeout' }
 if($p.ExitCode -ne 0){throw 'Coverage preview failed'}
 Copy-Item -LiteralPath (Join-Path $app 'Screenshot/vertical-hud-preview.png') -Destination (Join-Path $out "$name.png")
 Copy-Item -LiteralPath (Join-Path $app 'vertical-warp-preview.csv') -Destination (Join-Path $out "$name.csv")
 Write-Output "Coverage $name"
}}}
if((Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash -ne $hash){throw 'Configuration changed'}
Write-Output 'PASS: 27 compound states; config preserved'
