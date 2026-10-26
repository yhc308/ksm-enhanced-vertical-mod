param([int[]]$Widths=@(720,1080,1280))
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
Import-Module (Join-Path $root 'scripts/TestWorkspace.psm1') -Force
$python='python'
$results=@()
foreach($w in $Widths){
 $vertical=$w -ne 1280;$h=if($vertical){[int]($w*16/9)}else{720}
 $session=New-AsioFixture -Values @{vertical_enabled=[int]$vertical;windowsize="$w,$h";vertical_resolution="$w,$h";vertical_width=$w;vertical_height=$h;vsync=1;viewtiming=2;timing_ms_display=3;timing_ms_color=2;automaticmute=0} -Song
 if($w -eq 720){
  $report=Invoke-AsioFixture -Session $session -Flags @('--timing-hud-smoke-test') -Report 'timing-hud-test.txt' -Expected 'TOTAL \d+ checks passed'
  if(Test-Path -LiteralPath (Join-Path $session.appRoot 'timing-hud-failure.txt')){throw (Get-Content -LiteralPath (Join-Path $session.appRoot 'timing-hud-failure.txt') -Raw)}
  Write-Output (Get-Content -LiteralPath $report -Tail 1)
 }
 $chart=Join-Path $session.appRoot 'songs/K-Shoot MANIA/flavorofyouth/flavorofyouth_ch.ksh'
 Start-KsmOwnedProcess -Session $session -Arguments @('-auto','-from','0','--timing-hud-capture',$chart) -TimeoutSeconds 50
 $live=Join-Path $session.appRoot 'indicator-live.png'
 Copy-Item -LiteralPath (Join-Path $session.appRoot 'timing-captures/hud-8.png') -Destination $live
 Copy-Item -LiteralPath $live -Destination (Join-Path $root "reference/play_judge_indicator_$w.png") -Force
 if($vertical){
  Start-KsmOwnedProcess -Session $session -Arguments @('-auto','-from','0','--timing-hud-capture','--play-judge-indicator-fixture',$chart) -TimeoutSeconds 50
  foreach($pair in @(@('positive',0),@('negative',4),@('zero',5),@('large',6))){
   $dest=Join-Path $session.appRoot "indicator-$($pair[0]).png"
   Copy-Item -LiteralPath (Join-Path $session.appRoot "timing-captures/hud-$($pair[1]).png") -Destination $dest
   Copy-Item -LiteralPath $dest -Destination (Join-Path $root "reference/play_judge_indicator_$($pair[0])_$w.png") -Force
  }
 }
 & $python (Join-Path $root 'tests/PlayJudgeIndicatorVerifier.py') $session.appRoot $w
 if($LASTEXITCODE){throw "Indicator visual verification failed: $($session.root)"}
 $results+=[pscustomobject]@{width=$w;height=$h;session=$session.root;capture=(Join-Path $root "reference/play_judge_indicator_$w.png");status='PASS'}
 $results|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $root 'build/play-judge-indicator-tests.json') -Encoding UTF8
 Write-Output "INDICATOR $w`x$h PASS"
}
