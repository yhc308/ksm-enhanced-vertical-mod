param([Parameter(Mandatory=$true)][string]$ProjectRoot)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath($ProjectRoot)
Import-Module (Join-Path $root 'scripts/TestWorkspace.psm1') -Force
$build=Get-Content -LiteralPath (Join-Path $root 'build/latest.json') -Raw|ConvertFrom-Json
if($build.diagnostics){throw 'Normal smoke requires a build with diagnostics disabled.'}
$results=[Collections.Generic.List[object]]::new()
foreach($vertical in @(0,1)){
 foreach($kind in @('title','chart-cli')){
  $session=New-KsmTestRun -Mode Prepare
  $width=if($vertical){720}else{1280};$height=if($vertical){1280}else{720}
  $seed="[config]`nmastervol=0`naudio_backend=default`ncurrentlang=Japanese`nvertical_enabled=$vertical`nwindowsize=$width,$height`nvertical_resolution=$width,$height`nvertical_width=$width`nvertical_height=$height`n"
  [IO.File]::WriteAllText((Join-Path $session.appRoot 'config.ini'),(New-KsmConfiguration -Mode Auto -Existing $seed),[Text.UTF8Encoding]::new($false))
  $flags=@()
  if($kind -eq 'chart-cli'){$flags=@('-auto','-from','0',(Join-Path $session.appRoot 'mods/enhanced/judge_setting/Preview_Piano.kson'))}
  $process=Start-KsmOwnedProcess -Session $session -Arguments $flags -Interactive
  try{
   $watch=[Diagnostics.Stopwatch]::StartNew()
   $ready=$false;$readyTitle=''
   while($watch.Elapsed.TotalSeconds -lt 30){
    if($process.WaitForExit(100)){throw 'Normal process exited before startup verification.'}
    $process.Refresh()
    if(($build.configuration -eq 'Release' -and $process.MainWindowTitle -eq 'K-Shoot Mania V2.1.0 - Enhanced Vertical Mod (Unofficial Build)') -or ($build.configuration -eq 'Debug' -and $process.MainWindowTitle.StartsWith('K-Shoot Mania V2.1.0 - Enhanced Vertical Mod (Unofficial Build) (Debug Build) | '))){$readyTitle=$process.MainWindowTitle;$ready=$true;break}
   }
   if(-not $ready){throw "Expected normal window was not found; PID $($process.Id); observed title: $($process.MainWindowTitle)"}
   if($process.WaitForExit(6000)){throw 'Normal process exited during the smoke interval.'}
   if(-not $process.CloseMainWindow()){throw 'Owned normal test window did not accept close.'}
   if(-not $process.WaitForExit(30000)){throw 'Normal test did not shut down gracefully.'}
   if($process.ExitCode -ne 0){throw "Normal process failed: $($process.ExitCode)"}
   $scores=@(Get-ChildItem -LiteralPath $session.appRoot -Recurse -Filter '*.ksc' -File)
   if($scores.Count){throw 'Normal title/test-play smoke wrote score records.'}
   $results.Add([ordered]@{status='PASS';configuration=$build.configuration;diagnostics=$false;vertical=[bool]$vertical;kind=$kind;windowTitle=$readyTitle;session=$session.root;gracefulExit=$true;scoreFiles=0})
   Write-Output "NORMAL $($build.configuration) $width`x$height $kind PASS"
  }catch{
   if(-not $process.HasExited -and $process.Path -ieq $session.executable){$process.Kill();$process.WaitForExit()}
   $results.Add([ordered]@{status='FAIL';vertical=[bool]$vertical;kind=$kind;session=$session.root;error=$_.Exception.Message})
   throw
  }finally{
   $results|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $root "build/normal-smoke-$($build.configuration).json") -Encoding UTF8
  }
 }
}
