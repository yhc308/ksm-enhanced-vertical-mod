Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$app = Join-Path $projectRoot 'kshootmania/App'
$output = Join-Path $projectRoot 'test-output/background-v2-samples'
New-Item -ItemType Directory -Force -Path $output | Out-Null
$config = Join-Path $app 'config.ini'
$before = (Get-FileHash -LiteralPath $config).Hash
$themes = @(Get-ChildItem -LiteralPath (Join-Path $app 'imgs/bg') -Filter '*0.jpg' | ForEach-Object { $_.BaseName.Substring(0,$_.BaseName.Length-1) })
$layers = @(Get-ChildItem -LiteralPath (Join-Path $app 'imgs/bg') -Filter '*.gif' | ForEach-Object { $_.BaseName })
$chart = Join-Path (Split-Path $projectRoot -Parent) 'vertical_play_test/songs/K-Shoot MANIA/jurer/jurer_ex.ksh'
$manifest = @()
foreach ($kind in @('base','layer')) {
    $names = if ($kind -eq 'base') { $themes } else { $layers }
    foreach ($name in $names) {
        foreach ($gauge in @(0,70)) {
            $bg = if ($kind -eq 'base') { $name } else { 'night' }
            $layer = if ($kind -eq 'layer') { $name } else { 'arrow' }
            $arguments = "--vertical-preview-hud --vertical-preview-bg $bg --vertical-preview-layer $layer"
            if (Test-Path -LiteralPath $chart) { $arguments += ' --vertical-preview-chart "' + $chart + '"' }
            if ($gauge -eq 0) { $arguments += ' --vertical-preview-zero' }
            $process = Start-Process -FilePath (Join-Path $app 'kshootmania.exe') -WorkingDirectory $app -ArgumentList $arguments -WindowStyle Hidden -PassThru
            $process.WaitForExit()
            if ($process.ExitCode -ne 0) { throw "Preview failed: $kind/$name/$gauge" }
            $filename = "$kind-$name-gauge$gauge.png"
            Copy-Item -LiteralPath (Join-Path $app 'Screenshot/vertical-hud-preview.png') -Destination (Join-Path $output $filename)
            $manifest += [pscustomobject]@{kind=$kind;theme=$name;gauge=$gauge;file=$filename}
            Write-Output "Captured $filename"
        }
    }
}
if ((Get-FileHash -LiteralPath $config).Hash -ne $before) { throw 'Preview changed config.ini' }
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $output 'manifest.json') -Encoding UTF8
Write-Output "PASS: $($manifest.Count) captures; config.ini unchanged."
