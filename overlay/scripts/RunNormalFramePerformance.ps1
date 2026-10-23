param(
    [Parameter(Mandatory=$true)][string]$BaselineBuild,
    [Parameter(Mandatory=$true)][string]$CandidateBuild,
    [Parameter(Mandatory=$true)][string]$PresentMon
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$identity=[Security.Principal.WindowsIdentity]::GetCurrent()
$principal=[Security.Principal.WindowsPrincipal]::new($identity)
if(-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){throw 'PresentMon ETW capture requires a separately approved elevated run.'}
if((Get-FileHash -LiteralPath $PresentMon).Hash -ne 'B2A706BC6AD475749E3B7E3409263AA1E6906D45BDCF993F6DBC0F660188F1AF'){throw 'PresentMon 2.6.0 publisher digest mismatch.'}
Import-Module (Join-Path $root 'scripts/TestWorkspace.psm1') -Force
$builds=@{baseline=(Get-Content -LiteralPath $BaselineBuild -Raw|ConvertFrom-Json);candidate=(Get-Content -LiteralPath $CandidateBuild -Raw|ConvertFrom-Json)}
foreach($build in $builds.Values){if($build.configuration -ne 'Release' -or (Get-FileHash -LiteralPath $build.executable).Hash -ne $build.sha256){throw 'Saved Release executable hash mismatch.'}}
if($builds.candidate.diagnostics){throw 'Candidate must be the normal executable with diagnostics disabled.'}
$output=Join-Path $root ('build/normal-frame-performance/'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $output -Force|Out-Null
[ordered]@{protocol='Native chart-CLI autoplay through actual GameMain; external DXGI ETW capture';originalModified=$false;processFilter='Exact PID of this script-owned game only';samples='8 seconds after 3 seconds warmup';repetitions=3;resolutions=@('1280x720','720x1280');frameLimit=300;scope='Owned isolated runtime only; no driver/service/settings installation';builds=$builds;toolSha256=(Get-FileHash -LiteralPath $PresentMon).Hash}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $output 'protocol.json') -Encoding UTF8
$latest=Join-Path $root 'build/latest.json';$saved=[IO.File]::ReadAllBytes($latest)
$results=[Collections.Generic.List[object]]::new()
try{
    [IO.File]::WriteAllBytes($latest,[IO.File]::ReadAllBytes($CandidateBuild))
    foreach($vertical in @(0,1)){
        $size=if($vertical){'720,1280'}else{'1280,720'}
        $width=if($vertical){720}else{1280};$height=if($vertical){1280}else{720}
        $session=New-KsmTestRun -Mode Prepare
        $seed="[config]`nmastervol=0`naudio_backend=default`ncurrentlang=Japanese`nvertical_enabled=$vertical`nwindowsize=$size`nvertical_resolution=$size`nvertical_width=$width`nvertical_height=$height`nvsync=0;300`nhispeed=450`n"
        $config=New-KsmConfiguration -Mode Auto -Existing $seed
        $skinPath=Join-Path $session.appRoot 'skin_preset.json';$skin=if(Test-Path -LiteralPath $skinPath){[IO.File]::ReadAllBytes($skinPath)}else{$null}
        foreach($iteration in 1..3){
            $order=if(($iteration+$vertical)%2){@('baseline','candidate')}else{@('candidate','baseline')}
            foreach($side in $order){
                $build=$builds[$side]
                $boundary=[IO.Path]::GetFullPath($session.appRoot).TrimEnd([IO.Path]::DirectorySeparatorChar)+[IO.Path]::DirectorySeparatorChar
                if(-not [IO.Path]::GetFullPath($session.executable).StartsWith($boundary,[StringComparison]::OrdinalIgnoreCase)){throw 'Owned executable path escapes fixture.'}
                Copy-Item -LiteralPath $build.executable -Destination $session.executable -Force
                if((Get-FileHash -LiteralPath $session.executable).Hash -ne $build.sha256){throw 'Runtime executable copy mismatch.'}
                [IO.File]::WriteAllText((Join-Path $session.appRoot 'config.ini'),$config,[Text.UTF8Encoding]::new($false))
                if($null -ne $skin){[IO.File]::WriteAllBytes($skinPath,$skin)}elseif(Test-Path -LiteralPath $skinPath){Remove-Item -LiteralPath $skinPath}
                $gameInfo=[Diagnostics.ProcessStartInfo]::new();$gameInfo.FileName=$session.executable;$gameInfo.WorkingDirectory=$session.appRoot;$gameInfo.UseShellExecute=$false;$gameInfo.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
                foreach($arg in @('-auto','-from','0',(Join-Path $session.appRoot 'mods/enhanced/judge_setting/Preview_Piano.kson'))){$gameInfo.ArgumentList.Add($arg)}
                $game=[Diagnostics.Process]::Start($gameInfo)
                $monitor=$null
                try{
                    $watch=[Diagnostics.Stopwatch]::StartNew();$ready=$false
                    while($watch.Elapsed.TotalSeconds -lt 30){
                        if($game.WaitForExit(100)){throw 'Native game exited before capture.'};$game.Refresh()
                        if($game.MainWindowTitle -eq 'K-Shoot Mania V2.1.0 - Enhanced Vertical Mod (Unofficial Build)'){$ready=$true;break}
                    }
                    if(-not $ready){throw 'Native game window was not ready.'}
                    $csv=Join-Path $output "$side-$iteration-$vertical.csv"
                    $info=[Diagnostics.ProcessStartInfo]::new();$info.FileName=$PresentMon;$info.UseShellExecute=$false;$info.CreateNoWindow=$true;$info.RedirectStandardOutput=$true;$info.RedirectStandardError=$true
                    foreach($arg in @('--process_id',[string]$game.Id,'--session_name',('KsmRefactor-'+[guid]::NewGuid().ToString('N')),'--output_file',$csv,'--v1_metrics','--no_console_stats','--no_track_input','--no_track_gpu','--no_track_display','--delay','3','--timed','8','--terminate_after_timed')){$info.ArgumentList.Add($arg)}
                    $monitor=[Diagnostics.Process]::Start($info)
                    $stdout=$monitor.StandardOutput.ReadToEndAsync();$stderr=$monitor.StandardError.ReadToEndAsync()
                    if(-not $monitor.WaitForExit(25000)){throw 'Owned frame capture timed out.'}
                    [IO.File]::WriteAllText((Join-Path $output "$side-$iteration-$vertical.log"),$stdout.Result+"`n"+$stderr.Result)
                    if($monitor.ExitCode -ne 0){throw 'PresentMon capture failed; inspect retained log.'}
                    if($game.HasExited){throw 'Native game exited before capture completed.'}
                    if(-not $game.CloseMainWindow() -or -not $game.WaitForExit(30000) -or $game.ExitCode -ne 0){throw 'Native game failed graceful shutdown.'}
                    $data=@(Import-Csv -LiteralPath $csv|Where-Object {[int]$_.ProcessID -eq $game.Id -and [double]$_.MsBetweenPresents -gt 0})
                    if($data.Count -lt 500){throw 'Insufficient native presented frames.'}
                    $values=@($data|ForEach-Object {[double]$_.MsBetweenPresents}|Sort-Object)
                    $p99=$values[[int][Math]::Floor(($values.Count-1)*.99)]
                    $results.Add([ordered]@{side=$side;iteration=$iteration;vertical=$vertical;mode='native-play';frames=$values.Count;mean_ms=($values|Measure-Object -Average).Average;p99_ms=$p99;over_8_333ms=@($values|Where-Object {$_ -gt 1000.0/120}).Count;session=$session.root;processId=$game.Id;csv=$csv})
                    $results|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $output 'raw.json') -Encoding UTF8
                    if(@(Get-ChildItem -LiteralPath $session.appRoot -Recurse -File -Filter '*.ksc').Count){throw 'Native test-play wrote score records.'}
                    Write-Output "NATIVE FRAME CAPTURE $side iteration=$iteration vertical=$vertical frames=$($values.Count)"
                }finally{
                    if($null -ne $monitor -and -not $monitor.HasExited -and $monitor.Path -ieq $PresentMon){$monitor.Kill();$monitor.WaitForExit()}
                    if(-not $game.HasExited -and $game.Path -ieq $session.executable){$game.Kill();$game.WaitForExit()}
                }
            }
        }
    }
    foreach($side in @('baseline','candidate')){foreach($iteration in 1..3){
        @($results|Where-Object {$_.side -eq $side -and $_.iteration -eq $iteration})|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $output "$side-$iteration.json") -Encoding UTF8
    }}
    $before=@(1..3|ForEach-Object {Join-Path $output "baseline-$_.json"});$after=@(1..3|ForEach-Object {Join-Path $output "candidate-$_.json"})
    python (Join-Path $PSScriptRoot 'CompareFramePerformance.py') --baseline @before --candidate @after --output (Join-Path $output 'comparison.json')
    if($LASTEXITCODE){throw "Native frame gate failed: $output"}
    Write-Output "NATIVE FRAME PERFORMANCE PASS: $output"
}finally{[IO.File]::WriteAllBytes($latest,$saved)}
