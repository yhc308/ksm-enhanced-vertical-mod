param([ValidateSet('wide','classic','vertical')][string[]]$Cases=@('wide','classic','vertical'),[switch]$CoreOnly,[ValidateRange(360,2160)][int]$PortraitWidth=720)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'TestWorkspace.psm1') -Force
$python='python'
$verifier=Join-Path $root 'tests/SkinTestVerifier.py'
$results=@()
foreach($case in $Cases){
 $width=if($case -eq 'wide'){1280}elseif($case -eq 'classic'){640}else{$PortraitWidth}
 $height=if($case -eq 'wide'){720}elseif($case -eq 'classic'){480}else{[int]($PortraitWidth*16/9)}
 $session=New-AsioFixture -Values @{vertical_enabled=if($case -eq 'vertical'){1}else{0};windowsize="$width,$height";vertical_resolution="$width,$height";vertical_width=$width;vertical_height=$height;vsync=1;hispeed=713;assisttick=2;viewtiming=2;timing_ms_display=3;timing_ms_color=2}
 Set-Content -LiteralPath (Join-Path $session.appRoot 'skin-test-fixture') -Value 'isolated skin fixture'
 New-Item -ItemType Directory -Path (Join-Path $session.appRoot 'score') -Force|Out-Null
 [IO.File]::WriteAllText((Join-Path $session.appRoot 'score/sentinel.txt'),'USER-SCORE')
 & $python $verifier setup $session.appRoot
 if($LASTEXITCODE -ne 0){throw 'Skin fixture setup failed'}
 foreach($flag in @('--skin-core-test')){
  Start-KsmOwnedProcess -Session $session -Arguments @($flag) -TimeoutSeconds 60
  $failure=Join-Path $session.appRoot 'skin-test-failure.txt'
  if(Test-Path -LiteralPath $failure){throw (Get-Content -LiteralPath $failure -Raw)}
  if(-not(Test-Path -LiteralPath (Join-Path $session.appRoot 'core-passed.txt'))){throw "Missing core report: $($session.root)"}
  Write-Output "$case`: $(Get-Content -LiteralPath (Join-Path $session.appRoot 'core-passed.txt'))"
 }
 if(-not $CoreOnly){
  $flags=@('--skin-smoke-test');$save=$case -eq 'classic';if($save){$flags+='--skin-smoke-save'}
  Start-KsmOwnedProcess -Session $session -Arguments $flags -TimeoutSeconds 60
  if(Test-Path -LiteralPath $failure){throw (Get-Content -LiteralPath $failure -Raw)}
  if(-not(Test-Path -LiteralPath (Join-Path $session.appRoot 'smoke-passed.txt'))){throw "Missing smoke report: $($session.root)"}
  & $python $verifier verify $session.appRoot $(if($save){'save'}else{'cancel'})
  if($LASTEXITCODE -ne 0){throw "Skin verification failed: $($session.root)"}
 }
 $results+=[pscustomobject]@{case=$case;status='PASS';coreOnly=[bool]$CoreOnly;session=$session.root}
 $results|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $root 'build/skin-tests-latest.json') -Encoding UTF8
}

