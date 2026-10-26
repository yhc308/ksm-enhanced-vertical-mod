$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$app=Join-Path $root 'kshootmania/App'
$out=Join-Path $root 'test-output/wall-v10/performance'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$hash=(Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash
foreach($case in @('neutral','zoom','bottom','combined')) {
 foreach($run in 1..3) {
  foreach($mode in @('off','on')) {
   $flags='--vertical-preview-hud --vertical-preview-zero --vertical-preview-bg night --vertical-preview-layer arrow --vertical-preview-1080 --vertical-preview-benchmark'
   if($case -eq 'zoom'){$flags+=' --vertical-preview-zoom-top 100'}
   if($case -eq 'bottom'){$flags+=' --vertical-preview-zoom-bottom 100'}
   if($case -eq 'combined'){$flags+=' --vertical-preview-bench-cycle'}
   if($mode -eq 'off'){$flags+=' --vertical-preview-warp-off'}
   $p=Start-Process (Join-Path $app 'kshootmania.exe') -WorkingDirectory $app -ArgumentList $flags -WindowStyle Hidden -PassThru
   if(-not $p.WaitForExit(45000)){ $p.Kill();throw 'Benchmark timeout' }
   if($p.ExitCode -ne 0){throw 'Benchmark failed'}
   Copy-Item -LiteralPath (Join-Path $app 'vertical-warp-benchmark.csv') -Destination (Join-Path $out "$case-$mode-$run.csv")
   Write-Output "$case $mode $run completed"
  }
 }
}
if((Get-FileHash -LiteralPath (Join-Path $app 'config.ini')).Hash -ne $hash){throw 'Benchmark changed configuration'}
Write-Output 'PASS: configuration preserved'
