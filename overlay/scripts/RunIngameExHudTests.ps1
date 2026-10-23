param([int[]]$Widths=@(640,1280,1920,720,1080),[string]$Report='ingame-ex-hud-tests.json')
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
Import-Module (Join-Path $root 'scripts/TestWorkspace.psm1') -Force
$results=@();$python='python'
foreach($size in @(@(640,480),@(1280,720),@(1920,1080),@(720,1280),@(1080,1920))){
 $w=$size[0];$h=$size[1];if($w -notin $Widths){continue}
 foreach($kind in @('fixture','live')){
  if($kind -eq 'live' -and $w -notin @(1280,720)){continue}
  # Each capture mode owns a fresh directory: missed samples must never reuse fixture files.
  $session=New-AsioFixture -Values @{vertical_enabled=[int]($h -gt $w);windowsize="$w,$h";vertical_resolution="$w,$h";vertical_width=$w;vertical_height=$h;vsync=1;automaticmute=0} -Song
  $chart=Join-Path $session.appRoot 'songs/K-Shoot MANIA/flavorofyouth/flavorofyouth_ch.ksh'
  $flags=@('-auto','-from','0','--timing-hud-capture');if($kind -eq 'fixture'){$flags+='--ex-hud-fixture'}
  Start-KsmOwnedProcess -Session $session -Arguments ($flags+@($chart)) -TimeoutSeconds 45
  $dest=Join-Path $root "reference/ingame-ex-score/$w`x$h/$kind"
  New-Item -ItemType Directory -Path $dest -Force|Out-Null
  foreach($i in 0..9){
   Copy-Item -LiteralPath (Join-Path $session.appRoot "timing-captures/hud-$i.png") -Destination (Join-Path $dest "hud-$i.png") -Force
   Copy-Item -LiteralPath (Join-Path $session.appRoot "ex-hud-$i.json") -Destination (Join-Path $dest "hud-$i.json") -Force
  }
  & $python (Join-Path $root 'tests/IngameExHudVerifier.py') $session.appRoot $dest $w $h $kind
  if($LASTEXITCODE -ne 0){throw "EX HUD verification failed: $w/$h/$kind"}
  $results+=[pscustomobject]@{status='PASS';width=$w;height=$h;kind=$kind;session=$session.root;captures=$dest}
  $results|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $root ('build/'+$Report)) -Encoding UTF8
  Write-Output "EX HUD PASS $w/$h/$kind"
 }
}
