Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
function Get-KsmProjectRoot { Split-Path $PSScriptRoot -Parent }
function New-KsmConfiguration {
    param([string]$Mode,[string]$Existing='')
    $template=Join-Path (Get-KsmProjectRoot) 'config/default-windows.ini'
    $settings=[ordered]@{}
    foreach($line in ([IO.File]::ReadAllText($template) -split '\r?\n')){
        if($line -match '^([A-Za-z0-9_]+)=(.*)$'){$settings[$Matches[1]]=$Matches[2]}
    }
    $legacySeed=$Existing -and $Existing -notmatch '(?m)^key=' -and $Existing -match '(?m)^mastervol=0\s*$' -and $Existing -match '(?m)^windowsize=540,960\s*$'
    foreach($line in ($Existing -split '\r?\n')){
        if($line -match '^([A-Za-z0-9_]+)=(.*)$'){$settings[$Matches[1]]=$Matches[2]}
    }
    foreach($entry in @{'vertical_enabled'='1';'vertical_display_mode'='0';'vertical_resolution'='540,960';'vertical_width'='540';'vertical_height'='960'}.GetEnumerator()){
        if(-not $settings.Contains($entry.Key)){$settings[$entry.Key]=$entry.Value}
    }
    if($legacySeed){$settings['windowsize']='800,600';$settings['mastervol']='100'}
    if($Mode -ne 'Play'){$settings['mastervol']='0'}
    $lines=@('[config]')+@($settings.GetEnumerator()|ForEach-Object {'{0}={1}' -f $_.Key,$_.Value})
    return ($lines -join "`r`n")+"`r`n"
}
function Assert-KsmRuntimePath {
    param([string]$Path,[string]$Root)
    $current=[IO.Path]::GetFullPath($Path)
    $boundary=[IO.Path]::GetFullPath($Root).TrimEnd('\')
    if($current -ne $boundary -and -not $current.StartsWith($boundary+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Runtime path escapes project.'}
    while($current.Length -ge $boundary.Length){
        if(Test-Path -LiteralPath $current){
            if((Get-Item -LiteralPath $current -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Linked runtime path rejected: $current"}
        }
        if($current -eq $boundary){break}
        $current=Split-Path $current -Parent
    }
}
function Move-KsmLegacyPlaySkins {
    param([string]$App,[string]$Root)
    $images=Join-Path $App 'imgs'
    if(-not(Test-Path -LiteralPath $images -PathType Container)){return}
    $skins=Join-Path $App 'skins'
    Assert-KsmRuntimePath $skins $Root
    New-Item -ItemType Directory -Path $skins -Force|Out-Null
    $legacy=@(Get-ChildItem -LiteralPath $images -Directory | Where-Object {
        $_.Name -ne 'default' -and ($_.Name -notin @('bg','bg2','editor','gate','icon','jacket') -or (Test-Path -LiteralPath (Join-Path $_.FullName 'skin.json') -PathType Leaf))
    })
    $default=Join-Path $images 'default'
    if($legacy.Count -eq 0 -and -not(Test-Path -LiteralPath $default -PathType Container)){return}
    $backup=Join-Path $Root ('build/skin-layout-backups/'+[guid]::NewGuid().ToString('N'))
    Assert-KsmRuntimePath $backup $Root
    # Refuse collisions before changing any skin folder. User files are never merged or overwritten.
    foreach($folder in $legacy){
        Assert-KsmRuntimePath $folder.FullName $Root
        $destination=Join-Path $skins $folder.Name
        Assert-KsmRuntimePath $destination $Root
        if(Test-Path -LiteralPath $destination){throw "Skin migration collision; both folders preserved: $destination"}
        foreach($item in Get-ChildItem -LiteralPath $folder.FullName -Recurse -Force){Assert-KsmRuntimePath $item.FullName $Root}
    }
    New-Item -ItemType Directory -Path $backup -Force|Out-Null
    foreach($folder in $legacy){
        Copy-Item -LiteralPath $folder.FullName -Destination (Join-Path $backup $folder.Name) -Recurse
        Move-Item -LiteralPath $folder.FullName -Destination (Join-Path $skins $folder.Name)
    }
    if(Test-Path -LiteralPath $default -PathType Container){
        Assert-KsmRuntimePath $default $Root
        foreach($item in Get-ChildItem -LiteralPath $default -Recurse -Force){Assert-KsmRuntimePath $item.FullName $Root}
        Move-Item -LiteralPath $default -Destination (Join-Path $backup 'default')
    }
    [ordered]@{schemaVersion=1;source='imgs';destination='skins';skins=@($legacy|ForEach-Object {$_.Name});defaultBackup=(Join-Path $backup 'default')}|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $backup 'migration.json') -Encoding UTF8
}
function New-KsmPlayRun {
    param($Build)
    $root=Get-KsmProjectRoot
    $run=Join-Path $root 'play-test'
    $app=Join-Path $run 'App'
    Assert-KsmRuntimePath $app $root
    New-Item -ItemType Directory -Path $app -Force|Out-Null
    $guard=[IO.File]::Open((Join-Path $run '.launcher.lock'),[IO.FileMode]::OpenOrCreate,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
    try {
        $exe=Join-Path $app 'ksm-enhanced-vertical.exe'
        $metadata=Join-Path $run 'session.json'
        $session=[pscustomobject]@{schemaVersion=2;id='persistent-play';mode='Play';buildId=$Build.buildId;root=$run;appRoot=$app;executable=$exe;processIds=@();createdAt=[DateTime]::UtcNow.ToString('o');isRunning=$false;updatedFiles=0;pendingBuildId=''}
        if(Test-Path -LiteralPath $exe){
            try {$probe=[IO.File]::Open($exe,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)}
            catch [IO.IOException] {
                # Windows locks a loaded executable. Do not alter ANY files in its runtime.
                if(Test-Path -LiteralPath $metadata){$old=Get-Content -LiteralPath $metadata -Raw|ConvertFrom-Json;$session.buildId=$old.buildId;$session.processIds=@($old.processIds)}else{$session.buildId='existing'}
                $session.isRunning=$true;$session.pendingBuildId=$Build.buildId
                return $session
            }
            $probe.Dispose()
        }
        Move-KsmLegacyPlaySkins -App $app -Root $root
        $files=@([pscustomobject]@{source=$Build.executable;relative='ksm-enhanced-vertical.exe'})
        $source=Join-Path $root 'kshootmania/App'
        foreach($directory in @('assets','dll','engine','imgs','skins','lang','mods','se','shaders','skin_preview','ui')){
            $from=Join-Path $source $directory
            if(-not(Test-Path -LiteralPath $from)){continue}
            Assert-KsmRuntimePath $from $root
            foreach($item in Get-ChildItem -LiteralPath $from -Recurse -Force){
                if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Linked runtime resource rejected: $($item.FullName)"}
                if(-not $item.PSIsContainer){
                    $files+=[pscustomobject]@{source=$item.FullName;relative=[IO.Path]::GetRelativePath($source,$item.FullName)}
                }
            }
        }
        foreach($name in @('bass.dll','bass_fx.dll','bassasio.dll','bassmix.dll','icon.ico','icon_editor.ico')){
            $from=Join-Path $source $name
            if(Test-Path -LiteralPath $from){$files+=[pscustomobject]@{source=$from;relative=$name}}
        }
        # Update only changed managed files. Songs/config/score/cache are never in this list.
        foreach($file in $files){
            $dest=Join-Path $app $file.relative
            Assert-KsmRuntimePath $dest $root
            $same=(Test-Path -LiteralPath $dest -PathType Leaf) -and ((Get-FileHash -LiteralPath $file.source).Hash -eq (Get-FileHash -LiteralPath $dest).Hash)
            if($same){continue}
            New-Item -ItemType Directory -Path (Split-Path $dest -Parent) -Force|Out-Null
            $temporary=$dest+'.'+[guid]::NewGuid().ToString('N')+'.tmp'
            try {Copy-Item -LiteralPath $file.source -Destination $temporary;Move-Item -LiteralPath $temporary -Destination $dest -Force}
            finally {if(Test-Path -LiteralPath $temporary){Remove-Item -LiteralPath $temporary}}
            $session.updatedFiles++
        }
        foreach($retired in @('er_aps_back.png','er_aps_front.png')){
            $oldPath=Join-Path $app ('imgs/'+$retired)
            $authoring=Join-Path $root ('imgs/APS게이지/'+$retired)
            Assert-KsmRuntimePath $oldPath $root
            if((Test-Path -LiteralPath $oldPath -PathType Leaf) -and (Test-Path -LiteralPath $authoring -PathType Leaf) -and
               ((Get-FileHash -LiteralPath $oldPath).Hash -eq (Get-FileHash -LiteralPath $authoring).Hash)){
                Remove-Item -LiteralPath $oldPath
                $session.updatedFiles++
            }
        }
        $retiredInfo=Join-Path $app 'ui/viewport_info_icon.png'
        Assert-KsmRuntimePath $retiredInfo $root
        if((Test-Path -LiteralPath $retiredInfo -PathType Leaf) -and ((Get-FileHash -LiteralPath $retiredInfo).Hash -eq 'C2F7BDC5D76BDD07430FC2344379572F5DE74A20DC39FE800325E4A3E3C478B9')){Remove-Item -LiteralPath $retiredInfo;$session.updatedFiles++}
        New-Item -ItemType Directory -Path (Join-Path $app 'skins') -Force|Out-Null
        $configPath=Join-Path $app 'config.ini' 
        Assert-KsmRuntimePath $configPath $root
        if(-not(Test-Path -LiteralPath $configPath)){
            $existing=''
            $pointer=Join-Path $root 'test-runs/play-profile.json'
            if(Test-Path -LiteralPath $pointer){
                $previous=Get-Content -LiteralPath $pointer -Raw|ConvertFrom-Json
                $previousApp=[IO.Path]::GetFullPath($previous.appRoot)
                $allowed=[IO.Path]::GetFullPath((Join-Path $root 'test-runs')).TrimEnd('\')+'\'
                if($previousApp.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase)){
                    Assert-KsmRuntimePath $previousApp $root
                    $oldConfig=Join-Path $previousApp 'config.ini'
                    if(Test-Path -LiteralPath $oldConfig){$existing=[IO.File]::ReadAllText($oldConfig)}
                    $calibration=Join-Path $previousApp 'audio_calibration_profiles.json'
                    if(Test-Path -LiteralPath $calibration -PathType Leaf){Copy-Item -LiteralPath $calibration -Destination $app}
                }
            }
            [IO.File]::WriteAllText($configPath,(New-KsmConfiguration -Mode Play -Existing $existing),[Text.UTF8Encoding]::new($false))
        }
        Assert-KsmRuntimePath (Join-Path $app 'songs') $root
        New-Item -ItemType Directory -Path (Join-Path $app 'songs') -Force|Out-Null
        $session|ConvertTo-Json -Depth 5|Set-Content -LiteralPath $metadata -Encoding UTF8
        return $session
    } finally {$guard.Dispose()}
}
function New-KsmTestRun {
    param([ValidateSet('Auto','Preview','Play','Prepare')][string]$Mode)
    $root=Get-KsmProjectRoot
    $latest=Join-Path $root 'build/latest.json'
    if(-not(Test-Path -LiteralPath $latest)){throw 'Run scripts/Build.ps1 first.'}
    $build=Get-Content -LiteralPath $latest -Raw|ConvertFrom-Json
    if((Get-FileHash -LiteralPath $build.executable -Algorithm SHA256).Hash -ne $build.sha256){throw 'Build hash changed.'}
    if($Mode -eq 'Play'){return New-KsmPlayRun -Build $build}
    $id=(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N')
    $run=Join-Path $root "test-runs/$id"
    $app=Join-Path $run 'App'
    New-Item -ItemType Directory -Path $app -Force|Out-Null
    $source=Join-Path $root 'kshootmania/App'
    foreach($directory in @('assets','dll','engine','imgs','skins','lang','mods','se','shaders','skin_preview','ui')){
        $from=Join-Path $source $directory
        if(-not(Test-Path -LiteralPath $from)){continue}
        if(@(Get-ChildItem -LiteralPath $from -Recurse -Force | Where-Object {$_.Attributes -band [IO.FileAttributes]::ReparsePoint}).Count -gt 0){throw "Linked runtime resource rejected: $from"}
        Copy-Item -LiteralPath $from -Destination $app -Recurse
    }
    foreach($name in @('bass.dll','bass_fx.dll','bassasio.dll','bassmix.dll','icon.ico','icon_editor.ico')){
        $from=Join-Path $source $name
        if(Test-Path -LiteralPath $from){Copy-Item -LiteralPath $from -Destination $app}
    }
    New-Item -ItemType Directory -Path (Join-Path $app 'skins') -Force|Out-Null
    $exe=Join-Path $app 'ksm-enhanced-vertical.exe'
    Copy-Item -LiteralPath $build.executable -Destination $exe
    [IO.File]::WriteAllText((Join-Path $app 'config.ini'),(New-KsmConfiguration -Mode $Mode),[Text.UTF8Encoding]::new($false))
    New-Item -ItemType Directory -Path (Join-Path $app 'songs') -Force|Out-Null
    $fixtures=Join-Path $root 'tests/fixtures'
    if(Test-Path -LiteralPath $fixtures){Copy-Item -LiteralPath $fixtures -Destination $run -Recurse}
    $session=[pscustomobject]@{schemaVersion=1;id=$id;mode=$Mode;buildId=$build.buildId;root=$run;appRoot=$app;executable=$exe;processIds=@();createdAt=[DateTime]::UtcNow.ToString('o')}
    $session|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $run 'session.json') -Encoding UTF8
    return $session
}
function Start-KsmOwnedProcess {
    param($Session,[string[]]$Arguments=@(),[int]$TimeoutSeconds=60,[switch]$Interactive)
    # Only the process object returned here is observed; no game-name enumeration or termination.
    $info=[Diagnostics.ProcessStartInfo]::new()
    $info.FileName=$Session.executable
    $info.WorkingDirectory=$Session.appRoot
    $info.UseShellExecute=$false
    $info.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
    if($Interactive){$info.WindowStyle=[Diagnostics.ProcessWindowStyle]::Normal}
    foreach($argument in $Arguments){$info.ArgumentList.Add($argument)}
    $quiet=($env:KSM_TEST_QUIET -eq '1') -and -not $Interactive
    if($quiet){$info.RedirectStandardOutput=$true;$info.RedirectStandardError=$true}
    $process=[Diagnostics.Process]::Start($info)
    if($quiet){$stdout=$process.StandardOutput.ReadToEndAsync();$stderr=$process.StandardError.ReadToEndAsync()}
    $Session.processIds=@($Session.processIds)+@($process.Id)
    $Session|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $Session.root 'session.json') -Encoding UTF8
    if($Interactive){return $process}
    if($env:KSM_TEST_PROCESS_METRICS){
        $samples=[Collections.Generic.List[object]]::new()
        $watch=[Diagnostics.Stopwatch]::StartNew()
        while(-not $process.WaitForExit(100)){
            if($watch.Elapsed.TotalSeconds -gt $TimeoutSeconds){throw "Owned test PID $($process.Id) timed out; left running. Session: $($Session.root)"}
            $process.Refresh()
            $samples.Add([ordered]@{seconds=$watch.Elapsed.TotalSeconds;workingSetBytes=$process.WorkingSet64;privateBytes=$process.PrivateMemorySize64})
        }
        $metricsRoot=[IO.Path]::GetFullPath($env:KSM_TEST_PROCESS_METRICS)
        Assert-KsmRuntimePath $metricsRoot (Get-KsmProjectRoot)
        New-Item -ItemType Directory -Path $metricsRoot -Force|Out-Null
        [ordered]@{buildId=$Session.buildId;arguments=$Arguments;elapsedSeconds=$watch.Elapsed.TotalSeconds;samples=$samples}|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $metricsRoot "$($Session.id)-$($process.Id).json") -Encoding UTF8
    }elseif(-not $process.WaitForExit($TimeoutSeconds*1000)){throw "Owned test PID $($process.Id) timed out; left running. Session: $($Session.root)"}
    if($quiet){
        [IO.File]::WriteAllText((Join-Path $Session.root "$($process.Id)-stdout.log"),$stdout.Result)
        [IO.File]::WriteAllText((Join-Path $Session.root "$($process.Id)-stderr.log"),$stderr.Result)
    }
    $process.Refresh()
    if($process.ExitCode -ne 0){throw "Diagnostic failed ($($process.ExitCode)): $Arguments; $($Session.root)"}
}
Export-ModuleMember -Function Get-KsmProjectRoot,New-KsmConfiguration,New-KsmTestRun,Start-KsmOwnedProcess
