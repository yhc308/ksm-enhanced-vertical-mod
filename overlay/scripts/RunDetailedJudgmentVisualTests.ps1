param([int[]]$Widths=@(720,1080),[switch]$Landscape,[switch]$ShowNewRecord)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'TestWorkspace.psm1') -Force
$results=@()
foreach($width in $Widths){
 $height=if($Landscape){if($width -eq 640){480}else{720}}else{[int]($width*16/9)}
 $values=@{vertical_enabled=if($Landscape){0}else{1};windowsize="$width,$height";vertical_width=$width;vertical_height=$height;vertical_resolution="$width,$height";vsync='1'}
 $session=New-AsioFixture -Values $values -Song
 if($ShowNewRecord){
  $ui=Join-Path $session.appRoot $(if($Landscape){'mods/enhanced/ui/result.noco'}else{'mods/vertical/ui/result.noco'})
  $doc=Get-Content -LiteralPath $ui -Raw|ConvertFrom-Json -Depth 100
  $doc.params.overlay_newRecordPanelVisible.value=$true;$doc.params.overlay_newRecordNumber.value=60150
  $doc|ConvertTo-Json -Depth 100|Set-Content -LiteralPath $ui -Encoding UTF8
 }
 $capture=Invoke-AsioFixture -Session $session -Flags @('--result-detail-capture') -Report 'result-detail.png' -Expected ''
 if(@(Get-ChildItem -LiteralPath (Join-Path $session.appRoot 'score') -Recurse -File -ErrorAction SilentlyContinue).Count){throw 'Result fixture saved a score.'}
 $results+=[pscustomobject]@{size="$width`x$height";capture=$capture;session=$session.root}
 Write-Output "RESULT CAPTURE: $capture"
}
$results|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $root 'build/result-captures-latest.json') -Encoding UTF8