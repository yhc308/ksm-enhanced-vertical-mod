param(
    [Parameter(Mandatory=$true)][string]$ProjectRoot,
    [Parameter(Mandatory=$true)][string]$OriginalGameDirectory,
    [Parameter(Mandatory=$true)][string]$BuildInformation
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
$run=Join-Path $root ('build/payload-installation-tests/'+[guid]::NewGuid().ToString('N'))
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
$before=@{}
foreach($file in Get-ChildItem -LiteralPath $app -Recurse -File){$before[[IO.Path]::GetRelativePath($app,$file.FullName).Replace('\','/')]=(Get-FileHash -LiteralPath $file.FullName).Hash}
$protected=@('kshootmania.exe','config.ini','songs/user/chart.ksh','score/user.ksc','skins/User Skin/skin.json')
$latest=Join-Path $root 'build/latest.json'
$saved=[IO.File]::ReadAllBytes($latest)
$installer=Join-Path $root 'scripts/ModInstaller.ps1'
try{
    [IO.File]::WriteAllBytes($latest,[IO.File]::ReadAllBytes($BuildInformation))
    $payload=& (Join-Path $root 'scripts/New-DevelopmentPayload.ps1')
    $payload=[string](@($payload)[-1])
    & $installer -Action Install -GameDirectory $app -PackageDirectory $payload -AllowDevelopment|Out-Null
    & $installer -Action Verify -GameDirectory $app|Out-Null
    foreach($relative in $protected){if((Get-FileHash -LiteralPath (Join-Path $app $relative)).Hash -ne $before[$relative]){throw "Install changed user/original file: $relative"}}
    $session=[pscustomobject]@{id='installed-normal';root=$run;appRoot=$app;executable=(Join-Path $app 'ksm-enhanced-vertical.exe');processIds=@();buildId=$build.buildId}
    $process=Start-KsmOwnedProcess -Session $session -Interactive
    try{
        $watch=[Diagnostics.Stopwatch]::StartNew();$ready=$false
        while($watch.Elapsed.TotalSeconds -lt 30){
            if($process.WaitForExit(100)){throw 'Installed normal executable exited during startup.'}
            $process.Refresh()
            if($process.MainWindowTitle -eq 'K-Shoot Mania V2.1.0 - Enhanced Vertical Mod (Unofficial Build)'){$ready=$true;break}
        }
        if(-not $ready){throw 'Installed normal game window was not ready.'}
        if($process.WaitForExit(4000)){throw 'Installed normal game exited during the smoke interval.'}
        if(-not $process.CloseMainWindow() -or -not $process.WaitForExit(30000) -or $process.ExitCode -ne 0){throw 'Installed normal game did not close gracefully.'}
    }catch{
        if(-not $process.HasExited -and $process.Path -ieq $session.executable){$process.Kill();$process.WaitForExit()}
        throw
    }
    # The game's own normal config persistence is distinct from installer mutation.
    $configAfterRuntime=(Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash
    & $installer -Action Verify -GameDirectory $app|Out-Null
    & $installer -Action Remove -GameDirectory $app|Out-Null
    foreach($relative in $before.Keys){
        $expected=if($relative -eq 'config.ini'){$configAfterRuntime}else{$before[$relative]}
        if((Get-FileHash -LiteralPath (Join-Path $app $relative)).Hash -ne $expected){throw "Removal did not preserve/restore: $relative"}
    }
    if(Test-Path -LiteralPath (Join-Path $app 'ksm-enhanced-vertical.exe')){throw 'Mod executable remains after removal.'}
    [ordered]@{status='PASS';buildId=$build.buildId;diagnostics=$false;upstreamHash=$probe.executableHash;payload=$payload;fixture=$run;originalFilesRestored=$before.Count;installProtectedFiles=$protected;normalStartup=$true;gracefulExit=$true;userSkinsAndRecordsPreserved=$true;gameConfigAfterRuntimePreserved=$true}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $root 'build/payload-installation-smoke.json') -Encoding UTF8
    Write-Output "PAYLOAD INSTALLATION PASS: $run"
}finally{[IO.File]::WriteAllBytes($latest,$saved)}
