param([ValidateSet('landscape','portrait')][string]$Group='landscape')
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
Import-Module (Join-Path $root 'scripts/TestWorkspace.psm1') -Force
$python='python'
$sizes=if($Group -eq 'landscape'){@(@(640,480),@(1280,720),@(1920,1080))}else{@(@(720,1280),@(1080,1920))}
$results=@()
foreach($size in $sizes){
 $w=$size[0];$h=$size[1]
 $session=New-AsioFixture -Values @{vertical_enabled=[int]($h -gt $w);windowsize="$w,$h";vertical_resolution="$w,$h";automaticmute=0;vsync=1} -Song
 foreach($state in @('positive','negative','zero','large','hidden')){
  Set-AsioFixtureConfig -Session $session -Values @{vertical_enabled=[int]($h -gt $w);windowsize="$w,$h";vertical_resolution="$w,$h";automaticmute=0;vsync=1}
  $flags=@('--result-detail-capture','--ex-example-max')
  if($state -ne 'hidden'){$flags+="--best-checker-$state"}
  Start-KsmOwnedProcess -Session $session -Arguments $flags -TimeoutSeconds 30
  $dest=Join-Path $root "reference/result-best-score/$w`x$h/$state"
  New-Item -ItemType Directory -Path $dest -Force|Out-Null
  foreach($name in @('result-detail.png','result-best-layout.json','result-ex-layout.json')){Copy-Item -LiteralPath (Join-Path $session.appRoot $name) -Destination (Join-Path $dest $name) -Force}
  & $python (Join-Path $root 'tests/ResultBestScoreVerifier.py') $session.appRoot $dest $w $h $state
  if($LASTEXITCODE){throw "Best score pixels failed: $w/$h/$state"}
  $results+=[pscustomobject]@{status='PASS';width=$w;height=$h;state=$state;session=$session.root;captures=$dest}
  $results|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $root "build/result-best-score-$Group.json") -Encoding UTF8
  Write-Output "BEST SCORE UI PASS $w/$h/$state"
 }
}
