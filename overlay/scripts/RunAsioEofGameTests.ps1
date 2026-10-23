param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
$engine=Get-Content -LiteralPath (Join-Path $root 'build/audio-engine-latest.json') -Raw|ConvertFrom-Json
$results=@();$saved=$env:KSM_ASIO_TEST_MODE
try {
 foreach($vertical in @(0,1)) {foreach($mode in @('','48k')) {
  $env:KSM_ASIO_TEST_MODE=$mode
  $size=if($vertical){'720,1280'}else{'640,480'}
  $session=New-AsioFixture -Values @{vertical_enabled=$vertical;windowsize=$size;vertical_resolution=$size;audio_backend='asio:KSM Mock ASIO';automaticmute=0}
  Copy-Item -LiteralPath (Join-Path $engine.runtime 'bassasio.dll') -Destination (Join-Path $session.appRoot 'bassasio.dll') -Force
  Copy-Item -LiteralPath (Join-Path $root 'tests/fixtures/songs/K-Shoot MANIA/magicalkittens') -Destination (Join-Path $session.appRoot 'eof-song') -Recurse
  $report=Invoke-AsioFixture -Session $session -Flags @('--audio-end-game-test','--aps-game-fixture') -Report 'audio-end-game.txt' -Expected 'TOTAL game EOF cases passed' -Timeout 90
  if(@(Get-ChildItem -LiteralPath (Join-Path $session.appRoot 'score') -File -Recurse -ErrorAction SilentlyContinue).Count){throw 'Diagnostic score unexpectedly saved'}
  $results+=[pscustomobject]@{status='PASS';vertical=$vertical;rate=$(if($mode){48000}else{44100});session=$session.root;report=$report}
  Write-Output "REAL CHART EOF PASS vertical=$vertical mode=$mode"
 }}
 $results|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $root 'build/asio-eof-game-tests.json') -Encoding UTF8
} finally {[Environment]::SetEnvironmentVariable('KSM_ASIO_TEST_MODE',$saved,'Process')}
