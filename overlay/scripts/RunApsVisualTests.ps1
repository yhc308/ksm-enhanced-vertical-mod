param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
Import-Module (Join-Path $root 'scripts/TestWorkspace.psm1') -Force
$destination=Join-Path $root 'reference/aps'
New-Item -ItemType Directory -Path $destination -Force|Out-Null
foreach($vertical in @(0,1)){
 $session=New-AsioFixture -Values @{vertical_enabled=$vertical;windowsize=$(if($vertical){'720,1280'}else{'1280,720'});vsync=1}
 foreach($spent in @(0,1)){
  $flags=@('--vertical-preview-hud','--vertical-preview-fast','--aps-visual-fixture')
  $flags+=if($vertical){'--vertical-preview-720'}else{'--vertical-preview-landscape'}
  if($spent){$flags+='--aps-spent'}else{$flags+='--vertical-preview-hundred'}
  Start-KsmOwnedProcess -Session $session -Arguments $flags -TimeoutSeconds 30
  $file=Get-ChildItem -LiteralPath $session.appRoot -Filter 'vertical-hud-preview.png' -Recurse|Select-Object -First 1
  if(-not $file){throw "APS HUD capture missing: $($session.root)"}
  Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $destination "full-vertical-$vertical-spent-$spent.png") -Force
 }
}
& 'python' (Join-Path $root 'tests/ApsVisualVerifier.py')
if($LASTEXITCODE){throw 'APS pixel verification failed'}
