param([int[]]$Widths=@(720),[switch]$Landscape)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'TestWorkspace.psm1') -Force
$results=@()
foreach($width in $Widths){
 $height=if($Landscape){if($width -eq 640){480}else{720}}else{[int]($width*16/9)}
 $values=@{vertical_enabled=if($Landscape){0}else{1};windowsize="$width,$height";vertical_width=$width;vertical_height=$height;vertical_resolution="$width,$height";vsync='1';viewtiming=2;timing_position=0;timing_ms_display=3;timing_ms_color=2;assisttick=2;automaticmute=0;currentdir='K-Shoot MANIA';currentitem=1;currentlevel=1}
 $session=New-AsioFixture -Values $values -Song
 $chart=Join-Path $session.appRoot 'songs/K-Shoot MANIA/flavorofyouth/flavorofyouth_ch.ksh'
 foreach($mode in @('hud','menu')){
  $flags=if($mode -eq 'hud'){@('-auto','-from','0','--timing-hud-capture',$chart)}else{@('--timing-menu-capture')}
  Start-KsmOwnedProcess -Session $session -Arguments $flags -TimeoutSeconds 60
  $count=@(Get-ChildItem -LiteralPath (Join-Path $session.appRoot 'timing-captures') -Filter "$mode-*.png").Count
  $expected=if($mode -eq 'hud'){10}else{3}
  if($count -ne $expected){throw "Capture count mismatch: $mode $count/$expected; $($session.root)"}
  $results+=[pscustomobject]@{size="$width`x$height";mode=$mode;count=$count;session=$session.root}
  Write-Output "TIMING CAPTURE $width`x$height $mode`: $count"
 }
}
$results|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $root 'build/timing-captures-latest.json') -Encoding UTF8