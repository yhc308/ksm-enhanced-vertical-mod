param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$latest=Get-Content -LiteralPath (Join-Path $root 'build/latest.json') -Raw|ConvertFrom-Json
$directory=Join-Path $root ('build/audio-tests/'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $directory -Force|Out-Null
$oldLibrary=$env:KSM_AUDIO_TEST_LIBRARY_DIR;$oldRuntime=$env:KSM_AUDIO_TEST_RUNTIME_DIR;$oldMode=$env:KSM_ASIO_TEST_MODE
$results=@()
try {
 $env:KSM_AUDIO_TEST_LIBRARY_DIR=Split-Path $latest.executable -Parent
 $env:KSM_AUDIO_TEST_RUNTIME_DIR=$directory
 & (Join-Path $PSScriptRoot 'BuildAudioTests.cmd') 2>&1|Tee-Object -FilePath (Join-Path $directory 'build.log')
 if($LASTEXITCODE -ne 0){throw 'Audio test build failed'}
 function Run([string]$Name,[string[]]$Arguments=@(),[string]$Mode=''){
  $env:KSM_ASIO_TEST_MODE=$Mode
  $info=[Diagnostics.ProcessStartInfo]::new()
  $info.FileName=Join-Path $directory 'audio-output-test.exe';$info.WorkingDirectory=$directory
  $info.UseShellExecute=$false;$info.WindowStyle='Hidden';$info.RedirectStandardOutput=$true;$info.RedirectStandardError=$true
  foreach($argument in $Arguments){$info.ArgumentList.Add($argument)}
  $p=[Diagnostics.Process]::Start($info);$stdout=$p.StandardOutput.ReadToEndAsync();$stderr=$p.StandardError.ReadToEndAsync()
  if(-not $p.WaitForExit(60000)){throw "Owned audio test PID $($p.Id) timed out; left running."}
  $out=$stdout.GetAwaiter().GetResult();$err=$stderr.GetAwaiter().GetResult()
  [IO.File]::WriteAllText((Join-Path $directory ($Name+'.txt')),$out+$err)
  if($p.ExitCode -ne 0){throw "Audio test $Name failed: $out $err"}
  $script:results+=[pscustomobject]@{name=$Name;status='PASS';report=Join-Path $directory ($Name+'.txt')}
  Write-Output "$Name`: $($out.Trim())"
 }
 Run 'mix-44100';Run 'mix-48000' @() '48k'
 Run 'concurrency' @('--concurrency')
 Run 'rate-policy' @('--rate-policy')
 Run 'teardown' @('--teardown-stress')
 Run 'clock' @('--latency-clock')
 foreach($mode in @('missing','busy','mono','bad-rate','start-fail')){Run ('failure-'+$mode) @('--failure') $mode}
 [ordered]@{status='PASS';buildId=$latest.buildId;runtime=$directory;cases=$results}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $root 'build/audio-engine-latest.json') -Encoding UTF8
} finally {
 [Environment]::SetEnvironmentVariable('KSM_AUDIO_TEST_LIBRARY_DIR',$oldLibrary,'Process')
 [Environment]::SetEnvironmentVariable('KSM_AUDIO_TEST_RUNTIME_DIR',$oldRuntime,'Process')
 [Environment]::SetEnvironmentVariable('KSM_ASIO_TEST_MODE',$oldMode,'Process')
}