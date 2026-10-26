param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'TestWorkspace.psm1') -Force
$python='python'
$results=@()
foreach($vertical in @(0,1)){
 $width=if($vertical){1080}else{1280};$height=if($vertical){1920}else{720}
 $session=New-AsioFixture -Values @{vertical_enabled=$vertical;windowsize="$width,$height";vertical_resolution="$width,$height";vertical_width=$width;vertical_height=$height;vsync=1}
 Set-Content -LiteralPath (Join-Path $session.appRoot 'render-geometry-fixture') -Value 'isolated solid draw fixtures'
 & $python (Join-Path $root 'tests/PrepareRenderGeometryAssets.py') $session.appRoot
 if($LASTEXITCODE -ne 0){throw 'Failed to prepare render fixture'}
 Start-KsmOwnedProcess -Session $session -Arguments @('--render-geometry-test') -TimeoutSeconds 120
 $failure=Join-Path $session.appRoot 'render-geometry-failure.txt'
 if(Test-Path -LiteralPath $failure){throw (Get-Content -LiteralPath $failure -Raw)}
 $passed=Join-Path $session.appRoot 'render-geometry-passed.txt'
 if(-not(Test-Path -LiteralPath $passed)){throw "Missing render geometry report: $($session.root)"}
 if((Get-Content -LiteralPath (Join-Path $session.appRoot 'render-geometry.csv') -Raw) -match 'FAIL'){throw "Render report failed: $($session.root)"}
 Write-Output "RENDER vertical=$vertical`: $(Get-Content -LiteralPath $passed)"
 $results+=[pscustomobject]@{vertical=[bool]$vertical;status='PASS';session=$session.root}
}
$results|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $root 'build/render-geometry-latest.json') -Encoding UTF8
