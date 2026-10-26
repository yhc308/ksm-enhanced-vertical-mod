param([int[]]$Widths=@(1280,1920,640,720,1080))
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
Import-Module (Join-Path $root 'scripts/TestWorkspace.psm1') -Force
$python='python'
$results=@()
foreach($size in @(@(1280,720),@(1920,1080),@(640,480),@(720,1280),@(1080,1920))){
 $w=$size[0];$h=$size[1];$vertical=$h -gt $w
 if($w -notin $Widths){continue}
 $session=New-AsioFixture -Values @{vertical_enabled=[int]$vertical;windowsize="$w,$h";vertical_resolution="$w,$h";vertical_width=$w;vertical_height=$h;vsync=1} -Song
 $chart=Join-Path $session.appRoot 'songs/K-Shoot MANIA/flavorofyouth/flavorofyouth_ch.ksh'
 $base=@('-auto','-from','0','--timing-hud-capture','--play-judge-indicator-fixture')
 $flags=@($base);if($vertical){$flags+='--indicator-both'}
 Start-KsmOwnedProcess -Session $session -Arguments ($flags+@($chart)) -TimeoutSeconds 50
 foreach($item in @(@('positive',0),@('negative',4),@('zero',5),@('large',6))){
  Copy-Item -LiteralPath (Join-Path $session.appRoot "timing-captures/hud-$($item[1]).png") -Destination (Join-Path $session.appRoot "mini-$($item[0]).png") -Force
 }
 Copy-Item -LiteralPath (Join-Path $session.appRoot 'mini-positive.png') -Destination (Join-Path $root "reference/mini-indicator-$w`x$h.png") -Force
 if($w -eq 640 -or $w -eq 720){
  Start-KsmOwnedProcess -Session $session -Arguments ($base+@('--indicator-off',$chart)) -TimeoutSeconds 50
  Copy-Item -LiteralPath (Join-Path $session.appRoot 'timing-captures/hud-0.png') -Destination (Join-Path $session.appRoot 'mini-off.png') -Force
 }
 if($w -eq 720){
  Start-KsmOwnedProcess -Session $session -Arguments ($base+@($chart)) -TimeoutSeconds 50
  Copy-Item -LiteralPath (Join-Path $session.appRoot 'timing-captures/hud-0.png') -Destination (Join-Path $session.appRoot 'mini-bottom-only.png') -Force
 }
 if($w -eq 1280 -or $w -eq 720){
  $live=@('-auto','-from','0','--timing-hud-capture')
  if($vertical){Set-Content -LiteralPath (Join-Path $session.appRoot 'skin_preset.json') -Value '{"folder":"default","missing":"default","indicatorPortrait":2,"indicatorLandscape":true}' -Encoding UTF8}
  # Real AUTO judgments, with no injected fixture values in either orientation.
  Start-KsmOwnedProcess -Session $session -Arguments ($live+@($chart)) -TimeoutSeconds 50
  Copy-Item -LiteralPath (Join-Path $session.appRoot 'timing-captures/hud-8.png') -Destination (Join-Path $session.appRoot 'mini-live.png') -Force
 }
 & $python (Join-Path $root 'tests/MiniJudgeIndicatorVerifier.py') $session.appRoot $w $h
 if($LASTEXITCODE -ne 0){throw "Mini indicator verification failed: $($session.root)"}
 $results+=[pscustomobject]@{status='PASS';width=$w;height=$h;vertical=$vertical;session=$session.root}
 $results|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $root 'build/mini-indicator-tests.json') -Encoding UTF8
 Write-Output "MINI INDICATOR $w`x$h PASS"
}
