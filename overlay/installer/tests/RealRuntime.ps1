param(
    [Parameter(Mandatory=$true)][string]$ProjectRoot,
    [Parameter(Mandatory=$true)][string]$OriginalGameDirectory,
    [Parameter(Mandatory=$true)][string]$BuildInformation,
    [Parameter(Mandatory=$true)][string]$SetupExecutable
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath($ProjectRoot)
$original=[IO.Path]::GetFullPath($OriginalGameDirectory)
Import-Module (Join-Path $root 'scripts/ModCompatibility.psm1') -Force
Import-Module (Join-Path $root 'scripts/TestWorkspace.psm1') -Force
$build=Get-Content -LiteralPath $BuildInformation -Raw|ConvertFrom-Json
if($build.configuration -ne 'Release' -or $build.diagnostics){throw 'A normal Release build is required.'}
if((Get-FileHash -LiteralPath $build.executable).Hash -ne $build.sha256){throw 'Build executable hash differs.'}
$probe=Test-KsmModTarget $original (Join-Path $root 'compatibility/targets.json')
if(-not $probe.supported){throw ($probe.issues -join '; ')}
$run=Join-Path $root ('build/native-installer-runtime/'+[guid]::NewGuid().ToString('N'))
$app=Join-Path $run 'App'
New-Item -ItemType Directory -Path $app -Force|Out-Null
# Copy only runtime inputs into a new owned fixture; no original config/scores/songs.
foreach($directory in @('assets','dll','engine','imgs','lang','se','shaders','ui')){
    $source=Join-Path $original $directory
    if(@(Get-ChildItem -LiteralPath $source -Recurse -Force|Where-Object {$_.Attributes -band [IO.FileAttributes]::ReparsePoint}).Count){throw 'Linked original runtime inputs are unsupported.'}
    Copy-Item -LiteralPath $source -Destination (Join-Path $app $directory) -Recurse
}
foreach($name in @('kshootmania.exe','bass.dll','bass_fx.dll')){Copy-Item -LiteralPath (Join-Path $original $name) -Destination (Join-Path $app $name)}
$config=New-KsmConfiguration -Mode Auto -Existing "[config]`ncurrentlang=Japanese`nvertical_enabled=1`nwindowsize=720,1280`nvertical_resolution=720,1280`nmastervol=0`n"
[IO.File]::WriteAllText((Join-Path $app 'config.ini'),$config,[Text.UTF8Encoding]::new($false))
foreach($relative in @('songs/user/chart.ksh','score/user.ksc','skins/User Skin/skin.json')){
    $path=Join-Path $app $relative
    New-Item -ItemType Directory -Path (Split-Path $path -Parent) -Force|Out-Null
    [IO.File]::WriteAllText($path,'USER-OWNED PRESERVATION SENTINEL')
}
foreach($relative in @('score/normal.ksc','score/legacy_additional.ksc','score/new_additional.ksc','score_backup/backup.ksc','courses/score/course.ksc')){
 $path=Join-Path $app $relative;New-Item -ItemType Directory -Path (Split-Path $path -Parent) -Force|Out-Null
 [IO.File]::WriteAllText($path,'normal=1,2,3,4,5,6,7,8,9,10,11')
}
$before=@{}
foreach($file in Get-ChildItem -LiteralPath $app -Recurse -File){$before[[IO.Path]::GetRelativePath($app,$file.FullName).Replace('\','/')]=(Get-FileHash -LiteralPath $file.FullName).Hash}
$protected=@('kshootmania.exe','config.ini','songs/user/chart.ksh','score/user.ksc','skins/User Skin/skin.json')
$installer=$SetupExecutable
function Invoke-Native([string]$Action){
 $report=Join-Path $run ("native-"+$Action+"-"+[guid]::NewGuid().ToString('N')+".json")
 $arguments='--quiet --action '+$Action+' --target "'+$app+'" --report "'+$report+'"'
 $p=Start-Process -FilePath $installer -ArgumentList $arguments -PassThru -Wait -WindowStyle Hidden
 if($p.ExitCode -ne 0){throw (Get-Content -LiteralPath $report -Raw)}
 $script:lastNativeResult=Get-Content -LiteralPath $report -Raw|ConvertFrom-Json
}
try{
    Invoke-Native inspect
    $env:KSM_SETUP_FAIL_AFTER='2'; $env:KSM_SETUP_CRASH_AFTER='2'; $env:KSM_SETUP_LOW_SPACE='1'
    Invoke-Native install
    Remove-Item Env:KSM_SETUP_FAIL_AFTER,Env:KSM_SETUP_CRASH_AFTER,Env:KSM_SETUP_LOW_SPACE
    Invoke-Native verify
    foreach($relative in $protected){if((Get-FileHash -LiteralPath (Join-Path $app $relative)).Hash -ne $before[$relative]){throw "Install changed user/original file: $relative"}}
    $session=[pscustomobject]@{id='installed-normal';root=$run;appRoot=$app;executable=(Join-Path $app 'ksm-enhanced-vertical.exe');processIds=@();buildId=$build.buildId}
    foreach($mode in @(0,1)){
      foreach($kind in @('title','chart-cli')){
       $width=if($mode){720}else{1280};$height=if($mode){1280}else{720}
       $seed="[config]`ncurrentlang=Japanese`nvertical_enabled=$mode`nwindowsize=$width,$height`nvertical_resolution=$width,$height`nmastervol=0`n"
       [IO.File]::WriteAllText((Join-Path $app 'config.ini'),(New-KsmConfiguration -Mode Auto -Existing $seed),[Text.UTF8Encoding]::new($false))
       $flags=if($kind -eq 'chart-cli'){@('-auto','-from','0',(Join-Path $app 'mods/enhanced/judge_setting/Preview_Piano.kson'))}else{@()}
       $process=Start-KsmOwnedProcess -Session $session -Arguments $flags -Interactive
    try{
        $watch=[Diagnostics.Stopwatch]::StartNew();$ready=$false
        while($watch.Elapsed.TotalSeconds -lt 30){
            if($process.WaitForExit(100)){throw 'Installed normal executable exited during startup.'}
            $process.Refresh()
            if($process.MainWindowTitle -eq 'K-Shoot Mania V2.1.0 - Enhanced Vertical Mod (Unofficial Build)'){$ready=$true;break}
        }
        if(-not $ready){throw 'Installed normal game window was not ready.'}
        $busyReport=Join-Path $run 'running-game.json'
        $busy=Start-Process -FilePath $installer -ArgumentList ('--quiet --action remove --target "'+$app+'" --report "'+$busyReport+'"') -PassThru -Wait -WindowStyle Hidden
        if($busy.ExitCode -eq 0 -or (Get-Content -LiteralPath $busyReport -Raw|ConvertFrom-Json).code -ne 'busy'){throw 'Running game was not rejected'}
        if($process.WaitForExit(4000)){throw 'Installed normal game exited during the smoke interval.'}
        if(-not $process.CloseMainWindow() -or -not $process.WaitForExit(30000) -or $process.ExitCode -ne 0){throw 'Installed normal game did not close gracefully.'}
    }catch{
        if(-not $process.HasExited -and $process.Path -ieq $session.executable){$process.Kill();$process.WaitForExit()}
        throw
    }
      }
    }
    # The game's own normal config persistence is distinct from installer mutation.
    $configAfterRuntime=(Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash
    Invoke-Native verify
    foreach($relative in @('score/earned.ksc','score/earned_additional.ksc','courses/score/earned.ksc')){
     $path=Join-Path $app $relative;New-Item -ItemType Directory -Path (Split-Path $path -Parent) -Force|Out-Null
     [IO.File]::WriteAllText($path,'EARNED AFTER INSTALL')
     $before[$relative]=(Get-FileHash -LiteralPath $path).Hash
    }
    $createdBeforeRemove=@((Get-Content -LiteralPath (Join-Path $app '.ksm-enhanced-install.json') -Raw|ConvertFrom-Json).createdDirectories)
    Invoke-Native remove
    foreach($relativeDirectory in $createdBeforeRemove){
     $directory=Join-Path $app $relativeDirectory
     if((Test-Path -LiteralPath $directory -PathType Container) -and @(Get-ChildItem -LiteralPath $directory -Force).Count -eq 0){
      if(-not @($script:lastNativeResult.cleanupWarnings|Where-Object {$_.path -eq $relativeDirectory}).Count){throw "Tracked empty directory remains without a warning: $relativeDirectory"}
     }
    }
    foreach($relative in $before.Keys){
        $expected=if($relative -eq 'config.ini'){$configAfterRuntime}else{$before[$relative]}
        if((Get-FileHash -LiteralPath (Join-Path $app $relative)).Hash -ne $expected){throw "Removal did not preserve/restore: $relative"}
    }
    if(Test-Path -LiteralPath (Join-Path $app 'ksm-enhanced-vertical.exe')){throw 'Mod executable remains after removal.'}
    Invoke-Native install
    Invoke-Native verify
    $createdBeforeRemove=@((Get-Content -LiteralPath (Join-Path $app '.ksm-enhanced-install.json') -Raw|ConvertFrom-Json).createdDirectories)
    Invoke-Native remove
    foreach($relativeDirectory in $createdBeforeRemove){
     $directory=Join-Path $app $relativeDirectory
     if((Test-Path -LiteralPath $directory -PathType Container) -and @(Get-ChildItem -LiteralPath $directory -Force).Count -eq 0){
      if(-not @($script:lastNativeResult.cleanupWarnings|Where-Object {$_.path -eq $relativeDirectory}).Count){throw "Tracked empty directory remains without a warning: $relativeDirectory"}
     }
    }
    foreach($relative in $before.Keys){
     $expected=if($relative -eq 'config.ini'){$configAfterRuntime}else{$before[$relative]}
     if((Get-FileHash -LiteralPath (Join-Path $app $relative)).Hash -ne $expected){throw "Reinstall changed: $relative"}
    }
    [ordered]@{status='PASS';buildId=$build.buildId;diagnostics=$false;upstreamHash=$probe.executableHash;setup=$installer;fixture=$run;originalFilesRestored=$before.Count;installProtectedFiles=$protected;normalStartup=$true;runtimeCases=4;landscape=$true;portrait=$true;runningGameRejected=$true;productionFaultHooksDisabled=$true;emptyDirectoryCleanup=$true;gracefulExit=$true;userSkinsAndRecordsPreserved=$true;gameConfigAfterRuntimePreserved=$true}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $root 'build/native-installer-runtime-smoke.json') -Encoding UTF8
    Write-Output "NATIVE INSTALLATION PASS: $run"
}finally{Remove-Item Env:KSM_SETUP_FAIL_AFTER,Env:KSM_SETUP_CRASH_AFTER,Env:KSM_SETUP_LOW_SPACE -ErrorAction SilentlyContinue}
