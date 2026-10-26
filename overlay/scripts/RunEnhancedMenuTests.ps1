param([switch]$PortraitOnly)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'TestWorkspace.psm1') -Force
$results=@()
$cases=if($PortraitOnly){@(1)}else{@(0,1)}
foreach($vertical in $cases){
 $width=if($vertical){720}else{1280};$height=if($vertical){1280}else{720}
 $session=New-AsioFixture -Values @{vertical_enabled=$vertical;windowsize="$width,$height";vertical_resolution="$width,$height";vertical_width=$width;vertical_height=$height;vsync=1}
 Set-Content -LiteralPath (Join-Path $session.appRoot 'enhanced-menu-fixture') -Value 'isolated menu fixture'
 Start-KsmOwnedProcess -Session $session -Arguments @('--enhanced-menu-test') -TimeoutSeconds 30
 $report=Join-Path $session.appRoot 'enhanced-menu.csv'
 if(-not(Test-Path -LiteralPath $report)){throw "Missing menu report: $($session.root)"}
 if((Get-Content -LiteralPath $report -Raw) -match 'FAIL'){throw "Menu flow failed: $report"}
 if(-not(Test-Path -LiteralPath (Join-Path $session.appRoot 'enhanced-menu-passed.txt'))){throw "Incomplete menu flow: $report"}
 Write-Output "MENU $width`x$height`: $(Get-Content -LiteralPath (Join-Path $session.appRoot 'enhanced-menu-passed.txt'))"
 $results+=[pscustomobject]@{size="$width`x$height";status='PASS';session=$session.root}
}
$results|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $root 'build/menu-tests-latest.json') -Encoding UTF8
