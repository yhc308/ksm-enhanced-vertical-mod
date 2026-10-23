param([switch]$SamplesOnly)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$workspace=Split-Path $root -Parent
$app=Join-Path $root 'kshootmania/App'
$out=Join-Path $root 'test-output/guide-v16'
$public=Join-Path $workspace '기어_투시조정_v16_샘플/night_techno'
New-Item -ItemType Directory -Force -Path $out,$public | Out-Null
$hash=(Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash
function Capture([string]$dest,[string]$extra) {
 $flags="--vertical-preview-hud --vertical-preview-fast --vertical-preview-zero --vertical-preview-bg night --vertical-preview-layer techno --vertical-preview-geometry $extra"
 $p=Start-Process -FilePath (Join-Path $app 'kshootmania.exe') -WorkingDirectory $app -ArgumentList $flags -WindowStyle Hidden -PassThru
 if(-not $p.WaitForExit(30000)){ $p.Kill();throw "샘플 시간 초과: $dest" }
 if($p.ExitCode -ne 0){throw "샘플 실패: $dest"}
 Copy-Item -LiteralPath (Join-Path $app 'Screenshot/vertical-hud-preview.png') -Destination $dest
 Copy-Item -LiteralPath (Join-Path $app 'vertical-warp-preview.csv') -Destination ([IO.Path]::ChangeExtension($dest,'.csv'))
 Copy-Item -LiteralPath (Join-Path $app 'vertical-lane-preview-geometry.csv') -Destination ([IO.Path]::ChangeExtension($dest,'.geometry.csv'))
 Write-Output ([IO.Path]::GetFileName($dest))
}
Capture (Join-Path $out 'neutral-720.png') '--vertical-preview-720'
$chart=Join-Path $workspace 'kshootmania_v2.1.0_win/kshootmania_v2/songs/K-Shoot MANIA/jurer/jurer_ex.ksh'
$meta="--vertical-preview-1080 --vertical-preview-score 0 --vertical-preview-hispeed 700 --vertical-preview-chart `"$chart`""
$cases=@(
 @('01_중립',''),@('02_기울기_-10도','--vertical-preview-tilt -10'),@('03_기울기_-5도','--vertical-preview-tilt -5'),
 @('04_기울기_+5도','--vertical-preview-tilt 5'),@('05_기울기_+10도','--vertical-preview-tilt 10'),
 @('06_상단_-100','--vertical-preview-zoom-top -100'),@('07_상단_+100','--vertical-preview-zoom-top 100'),
 @('08_하단_-100','--vertical-preview-zoom-bottom -100'),@('09_하단_+100','--vertical-preview-zoom-bottom 100'))
foreach($case in $cases){Capture (Join-Path $public ($case[0]+'.png')) "$meta $($case[1])"}
$dir=Join-Path $out 'compound';New-Item -ItemType Directory -Force -Path $dir | Out-Null
foreach($shift in @(-50,50)) {
 foreach($top in @(-100,100)){foreach($bottom in @(-100,100)){
  Capture (Join-Path $dir "shift$shift-top$top-bottom$bottom.png") "--vertical-preview-shift $shift --vertical-preview-zoom-top $top --vertical-preview-zoom-bottom $bottom --vertical-preview-tilt 10"
 }}
}
if(-not $SamplesOnly) {
foreach($theme in @('arrow','techno','wave','smoke','snow','sakura','hidden')) {
 $dir=Join-Path $out "themes/$theme";New-Item -ItemType Directory -Force -Path $dir | Out-Null
 foreach($bottom in @(-100,0,100)){
  foreach($mode in @('on','off','bypass')) {
   $flags="--vertical-preview-layer $theme --vertical-preview-zoom-bottom $bottom"
   if($mode -eq 'off'){$flags+=' --vertical-preview-depth-off'}
   if($mode -eq 'bypass'){$flags+=' --vertical-preview-warp-off'}
   Capture (Join-Path $dir "bottom$bottom-$mode.png") $flags
  }
 }
}
}
if((Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash -ne $hash){throw '미리보기 설정 보존 실패'}
Write-Output 'PASS: 샘플 생성 및 설정 파일 보존'
