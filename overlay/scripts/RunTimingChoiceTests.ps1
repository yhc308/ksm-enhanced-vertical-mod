param([switch]$BackCapture)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
$build=Get-Content (Join-Path $root 'build/latest.json') -Raw|ConvertFrom-Json
$destination=Join-Path $root ('reference/timing-choices/'+$build.buildId)
New-Item -ItemType Directory -Path $destination -Force|Out-Null
Import-Module (Join-Path $root 'scripts/TestWorkspace.psm1') -Force
$results=@()
foreach($vertical in @(0,1)){
 $w=if($vertical){720}else{1280};$h=if($vertical){1280}else{720}
 $session=New-AsioFixture -Values @{vertical_enabled=$vertical;windowsize="$w,$h";vertical_resolution="$w,$h";vertical_width=$w;vertical_height=$h;vsync=1;viewtiming=2;automaticmute=0;currentdir='K-Shoot MANIA';currentitem=1;currentlevel=1} -Song
 $flags=@('--timing-choices-capture');if($BackCapture){$flags+='--back-button-capture'}
 Start-KsmOwnedProcess -Session $session -Arguments $flags -TimeoutSeconds 60
 $captures=@(Get-ChildItem -LiteralPath (Join-Path $session.appRoot 'timing-captures') -Filter 'menu-*.png')
 if($captures.Count -ne 4){throw "Expected four actual select menu choices, got $($captures.Count): $($session.root)"}
 foreach($file in $captures){Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $destination "$w-$h-$($file.Name)")}
 $results+=[pscustomobject]@{size="$w`x$h";status='PASS';captures=4;session=$session.root}
 Write-Output "TIMING CHOICES $w`x$h PASS"
}
$results|ConvertTo-Json -Depth 5|Set-Content (Join-Path $root 'build/timing-choices-latest.json') -Encoding utf8
