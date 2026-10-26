param([Parameter(Mandatory=$true)][string]$Driver)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
Import-Module (Join-Path $root 'scripts/TestWorkspace.psm1') -Force
$playExe=Join-Path $root 'play-test/App/ksm-enhanced-vertical.exe'
if(@(Get-CimInstance Win32_Process -Filter "Name='ksm-enhanced-vertical.exe'"|Where-Object {$_.ExecutablePath -eq $playExe}).Count){throw 'Play is running; hardware test deferred'}
$session=New-AsioFixture -Values @{audio_backend=('asio:'+$Driver);mastervol=0;automaticmute=0;vertical_enabled=1;windowsize='720,1280';vertical_resolution='720,1280'}
$chartRoot=Join-Path $session.appRoot 'songs/eof-song'
Copy-Item -LiteralPath (Join-Path $root 'tests/fixtures/songs/K-Shoot MANIA/magicalkittens') -Destination $chartRoot -Recurse
Start-KsmOwnedProcess -Session $session -Arguments @('-auto','-from','81',(Join-Path $chartRoot 'magicalkittens_ex.ksh')) -TimeoutSeconds 45
$logs=@(Get-ChildItem -LiteralPath (Join-Path $session.appRoot 'log') -Filter 'events-*.log' -Recurse)
$text=($logs|ForEach-Object {Get-Content -LiteralPath $_.FullName -Raw}) -join "`n"
foreach($pattern in @('CONFIG_BACKEND 0 0 asio:','OUTPUT [1-9][0-9]* [1-9][0-9]*','JUDGMENT_FINISHED_OR_FAILED','BGM_STREAM_NOT_PLAYING','BGM_TAIL_CLOCK_BEGIN','PLAY_FINISH_REQUEST')){if($text -notmatch $pattern){throw "Missing real device evidence: $pattern in $($session.root)"}}
[ordered]@{status='PASS';driver=$Driver;session=$session.root;muted=$true;mode='real ASIO device; actual OGG and chart; automatic exit after EOF'}|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $root 'build/asio-eof-device-test.json') -Encoding UTF8
Write-Output "REAL DEVICE EOF PASS $Driver $($session.root)"
