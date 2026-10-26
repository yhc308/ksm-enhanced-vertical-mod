param([ValidateSet('core','landscape','portrait')][string]$Group='core')
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
Import-Module (Join-Path $root 'scripts/TestWorkspace.psm1') -Force
$python='python'
if($Group -eq 'core'){
 $session=New-AsioFixture
 $report=Invoke-AsioFixture -Session $session -Flags @('--ex-score-test') -Report 'ex-score-test.txt' -Expected 'TOTAL \d+ EX checks passed' -Timeout 45
 $session.root|Set-Content -LiteralPath (Join-Path $root 'build/ex-score-core-session.txt') -Encoding UTF8
 Write-Output (Get-Content -LiteralPath $report -Tail 1)
 return
}
$cases=if($Group -eq 'landscape'){@(@(640,480),@(1280,720),@(1920,1080))}else{@(@(720,1280),@(1080,1920))}
$results=@()
foreach($size in $cases){
 $w=$size[0];$h=$size[1]
 $session=New-AsioFixture -Values @{vertical_enabled=[int]($h -gt $w);windowsize="$w,$h";vertical_resolution="$w,$h";currentdir='K-Shoot MANIA';currentitem=1;currentlevel=1;automaticmute=0;vsync=1} -Song
 foreach($state in @('percent','minus','max','none')){
  Set-AsioFixtureConfig -Session $session -Values @{vertical_enabled=[int]($h -gt $w);windowsize="$w,$h";vertical_resolution="$w,$h";currentdir='K-Shoot MANIA';currentitem=1;currentlevel=1;automaticmute=0;vsync=1}
  Start-KsmOwnedProcess -Session $session -Arguments @('--timing-menu-capture',"--ex-example-$state") -TimeoutSeconds 30
  Start-KsmOwnedProcess -Session $session -Arguments @('--result-detail-capture',"--ex-example-$state") -TimeoutSeconds 30
  $dest=Join-Path $root "reference/ex-score/$w`x$h/$state"
  New-Item -ItemType Directory -Path $dest -Force|Out-Null
  Copy-Item -LiteralPath (Join-Path $session.appRoot 'timing-captures/menu-0.png') -Destination (Join-Path $dest 'select.png') -Force
  Copy-Item -LiteralPath (Join-Path $session.appRoot 'result-detail.png') -Destination (Join-Path $dest 'result.png') -Force
  foreach($n in @('menu','result')){Copy-Item -LiteralPath (Join-Path $session.appRoot "$n-ex-layout.json") -Destination (Join-Path $dest "$n-layout.json") -Force}
  & $python (Join-Path $root 'tests/ExScoreVerifier.py') $session.appRoot $dest $w $h $state
  if($LASTEXITCODE){throw "EX visual verification failed $w/$h/$state"}
  $results+=[pscustomobject]@{status='PASS';width=$w;height=$h;state=$state;session=$session.root;captures=$dest}
  $results|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $root "build/ex-score-$Group-tests.json") -Encoding UTF8
  Write-Output "EX UI PASS $w/$h/$state"
 }
}
