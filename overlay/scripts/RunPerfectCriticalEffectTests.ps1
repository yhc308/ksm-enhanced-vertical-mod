param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
Import-Module (Join-Path $root 'scripts/TestWorkspace.psm1') -Force
$results=@()
foreach($vertical in @(0,1)){
 $w=if($vertical){720}else{1280};$h=if($vertical){1280}else{720}
 $session=New-AsioFixture -Values @{vertical_enabled=$vertical;windowsize="$w,$h";vertical_resolution="$w,$h";vertical_width=$w;vertical_height=$h;vsync=1;viewtiming=2;timing_ms_display=3;timing_ms_color=2;automaticmute=0} -Song
 if($vertical -eq 0){
  $report=Invoke-AsioFixture -Session $session -Flags @('--timing-hud-smoke-test') -Report 'timing-hud-test.txt' -Expected 'TOTAL \d+ checks passed'
  if(Test-Path -LiteralPath (Join-Path $session.appRoot 'timing-hud-failure.txt')){throw (Get-Content -LiteralPath (Join-Path $session.appRoot 'timing-hud-failure.txt') -Raw)}
  Write-Output (Get-Content -LiteralPath $report -Tail 1)
 }
 $chart=Join-Path $session.appRoot 'songs/K-Shoot MANIA/flavorofyouth/flavorofyouth_ch.ksh'
 Start-KsmOwnedProcess -Session $session -Arguments @('-auto','-from','0','--timing-hud-capture',$chart) -TimeoutSeconds 50
 $name=if($vertical){'portrait'}else{'landscape'}
 $target=Join-Path $root "reference/perfect_critical_effect_$name.png"
 Copy-Item -LiteralPath (Join-Path $session.appRoot 'timing-captures/hud-8.png') -Destination $target -Force
 $results+=[pscustomobject]@{vertical=[bool]$vertical;status='PASS';session=$session.root;capture=$target}
 Write-Output "P-CRITICAL EFFECT $name captured"
}
$results|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $root 'build/perfect-critical-effects-tests.json') -Encoding UTF8
