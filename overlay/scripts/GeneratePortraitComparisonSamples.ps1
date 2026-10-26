$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$workspace=Split-Path $root -Parent
$app=Join-Path $root 'kshootmania/App'
$out=Join-Path $workspace '버티컬_인게임_각도비교_샘플_20261017'
if(Test-Path -LiteralPath $out){throw '같은 이름의 샘플 폴더가 이미 있습니다. 기존 결과는 덮어쓰지 않습니다.'}
$measure=Join-Path $out '측정자료'
New-Item -ItemType Directory -Force -Path $out,$measure | Out-Null
$chart=Join-Path $workspace 'vertical_play_test/songs/K-Shoot MANIA/jurer/jurer_ex.ksh'
$spin=Join-Path $root 'test-output/spin-v13/Tornaido Test.kson'
$sourceHash=(Get-FileHash -LiteralPath (Join-Path $app 'kshootmania.exe')).Hash
$configHash=(Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash
$playHash=(Get-FileHash -LiteralPath (Join-Path $workspace 'vertical_play_test/config.ini')).Hash
$cases=[System.Collections.Generic.List[object]]::new()
function Add-Case([string]$folder,[string]$name,[string]$label,[string]$flags,[string]$kind='outline') {
 $cases.Add([pscustomobject]@{folder=$folder;name=$name;label=$label;flags=$flags;kind=$kind})
}
Add-Case '00_기본_인게임' '01_디폴트_기어' '디폴트 · 외곽 비교용' ''
Add-Case '00_기본_인게임' '02_노트와_레이저' '노트·레이저 표시' '' 'fixture'
Add-Case '00_기본_인게임' '03_판정효과와_와이드' '판정 효과·와이드 레이저' '--vertical-preview-effects --vertical-preview-wide' 'fixture'
Add-Case '00_기본_인게임' '04_고정_체인HUD' '고정 체인 HUD' '--vertical-preview-event-rate 0' 'spin'
$values=@(-100,-75,-50,-25,0,25,50,75,100)
$i=0
foreach($v in $values){$i++;$angle=$v/10.0;$sign=if($v -gt 0){'+'}else{''};$a=$angle.ToString('0.##',[Globalization.CultureInfo]::InvariantCulture);Add-Case '01_노브_기울기' ('{0:D2}_기울기_{1}{2}도' -f $i,$sign,$a) ("노브 $sign$v% / $sign$a 도") "--vertical-preview-tilt $a"}
foreach($part in @('top','bottom')) {
 $folder=if($part -eq 'top'){'02_상단_확대축소'}else{'03_하단_확대축소'}
 $title=if($part -eq 'top'){'상단'}else{'하단'};$i=0
 foreach($v in $values){$i++;$sign=if($v -gt 0){'+'}else{''};Add-Case $folder ('{0:D2}_{1}_{2}{3}' -f $i,$title,$sign,$v) ("$title $sign$v") "--vertical-preview-zoom-$part $v"}
}
$i=0
foreach($v in @(-50,-25,0,25,50)){$i++;$sign=if($v -gt 0){'+'}else{''};Add-Case '04_좌우_이동' ('{0:D2}_좌우이동_{1}{2}' -f $i,$sign,$v) ("좌우 이동 $sign$v") "--vertical-preview-shift $v"}
$i=0
foreach($top in @(-100,100)){foreach($bottom in @(-100,100)){$i++;Add-Case '05_복합_변형' ('{0:D2}_상단_{1}_하단_{2}_기울기_10도' -f $i,$top,$bottom) ("상단 $top / 하단 $bottom / +10도") "--vertical-preview-zoom-top $top --vertical-preview-zoom-bottom $bottom --vertical-preview-tilt 10"}}
foreach($event in @(0,1)){foreach($phase in @(.25,.5,.75)){$percent=[int]($phase*100);Add-Case '06_원본_회전연출' ('이벤트{0:D2}_진행_{1:D2}퍼센트' -f ($event+1),$percent) ("회전 이벤트 $($event+1) / $percent%") "--vertical-preview-event-index $event --vertical-preview-event-rate $phase" 'spin'}}
$index=0
foreach($case in $cases) {
 $index++;$dir=Join-Path $out $case.folder;New-Item -ItemType Directory -Force -Path $dir | Out-Null
 $flags='--vertical-preview-hud --vertical-preview-fast --vertical-preview-1080 --vertical-preview-zero --vertical-preview-score 0 --vertical-preview-hispeed 700 --vertical-preview-bg night --vertical-preview-layer techno --vertical-preview-geometry'
 if($case.kind -eq 'outline'){$flags+=" --vertical-preview-chart `"$chart`""}
 if($case.kind -eq 'spin'){$flags+=" --vertical-preview-chart `"$spin`""}
 $flags+=' '+$case.flags
 $before=Get-Date
 $p=Start-Process -FilePath (Join-Path $app 'kshootmania.exe') -WorkingDirectory $app -ArgumentList $flags -WindowStyle Hidden -PassThru
 if(-not $p.WaitForExit(30000)){$p.Kill();throw "샘플 시간 초과: $($case.name)"}
 $p.Refresh();if($p.ExitCode -ne 0){throw "샘플 실행 실패: $($case.name)"}
 $image=Join-Path $app 'Screenshot/vertical-hud-preview.png'
 if((Get-Item -LiteralPath $image).LastWriteTime -lt $before.AddSeconds(-1)){throw '새 캡처가 생성되지 않았습니다.'}
 Copy-Item -LiteralPath $image -Destination (Join-Path $dir ($case.name+'.png'))
 $dataDir=Join-Path $measure $case.folder;New-Item -ItemType Directory -Force -Path $dataDir | Out-Null
 Copy-Item -LiteralPath (Join-Path $app 'vertical-lane-preview-geometry.csv') -Destination (Join-Path $dataDir ($case.name+'.기어.csv'))
 Copy-Item -LiteralPath (Join-Path $app 'vertical-warp-preview.csv') -Destination (Join-Path $dataDir ($case.name+'.배경.csv'))
 Write-Output "[$index/$($cases.Count)] $($case.folder) / $($case.name)"
}
if($configHash -ne (Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash){throw '개발본 설정 파일이 변경됐습니다.'}
if($playHash -ne (Get-FileHash -LiteralPath (Join-Path $workspace 'vertical_play_test/config.ini')).Hash){throw '테스트 게임 설정 파일이 변경됐습니다.'}
if($sourceHash -ne (Get-FileHash -LiteralPath (Join-Path $app 'kshootmania.exe')).Hash){throw '샘플 생성 도중 실행 파일이 변경됐습니다.'}
[pscustomobject]@{date='2026-10-17';resolution=@(1080,1920);background='night';wall='techno';gauge=0;score=0;hispeed=700;executable_sha256=$sourceHash;configuration_preserved=$true;cases=$cases} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $measure '샘플목록.json') -Encoding utf8
Write-Output 'PASS: 각도별 원본 PNG 생성 및 게임 설정·실행 파일 보존'
