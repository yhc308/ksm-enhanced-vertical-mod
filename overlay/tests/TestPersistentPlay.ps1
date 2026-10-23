Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
$fixture=Join-Path $project ('build/persistent-play-tests/'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path (Join-Path $fixture 'build/bin'),(Join-Path $fixture 'config'),(Join-Path $fixture 'kshootmania/App/ui/scene') -Force|Out-Null
Copy-Item -LiteralPath (Join-Path $project 'config/default-windows.ini') -Destination (Join-Path $fixture 'config/default-windows.ini')
$checks=@()
function Check([bool]$Condition,[string]$Name){if(-not $Condition){throw "FAIL: $Name"};$script:checks+=$Name}
function Put([string]$Path,[string]$Value){New-Item -ItemType Directory -Path (Split-Path $Path -Parent) -Force|Out-Null;[IO.File]::WriteAllText($Path,$Value)}
$csc=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$source=Join-Path $fixture 'sentinel.cs'
Put $source 'using System.IO;using System.Threading;class Sentinel{static void Main(string[] a){File.WriteAllText(a[0],"ready");while(!File.Exists(a[1]))Thread.Sleep(50);}}'
$exe=Join-Path $fixture 'build/bin/ksm-enhanced-vertical.exe'
& $csc /nologo /target:exe "/out:$exe" $source
if($LASTEXITCODE -ne 0){throw 'Sentinel build failed'}
$build=@{buildId='A';executable=$exe;sha256=(Get-FileHash -LiteralPath $exe).Hash}
$build|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $fixture 'build/latest.json')
Put (Join-Path $fixture 'kshootmania/App/ui/scene/test.noco') 'UI-A'
Put (Join-Path $fixture 'kshootmania/App/imgs/unused.png') 'LEGACY-IMAGE'
Put (Join-Path $fixture 'kshootmania/App/imgs/bg/unused.gif') 'LEGACY-BACKGROUND'
Put (Join-Path $fixture 'kshootmania/App/imgs/base.gif') 'DEFAULT-IMAGE'
Put (Join-Path $fixture 'kshootmania/App/imgs/bg/used.gif') 'DEFAULT-BACKGROUND'
Put (Join-Path $fixture 'kshootmania/App/skins/MySkin/base.gif') 'CUSTOM-SKIN'
Put (Join-Path $fixture 'kshootmania/App/skins/editor/skin.json') '{}'
Put (Join-Path $fixture 'kshootmania/App/skins/editor/base.gif') 'EXPLICIT-CUSTOM-SKIN'

Put (Join-Path $fixture 'play-test/App/imgs/Legacy User/base.gif') 'LEGACY-USER-SKIN'
Put (Join-Path $fixture 'play-test/App/imgs/default/base.gif') 'OLD-DEFAULT'

$module=Import-Module (Join-Path $project 'scripts/TestWorkspace.psm1') -Force -PassThru
& $module {param($p) $script:persistentTestRoot=$p;function script:Get-KsmProjectRoot {$script:persistentTestRoot}} $fixture
$first=New-KsmTestRun -Mode Play
Check ($first.updatedFiles -eq 9) 'Initial runtime installs once'
Check (Test-Path -LiteralPath (Join-Path $first.appRoot 'imgs/unused.png')) 'Flat base images remain available'
Check (Test-Path -LiteralPath (Join-Path $first.appRoot 'imgs/bg/unused.gif')) 'Base image subfolders remain available'
Check (Test-Path -LiteralPath (Join-Path $first.appRoot 'imgs/bg/used.gif')) 'Default skin background resources remain available'
Check (Test-Path -LiteralPath (Join-Path $first.appRoot 'skins/MySkin/base.gif')) 'Additional skin folders remain available'
Check (Test-Path -LiteralPath (Join-Path $first.appRoot 'skins/editor/base.gif')) 'Explicit skins with a legacy folder name remain available'

Check (([IO.File]::ReadAllText((Join-Path $first.appRoot 'skins/Legacy User/base.gif'))) -eq 'LEGACY-USER-SKIN') 'Old user skin moved without changing bytes'
Check (-not(Test-Path -LiteralPath (Join-Path $first.appRoot 'imgs/default'))) 'Old default directory is retired'
Check (@(Get-ChildItem -LiteralPath (Join-Path $fixture 'build/skin-layout-backups') -Filter 'base.gif' -Recurse).Count -eq 2) 'Old user skin and default backup preserved'

$song=Join-Path $first.appRoot 'songs/테스트 곡/chart.ksh'
Put $song 'USER-CHART';Put (Join-Path $first.appRoot 'score/user.dat') 'USER-SCORE';Put (Join-Path $first.appRoot 'cache/user.dat') 'USER-CACHE'
$config=Join-Path $first.appRoot 'config.ini';Put $config 'USER-SETTINGS'
$before=(Get-Item -LiteralPath (Join-Path $first.appRoot 'ksm-enhanced-vertical.exe')).LastWriteTimeUtc
$second=New-KsmTestRun -Mode Play
Check ($first.appRoot -eq $second.appRoot) 'Repeated Play reuses the same game folder'
Check ($second.updatedFiles -eq 0) 'Unchanged launch copies zero runtime files'
Check ((Get-Item -LiteralPath $second.executable).LastWriteTimeUtc -eq $before) 'Unchanged executable is not rewritten'
Check (-not(Test-Path -LiteralPath (Join-Path $fixture 'test-runs'))) 'Play creates no new game snapshot'
$ready=Join-Path $fixture 'ready';$finish=Join-Path $fixture 'finish'
$info=[Diagnostics.ProcessStartInfo]::new();$info.FileName=$first.executable;$info.UseShellExecute=$false;$info.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden;$info.ArgumentList.Add($ready);$info.ArgumentList.Add($finish)
$process=[Diagnostics.Process]::Start($info)
try {
 for($i=0;$i -lt 50 -and -not(Test-Path -LiteralPath $ready);$i++){Start-Sleep -Milliseconds 100}
 Check (Test-Path -LiteralPath $ready) 'Owned running-game sentinel is ready'
 Put (Join-Path $fixture 'kshootmania/App/ui/scene/test.noco') 'UI-B'
 $build.buildId='B';$build|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $fixture 'build/latest.json')
 $busy=New-KsmTestRun -Mode Play
 Check ($busy.isRunning -and $busy.buildId -eq 'A' -and $busy.pendingBuildId -eq 'B') 'New build waits while the game is running'
 Check (([IO.File]::ReadAllText((Join-Path $busy.appRoot 'ui/scene/test.noco'))) -eq 'UI-A') 'Running game resources remain unchanged'
 $process.Refresh();Check (-not $process.HasExited) 'Running game is never terminated'
} finally {Put $finish 'finish';if(-not $process.WaitForExit(5000)){throw 'Owned sentinel did not exit after finish signal'}}
$updated=New-KsmTestRun -Mode Play
Check (-not $updated.isRunning -and $updated.buildId -eq 'B' -and $updated.updatedFiles -eq 1) 'Next launch applies only changed resources'
Check (([IO.File]::ReadAllText((Join-Path $updated.appRoot 'ui/scene/test.noco'))) -eq 'UI-B') 'Changed resource applied after exit'
Check (([IO.File]::ReadAllText($song)) -eq 'USER-CHART') 'Test songs persist across updates'
Check (([IO.File]::ReadAllText($config)) -eq 'USER-SETTINGS') 'User settings persist across updates'
Check (([IO.File]::ReadAllText((Join-Path $updated.appRoot 'score/user.dat'))) -eq 'USER-SCORE') 'Scores persist across updates'
Check (([IO.File]::ReadAllText((Join-Path $updated.appRoot 'cache/user.dat'))) -eq 'USER-CACHE') 'Cache persists across updates'
@{status='PASS';count=$checks.Count;checks=$checks;fixture=$fixture}|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $project 'build/persistent-play-check.json') -Encoding UTF8
Write-Output "PASS $($checks.Count) persistent Play checks"
