param([ValidateSet('hidden','idle','overlap')][string[]]$Modes=@('hidden','idle','overlap'),[ValidateSet(0,1)][int[]]$Orientations=@(0,1))
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
Import-Module (Join-Path $root 'scripts/TestWorkspace.psm1') -Force
$results=@()
foreach($vertical in $Orientations){
 $size=if($vertical){'1080,1920'}else{'1280,720'}
 $session=New-AsioFixture -Values @{vertical_enabled=$vertical;windowsize=$size;vertical_resolution=$size;vsync=0}
 foreach($mode in $Modes){
  $flags=@('--vertical-preview-hud','--vertical-preview-benchmark','--aps-performance-fixture',"--aps-performance-$mode")
  $flags+=if($vertical){'--vertical-preview-1080'}else{'--vertical-preview-landscape'}
  Start-KsmOwnedProcess -Session $session -Arguments $flags -TimeoutSeconds 60
  $source=Join-Path $session.appRoot 'vertical-warp-benchmark.csv'
  $data=Import-Csv -LiteralPath $source|Select-Object -First 1
  if([int]$data.samples -ne 960){throw 'APS benchmark ended before collecting all 960 measured frames.'}
  $copy=Join-Path $root "reference/aps/performance-$vertical-$mode.csv";Copy-Item -LiteralPath $source -Destination $copy -Force
  $results+=[pscustomobject]@{vertical=$vertical;mode=$mode;mean_ms=[double]$data.mean_ms;p99_ms=[double]$data.p99_ms;over_8_333ms=[int]$data.over_8_333ms;session=$session.root}
  $results|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $root 'build/aps-performance.json') -Encoding UTF8
  Write-Output "APS PERFORMANCE vertical=$vertical mode=$mode mean=$($data.mean_ms)ms p99=$($data.p99_ms)ms"
 }
}
