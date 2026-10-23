param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
$session=New-AsioFixture -Values @{vsync=0;viewtiming=2;inputdelay=0;laserdelay=0;visualoffset=0;audio_backend='asio:KSM Mock ASIO';automaticmute=0}
$engine=Get-Content -LiteralPath (Join-Path $root 'build/audio-engine-latest.json') -Raw|ConvertFrom-Json
Copy-Item -LiteralPath (Join-Path $engine.runtime 'bassasio.dll') -Destination (Join-Path $session.appRoot 'bassasio.dll') -Force
$destination=Join-Path $root ('reference/judge-setting/'+$session.buildId)
New-Item -ItemType Directory -Force -Path $destination|Out-Null
try {
 $report=Invoke-AsioFixture -Session $session -Flags @('--judge-setting-test') -Report 'judge-setting-test.txt' -Expected 'TOTAL \d+ judge-setting checks passed' -Timeout 240
 foreach($file in Get-ChildItem -LiteralPath $session.appRoot -Filter 'judge-setting-*' -File){Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $destination $file.Name) -Force}
 $records=@(Get-ChildItem -LiteralPath $session.appRoot -Filter '*.ksc' -Recurse -File)
 if($records.Count){throw 'Judge setting test wrote score records.'}
 [ordered]@{status='PASS';buildId=$session.buildId;session=$session.root;reference=$destination;report=$report;scoreFiles=$records.Count}|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $root 'build/judge-setting-tests.json') -Encoding UTF8
 Get-Content -LiteralPath $report -Tail 1
} catch {
 $failure=Join-Path $session.appRoot 'judge-setting-failure.txt'
 if(Test-Path -LiteralPath $failure){Get-Content -LiteralPath $failure}
 Write-Output "Judge setting session: $($session.root)"
 throw
}
