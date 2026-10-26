param([int[]]$Widths=@(720,1080))
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'TestWorkspace.psm1') -Force
$python='python'
$results=@()
foreach($width in $Widths){
 $height=[int]($width*16/9);$values=@{vertical_enabled=1;windowsize="$width,$height";vertical_resolution="$width,$height";vertical_width=$width;vertical_height=$height;vsync=1;currentdir='LayoutSelection';currentitem=7;currentlevel=2;automaticmute=0}
 $session=New-AsioFixture -Values $values -Song
 & $python (Join-Path $root 'tests/PortraitScenesVerifier.py') setup $session.appRoot
 if($LASTEXITCODE -ne 0){throw 'Portrait fixture setup failed'}
 $sizeFlag=if($width -eq 1080){'--vertical-preview-1080'}else{'--vertical-preview-720'}
 Start-KsmOwnedProcess -Session $session -Arguments @('--vertical-preview-prepare',$sizeFlag,'--vertical-preview-chart',(Join-Path $session.appRoot 'layout-fixture/prepare.ksh')) -TimeoutSeconds 30
 Start-KsmOwnedProcess -Session $session -Arguments @('--vertical-preview-option-scene',$sizeFlag) -TimeoutSeconds 30
 foreach($kind in @('base','down','up')){
  Set-AsioFixtureConfig -Session $session -Values $values
  $flags=@('--vertical-preview-select',$sizeFlag)
  if($kind -ne 'base'){$flags+='--vertical-preview-select-'+$kind}
  Start-KsmOwnedProcess -Session $session -Arguments $flags -TimeoutSeconds 30
  Copy-Item -LiteralPath (Join-Path $session.appRoot 'select-preview-state.json') -Destination (Join-Path $session.appRoot "select-state-$kind.json") -Force
  $capture=@(Get-ChildItem -LiteralPath $session.appRoot -Recurse -Filter 'vertical-select-preview.png' -File)
  if($capture.Count -ne 1){throw 'Missing selection capture'}
  Move-Item -LiteralPath $capture[0].FullName -Destination (Join-Path $session.appRoot "vertical-select-$kind.png") -Force
 }
 & $python (Join-Path $root 'tests/PortraitScenesVerifier.py') verify $session.appRoot $width
 if($LASTEXITCODE -ne 0){throw "Portrait verification failed: $($session.root)"}
 $results+=[pscustomobject]@{width=$width;status='PASS';session=$session.root}
}
$results|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $root 'build/portrait-scenes-latest.json') -Encoding UTF8
