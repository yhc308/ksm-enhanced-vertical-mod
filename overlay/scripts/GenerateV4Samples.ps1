Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$app = Join-Path $root 'kshootmania/App'
$out = Join-Path $root 'test-output/background-v4-samples'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$config = Join-Path $app 'config.ini'
$before = (Get-FileHash -LiteralPath $config).Hash
$chart = Join-Path (Split-Path $root -Parent) 'vertical_play_test/songs/K-Shoot MANIA/jurer/jurer_ex.ksh'
foreach ($theme in @('cloudy','sky','cyber','smoke')) {
    $bg = if ($theme -eq 'smoke') { 'night' } else { $theme }
    $layer = if ($theme -eq 'smoke') { 'smoke' } else { 'arrow' }
    $args = "--vertical-preview-hud --vertical-preview-zero --vertical-preview-bg $bg --vertical-preview-layer $layer"
    if (Test-Path -LiteralPath $chart) { $args += ' --vertical-preview-chart "' + $chart + '"' }
    $p = Start-Process -FilePath (Join-Path $app 'kshootmania.exe') -WorkingDirectory $app -ArgumentList $args -WindowStyle Hidden -PassThru
    $p.WaitForExit()
    if ($p.ExitCode -ne 0) { throw "Capture failed: $theme" }
    Copy-Item -LiteralPath (Join-Path $app 'Screenshot/vertical-hud-preview.png') -Destination (Join-Path $out "$theme-gauge0.png")
    Write-Output "Captured $theme (gauge 0%)"
}
if ((Get-FileHash -LiteralPath $config).Hash -ne $before) { throw 'Preview changed config.ini' }
Write-Output 'PASS: four requested captures, config unchanged.'
