param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
$engine=Get-Content -LiteralPath (Join-Path $root 'build/audio-engine-latest.json') -Raw|ConvertFrom-Json
$session=New-AsioFixture
$results=@()
function Run([string]$Flag,[string]$Report,[string]$Expected='TOTAL \d+ checks passed'){
 $path=Invoke-AsioFixture -Session $session -Flags @($Flag) -Report $Report -Expected $Expected
 $script:results+=[pscustomobject]@{flag=$Flag;status='PASS';report=$path}
 Write-Output ((Get-Content -LiteralPath $path|Where-Object {$_ -match 'TOTAL|stages='}|Select-Object -Last 1))
}
$oldMode=$env:KSM_ASIO_TEST_MODE;$oldLatency=$env:KSM_ASIO_TEST_LATENCY
try {
 Run '--timing-hud-smoke-test' 'timing-hud-test.txt'
 Run '--audio-calibration-smoke-test' 'audio-calibration-test.txt'
 Copy-Item -LiteralPath (Join-Path $engine.runtime 'bassasio.dll') -Destination (Join-Path $session.appRoot 'bassasio.dll') -Force
 Copy-Item -LiteralPath (Join-Path $engine.runtime 'test-stereo.wav') -Destination (Join-Path $session.appRoot 'timing-fixture.wav') -Force
 Copy-Item -LiteralPath (Join-Path $root 'tests/fixtures/songs/K-Shoot MANIA/magicalkittens/magicalkittens.ogg') -Destination (Join-Path $session.appRoot 'eof-regression.ogg') -Force
 foreach($mode in @('','48k')){foreach($latency in @(64,128,512)){
  $env:KSM_ASIO_TEST_MODE=$mode;$env:KSM_ASIO_TEST_LATENCY=[string]$latency
  Set-AsioFixtureConfig -Session $session -Values @{audio_backend='asio:KSM Mock ASIO'}
  Run '--audio-timing-smoke-test' 'audio-timing-test.txt'
  Copy-Item -LiteralPath (Join-Path $session.appRoot 'audio-timing-test.txt') -Destination (Join-Path $session.root $('audio-'+$(if($mode){48000}else{44100})+'-latency-'+$latency+'.txt'))
  $results[-1].report=Join-Path $session.root $('audio-'+$(if($mode){48000}else{44100})+'-latency-'+$latency+'.txt')
 }}
 Copy-Item -LiteralPath (Join-Path $root 'kshootmania/App/bassasio.dll') -Destination (Join-Path $session.appRoot 'bassasio.dll') -Force
 $env:KSM_ASIO_TEST_MODE=''
 Set-AsioFixtureConfig -Session $session
 $path=Invoke-AsioFixture -Session $session -Flags @('--audio-options-smoke-test','--audio-output-switch-smoke-test') -Report 'audio-switch-smoke-status.txt' -Expected 'stages=3, error=, BGMPlaying=true, active=default'
 $results+=[pscustomobject]@{flag='deferred-selection-smoke';status='PASS';report=$path}
 [ordered]@{status='PASS';session=$session.root;cases=$results}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $root 'build/audio-integration-latest.json') -Encoding UTF8
} finally {[Environment]::SetEnvironmentVariable('KSM_ASIO_TEST_MODE',$oldMode,'Process');[Environment]::SetEnvironmentVariable('KSM_ASIO_TEST_LATENCY',$oldLatency,'Process')}