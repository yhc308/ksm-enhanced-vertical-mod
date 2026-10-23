$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$app=Join-Path $root 'kshootmania/App'
$out=Join-Path $root 'test-output/combo-v17'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$hash=(Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash
$chart=Join-Path $root 'test-output/spin-v13/Tornaido Test.kson'
$cases=@(
 @('neutral',''),@('tilt-minus10','--vertical-preview-tilt -10'),@('tilt-plus10','--vertical-preview-tilt 10'),
 @('shift-minus50','--vertical-preview-shift -50'),@('shift-plus50','--vertical-preview-shift 50'),
 @('top-plus100','--vertical-preview-zoom-top 100'),@('bottom-minus100','--vertical-preview-zoom-bottom -100'),
 @('bottom-plus100','--vertical-preview-zoom-bottom 100'),
 @('spin-quarter','--vertical-preview-event-rate .25'),@('spin-half','--vertical-preview-event-rate .5'),@('spin-threequarter','--vertical-preview-event-rate .75'),
 @('combined','--vertical-preview-tilt 10 --vertical-preview-zoom-top 100 --vertical-preview-zoom-bottom 100 --vertical-preview-shift 50'),
 @('neutral-1080','--vertical-preview-1080'),@('combined-1080','--vertical-preview-1080 --vertical-preview-tilt 10 --vertical-preview-zoom-top 100 --vertical-preview-zoom-bottom 100 --vertical-preview-shift 50'))
foreach($case in $cases) {
 $resolution=if($case[0].EndsWith('1080')){'--vertical-preview-1080'}else{'--vertical-preview-720'}
 $flags="--vertical-preview-hud --vertical-preview-fast --vertical-preview-zero $resolution --vertical-preview-bg night --vertical-preview-layer techno --vertical-preview-chart `"$chart`" --vertical-preview-event-rate 0 $($case[1])"
 $p=Start-Process -FilePath (Join-Path $app 'kshootmania.exe') -WorkingDirectory $app -ArgumentList $flags -WindowStyle Hidden -PassThru
 if(-not $p.WaitForExit(30000)){throw '체인 샘플 시간 초과'}
 if($p.ExitCode -ne 0){throw '체인 샘플 실패'}
 Copy-Item -LiteralPath (Join-Path $app 'Screenshot/vertical-hud-preview.png') -Destination (Join-Path $out ($case[0]+'.png'))
 Write-Output $case[0]
}
if($hash -ne (Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash){throw '설정 보존 실패'}
Write-Output 'PASS: 체인 고정 HUD 샘플 및 실제 설정 보존'
