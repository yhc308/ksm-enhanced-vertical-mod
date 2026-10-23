param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'scripts/ModCompatibility.psm1') -Force
Import-Module (Join-Path $root 'scripts/TestWorkspace.psm1') -Force
$installer=Join-Path $root 'scripts/ModInstaller.ps1'
$fixture=Join-Path $root ('build/environment-tests/'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $fixture -Force|Out-Null
$checks=@()
function Check([bool]$Value,[string]$Name){if(-not $Value){throw "FAIL: $Name"};$script:checks+=$Name}
function Reject([scriptblock]$Operation,[string]$Name){$rejected=$false;try{& $Operation|Out-Null}catch{$rejected=$true};Check $rejected $Name}
function Put([string]$Path,[string]$Text){New-Item -ItemType Directory -Path (Split-Path $Path -Parent) -Force|Out-Null;[IO.File]::WriteAllText($Path,$Text)}
function Hash([string]$Path){(Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash}
$package=Join-Path $fixture 'package'
Put (Join-Path $package 'payload/ksm-enhanced-vertical.exe') 'MOD-RUNTIME'
Put (Join-Path $package 'payload/mods/vertical/ui/test.noco') 'MOD-UI'
Put (Join-Path $package 'payload/imgs/standby_vertical.png') 'DEFAULT-SKIN'
$files=foreach($relative in @('ksm-enhanced-vertical.exe','mods/vertical/ui/test.noco','imgs/standby_vertical.png')){[ordered]@{path=$relative;sha256=Hash (Join-Path $package "payload/$relative");policy=if($relative -like 'imgs/*'){'seed'}else{'replace'}}}
$manifest=[ordered]@{schemaVersion=2;modId='ksm-enhanced-vertical';status='development';runtimeContract='test-contract';supportedProfiles=@('upstream-A','upstream-B');buildId='fixture';files=@($files)}
$manifest|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $package 'manifest.json') -Encoding UTF8
$profiles=@()
foreach($version in @('A','B')){
 $game=Join-Path $fixture "game-$version"
 Put (Join-Path $game 'kshootmania.exe') "ORIGINAL-$version"
 Put (Join-Path $game 'ui/scene/title.noco') "UI-$version"
 Put (Join-Path $game 'skins/User Skin/base.gif') 'USER-CUSTOM-SKIN'
 Put (Join-Path $game 'config.ini') 'USER-CONFIG'
 Put (Join-Path $game 'songs/chart.ksh') 'USER-CHART'
 Put (Join-Path $game 'score/user.dat') 'USER-SCORE'
 Put (Join-Path $game 'imgs/standby_vertical.png') 'USER-SKIN'
 Put (Join-Path $game 'mods/vertical/ui/test.noco') 'ORIGINAL-UI'
 $profiles+=[ordered]@{id="upstream-$version";upstreamVersion=$version;runtimeContract='test-contract';status='verified';executableHashes=@(Hash (Join-Path $game 'kshootmania.exe'));requiredFiles=@(@{path='ui/scene/title.noco';sha256=Hash (Join-Path $game 'ui/scene/title.noco')})}
}
[ordered]@{schemaVersion=2;modId='ksm-enhanced-vertical';profiles=$profiles}|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $package 'targets.json') -Encoding UTF8
foreach($version in @('A','B')){
 $game=Join-Path $fixture "game-$version"
 $before=@{};foreach($relative in @('kshootmania.exe','config.ini','songs/chart.ksh','score/user.dat','skins/User Skin/base.gif','imgs/standby_vertical.png','mods/vertical/ui/test.noco')){$before[$relative]=Hash (Join-Path $game $relative)}
 $probe=Test-KsmModTarget $game (Join-Path $package 'targets.json')
 Check ($probe.supported -and $probe.profileId -eq "upstream-$version") "Version $version adapter selected"
 & $installer -Action Install -GameDirectory $game -PackageDirectory $package -AllowDevelopment|Out-Null
 & $installer -Action Verify -GameDirectory $game|Out-Null
 Check ((Hash (Join-Path $game 'kshootmania.exe')) -eq $before['kshootmania.exe']) "Version $version original executable preserved"
 foreach($relative in @('config.ini','songs/chart.ksh','score/user.dat','skins/User Skin/base.gif','imgs/standby_vertical.png')){Check ((Hash (Join-Path $game $relative)) -eq $before[$relative]) "Version $version preserved $relative"}
 $held=[IO.File]::Open((Join-Path $game 'ksm-enhanced-vertical.exe'),[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::Read)
 try {
  Reject {& $installer -Action Remove -GameDirectory $game} "Version $version locked mod rejected without stopping a game"
  Check (([IO.File]::ReadAllText((Join-Path $game 'mods/vertical/ui/test.noco'))) -eq 'MOD-UI') "Version $version lock failure leaves installed resources unchanged"
 } finally {$held.Dispose()}
 Put (Join-Path $game 'mods/vertical/ui/test.noco') 'USER-EDITED-MOD'
 Reject { & $installer -Action Remove -GameDirectory $game } "Version $version modified mod protected"
 Check (([IO.File]::ReadAllText((Join-Path $game 'mods/vertical/ui/test.noco'))) -eq 'USER-EDITED-MOD') "Version $version modified content retained"
 Copy-Item -LiteralPath (Join-Path $package 'payload/mods/vertical/ui/test.noco') -Destination (Join-Path $game 'mods/vertical/ui/test.noco') -Force
 & $installer -Action Remove -GameDirectory $game|Out-Null
 Check (-not(Test-Path -LiteralPath (Join-Path $game 'ksm-enhanced-vertical.exe'))) "Version $version sidecar removed"
 foreach($relative in $before.Keys){Check ((Hash (Join-Path $game $relative)) -eq $before[$relative]) "Version $version restored $relative"}
}
$game=Join-Path $fixture 'game-A'
Put (Join-Path $game 'kshootmania.exe') 'UNKNOWN-UPSTREAM'
Reject {& $installer -Action Install -GameDirectory $game -PackageDirectory $package -AllowDevelopment} 'Unknown upstream rejected'
Check (-not(Test-Path -LiteralPath (Join-Path $game 'ksm-enhanced-vertical.exe'))) 'Unknown upstream no mod mutation'
Put (Join-Path $game 'kshootmania.exe') 'ORIGINAL-A'
Put (Join-Path $game 'ui/scene/title.noco') 'INCOMPATIBLE-UI'
Reject {& $installer -Action Install -GameDirectory $game -PackageDirectory $package -AllowDevelopment} 'Resource contract mismatch rejected'
Put (Join-Path $game 'ui/scene/title.noco') 'UI-A'
Reject {Resolve-KsmSafePath $game '../escape'} 'Traversal rejected'
Reject {Assert-KsmPayloadPath 'config.ini'} 'User config payload rejected'
Reject {Assert-KsmPayloadPath 'kshootmania.exe'} 'Original executable payload rejected'
Put (Join-Path $package 'payload/mods/vertical/ui/test.noco') 'TAMPER'
Reject {& $installer -Action Install -GameDirectory $game -PackageDirectory $package -AllowDevelopment} 'Tampered payload rejected before mutation'
Put (Join-Path $package 'payload/mods/vertical/ui/test.noco') 'MOD-UI'
& $installer -Action Install -GameDirectory $game -PackageDirectory $package -AllowDevelopment|Out-Null
Put (Join-Path $game 'kshootmania.exe') 'NEW-UPSTREAM-A'
Reject {& $installer -Action Verify -GameDirectory $game} 'Official update detected'
& $installer -Action Remove -GameDirectory $game|Out-Null
Check (([IO.File]::ReadAllText((Join-Path $game 'kshootmania.exe'))) -eq 'NEW-UPSTREAM-A') 'Remove after official update preserves new original'
# A default image seeded by the installer becomes user-owned and survives later editing/removal.
Put (Join-Path $game 'kshootmania.exe') 'ORIGINAL-A'
Remove-Item -LiteralPath (Join-Path $game 'imgs/standby_vertical.png')
& $installer -Action Install -GameDirectory $game -PackageDirectory $package -AllowDevelopment|Out-Null
Check (([IO.File]::ReadAllText((Join-Path $game 'imgs/standby_vertical.png'))) -eq 'DEFAULT-SKIN') 'Fresh image seed installed'
Put (Join-Path $game 'imgs/standby_vertical.png') 'USER-EDITED-SEEDED-SKIN'
& $installer -Action Verify -GameDirectory $game|Out-Null
& $installer -Action Remove -GameDirectory $game|Out-Null
Check (([IO.File]::ReadAllText((Join-Path $game 'imgs/standby_vertical.png'))) -eq 'USER-EDITED-SEEDED-SKIN') 'User-edited seeded image preserved on removal'
# Hold a same-name game process open and exercise the actual isolated launcher module.
$csc=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$source=Join-Path $fixture 'sentinel.cs'
Put $source 'using System;using System.IO;using System.Threading;class Sentinel{static void Main(string[] a){File.WriteAllText(a[0],"ready");while(!File.Exists(a[1]))Thread.Sleep(50);}}'
$sentinelExe=Join-Path $fixture 'kshootmania.exe'
& $csc /nologo /target:exe "/out:$sentinelExe" $source
if($LASTEXITCODE -ne 0){throw 'Sentinel build failed'}
$ready=Join-Path $fixture 'ready';$finish=Join-Path $fixture 'finish'
$process=Start-Process -FilePath $sentinelExe -ArgumentList @($ready,$finish) -WindowStyle Hidden -PassThru
try {
 for($i=0;$i -lt 50 -and -not(Test-Path -LiteralPath $ready);$i++){Start-Sleep -Milliseconds 100}
 Check (Test-Path -LiteralPath $ready) 'Same-name running game sentinel ready'
 $first=New-KsmTestRun -Mode Prepare
 $second=New-KsmTestRun -Mode Prepare
 Check ($first.appRoot -ne $second.appRoot) 'Concurrent runs use different App paths'
 Put (Join-Path $first.appRoot 'config.ini') 'OPEN-TEST-SETTINGS'
 Start-KsmOwnedProcess -Session $second -Arguments @('--vertical-menu-input-test') -TimeoutSeconds 60
 $process.Refresh()
 Check (-not $process.HasExited) 'Existing same-name game remained running during diagnostics'
 Check (([IO.File]::ReadAllText((Join-Path $first.appRoot 'config.ini'))) -eq 'OPEN-TEST-SETTINGS') 'Other test run settings untouched'
 Check ((Get-Content -LiteralPath (Join-Path $second.appRoot 'vertical-menu-input.csv')|Select-Object -Last 1) -match 'PASS') 'Diagnostic completes with existing game process'
} finally {Put $finish 'finish';if(-not $process.WaitForExit(5000)){throw 'Owned sentinel did not exit after its explicit finish signal'}}
[ordered]@{status='PASS';count=$checks.Count;checks=$checks;fixture=$fixture}|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $fixture 'results.json') -Encoding UTF8
Write-Output "PASS $($checks.Count) environment checks: $fixture"