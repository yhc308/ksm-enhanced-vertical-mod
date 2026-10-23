param([ValidateSet('landscape','portrait')][string]$Group)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
Import-Module (Join-Path $root 'scripts/TestWorkspace.psm1') -Force
$python='python'
$cases=if($Group -eq 'landscape'){@(@(640,480),@(1280,720),@(1920,1080))}else{@(@(720,1280),@(1080,1920))}
$results=@()
foreach($size in $cases){
 $w=$size[0];$h=$size[1];$vertical=$h -gt $w
 foreach($palette in @(0,1)){
  $session=New-AsioFixture -Values @{vertical_enabled=[int]$vertical;windowsize="$w,$h";vertical_resolution="$w,$h";vertical_width=$w;vertical_height=$h;vsync=1;viewtiming=2;judge_color=$palette;timing_position=0;timing_ms_display=3;timing_ms_color=2;assisttick=2;automaticmute=0;currentdir='K-Shoot MANIA';currentitem=1;currentlevel=1} -Song
  $app=$session.appRoot;$chart=Join-Path $app 'songs/K-Shoot MANIA/flavorofyouth/flavorofyouth_ch.ksh'
  $before=@{};Get-Content -LiteralPath (Join-Path $app 'config.ini')|ForEach-Object {if($_ -match '^([^=]+)=(.*)$'){$before[$Matches[1]]=$Matches[2]}}
  Start-KsmOwnedProcess -Session $session -Arguments @('--timing-menu-capture') -TimeoutSeconds 30
  $after=@{};Get-Content -LiteralPath (Join-Path $app 'config.ini')|ForEach-Object {if($_ -match '^([^=]+)=(.*)$'){$after[$Matches[1]]=$Matches[2]}}
  foreach($key in @('judge_color','timing_position','timing_ms_display','timing_ms_color','viewtiming')){if($before[$key] -ne $after[$key]){throw "Menu capture changed $key"}}
  $flags=@('-auto','-from','0','--timing-hud-capture','--play-judge-indicator-fixture')
  if($vertical){$flags+='--indicator-both'}
  Start-KsmOwnedProcess -Session $session -Arguments ($flags+@($chart)) -TimeoutSeconds 40
  foreach($item in @(@('positive',0),@('negative',4),@('zero',5),@('large',6))){
   foreach($prefix in @('mini','indicator')){Copy-Item -LiteralPath (Join-Path $app "timing-captures/hud-$($item[1]).png") -Destination (Join-Path $app "$prefix-$($item[0]).png")}
  }
  Copy-Item -LiteralPath (Join-Path $app 'timing-captures/hud-8.png') -Destination (Join-Path $app 'indicator-live.png')
  & $python (Join-Path $root 'tests/MiniJudgeIndicatorVerifier.py') $app $w $h
  if($LASTEXITCODE){throw "Mini verification failed $w/$palette"}
  if($vertical){
   & $python (Join-Path $root 'tests/PlayJudgeIndicatorVerifier.py') $app $w
   if($LASTEXITCODE){throw "Bottom verification failed $w/$palette"}
  }
  Start-KsmOwnedProcess -Session $session -Arguments @('--result-detail-capture') -TimeoutSeconds 30
  & $python (Join-Path $root 'tests/JudgeColorVerifier.py') $app $w $h $palette
  if($LASTEXITCODE){throw "Judge color verification failed $w/$palette"}
  $dest=Join-Path $root "reference/judge-color/$w`x$h/$palette"
  New-Item -ItemType Directory -Path $dest -Force|Out-Null
  foreach($n in @('menu-0','menu-1','menu-2','hud-0','hud-4','hud-8')){Copy-Item -LiteralPath (Join-Path $app "timing-captures/$n.png") -Destination (Join-Path $dest "$n.png")}
  Copy-Item -LiteralPath (Join-Path $app 'result-detail.png') -Destination (Join-Path $dest 'result.png')
  $results+=[pscustomobject]@{status='PASS';width=$w;height=$h;palette=$palette;session=$session.root;captures=$dest}
  $results|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $root "build/judge-color-$Group-tests.json") -Encoding UTF8
  Write-Output "JUDGE COLOR $w`x$h palette=$palette PASS"
 }
}
