param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
$session=New-AsioFixture -Values @{vsync=0;viewtiming=2;automaticmute=0}
$report=Invoke-AsioFixture -Session $session -Flags @('--judge-setting-test','--judge-setting-performance') -Report 'judge-setting-performance.csv' -Expected 'width,height,mode,frames,mean_ms,p99_ms,over_8_333ms' -Timeout 180
$destination=Join-Path $root ('reference/judge-setting/'+$session.buildId)
New-Item -ItemType Directory -Force -Path $destination|Out-Null
Copy-Item -LiteralPath $report -Destination (Join-Path $destination 'performance.csv') -Force
$cases=@(Import-Csv -LiteralPath $report)
if($cases.Count -ne 6 -or @($cases|Where-Object {[int]$_.frames -ne 960}).Count){throw 'Incomplete performance report.'}
[ordered]@{status='PASS';buildId=$session.buildId;session=$session.root;cases=$cases}|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $root 'build/judge-setting-performance.json') -Encoding UTF8
$cases|Format-Table
