param([ValidateSet('before','after')][string]$Profile='after')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$app=Join-Path $root 'kshootmania/App'
$out=Join-Path $root "test-output/gear-origin/$Profile"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$hash=(Get-FileHash (Join-Path $app 'config.ini')).Hash
foreach($layer in @('arrow','techno')) {
 foreach($degrees in @(-10,0,10)) {
  $flags="--vertical-preview-hud --vertical-preview-zero --vertical-preview-bg night --vertical-preview-layer $layer --vertical-preview-tilt $degrees"
  $p=Start-Process (Join-Path $app 'kshootmania.exe') -WorkingDirectory $app -ArgumentList $flags -WindowStyle Hidden -PassThru
  if(-not $p.WaitForExit(20000)){throw 'Preview timed out'}
  if($p.ExitCode -ne 0){throw 'Preview failed'}
  Copy-Item -LiteralPath (Join-Path $app 'Screenshot/vertical-hud-preview.png') -Destination (Join-Path $out "$layer-$degrees.png")
  Write-Output "$Profile $layer $degrees"
 }
}
if((Get-FileHash (Join-Path $app 'config.ini')).Hash -ne $hash){throw 'Preview modified config'}
