param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
$session=New-AsioFixture
$report=Invoke-AsioFixture -Session $session -Flags @('--aps-test') -Report 'aps-test.txt' -Expected 'TOTAL \d+ APS checks passed' -Timeout 60
$destination=Join-Path $root 'reference/aps'
New-Item -ItemType Directory -Path $destination -Force|Out-Null
foreach($file in Get-ChildItem -LiteralPath $session.appRoot -Filter 'aps-*.png'){Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $destination $file.Name) -Force}
$session.root|Set-Content -LiteralPath (Join-Path $root 'build/aps-session.txt') -Encoding UTF8
Copy-Item -LiteralPath $report -Destination (Join-Path $destination 'aps-test.txt') -Force
Write-Output (Get-Content -LiteralPath $report -Tail 1)
Write-Output "APS session: $($session.root)"
