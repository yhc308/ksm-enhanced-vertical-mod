param([ValidateSet(640,800,1280,1920)][int]$LandscapeWidth=1280,[switch]$LandscapeOnly,[int]$LandscapeHeight=0,[ValidateSet(720,1080)][int]$PortraitWidth=720)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
Import-Module (Join-Path $root 'scripts/TestWorkspace.psm1') -Force
$python='python'
$results=@()
foreach($vertical in $(if($LandscapeOnly){@(0)}else{@(0,1)})){
 $w=if($vertical){$PortraitWidth}else{$LandscapeWidth};$h=if($vertical){[int]($PortraitWidth*16/9)}elseif($LandscapeHeight -gt 0){$LandscapeHeight}elseif($LandscapeWidth -eq 1280){720}else{[int]($LandscapeWidth*3/4)}
 $session=New-AsioFixture -Values @{vertical_enabled=$vertical;windowsize="$w,$h";vertical_resolution="$w,$h";vertical_width=$w;vertical_height=$h;vsync=1;hispeed="850"}
 Set-Content -LiteralPath (Join-Path $session.appRoot 'skin-test-fixture') -Value 'isolated viewport fixture'
 & $python (Join-Path $root 'tests/SkinTestVerifier.py') setup $session.appRoot
 if($LASTEXITCODE -ne 0){throw 'Viewport fixture setup failed'}
 Set-Content -LiteralPath (Join-Path $session.appRoot 'skin_preset.json') -Value '{"folder":"default","missing":"default","background1Brightness":100,"background2Opacity":100,"laserWidthPercent":100,"noteHeightPercent":100,"indicatorPortrait":1,"indicatorLandscape":true,"keyBeamMode":1}' -Encoding UTF8
 $skinBefore=(Get-FileHash -LiteralPath (Join-Path $session.appRoot 'skin_preset.json')).Hash
 Start-KsmOwnedProcess -Session $session -Arguments @('--skin-smoke-test','--viewport-options-smoke') -TimeoutSeconds 100
 & $python (Join-Path $root 'tests/ViewportFrameVerifier.py') $session.appRoot
 if($LASTEXITCODE -ne 0){throw 'Viewport rendered frame parity failed'}
 $report=Join-Path $session.appRoot 'viewport-options-passed.txt'
 if(-not(Test-Path -LiteralPath $report)){throw "Missing viewport options report: $($session.root)"}
 if($skinBefore -ne (Get-FileHash -LiteralPath (Join-Path $session.appRoot 'skin_preset.json')).Hash){throw 'Cancelled draft changed skin_preset.json'}
 $name=if($vertical){'portrait'}elseif($LandscapeWidth -eq 1280 -and $h -eq 720){'landscape'}else{"landscape-$LandscapeWidth"}
 foreach($image in @('viewport-options-group','viewport-depth-details','viewport-indicator-off','viewport-indicator-both','viewport-back-group')){
  Copy-Item -LiteralPath (Join-Path $session.appRoot "$image.png") -Destination (Join-Path $root "reference/$image-$name.png") -Force
 }
 $localization=Join-Path $root ('reference/mod-localization/'+$session.buildId)
 New-Item -ItemType Directory -Path $localization -Force|Out-Null
 foreach($image in Get-ChildItem -LiteralPath $session.appRoot -Filter 'viewport-l10n-*.png'){Copy-Item -LiteralPath $image.FullName -Destination $localization -Force}
 $results+=[pscustomobject]@{status='PASS';vertical=[bool]$vertical;session=$session.root;report=(Get-Content -LiteralPath $report -Raw);cancelPreservedSettings=$true}
 $results | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $root 'build/viewport-options-tests.json') -Encoding UTF8
 Write-Output "VIEWPORT OPTIONS $name PASS"
}
