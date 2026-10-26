Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$sourcePath = Join-Path $projectRoot 'kshootmania/App/ui/scene/play_hud.noco'
$hud = Get-Content -LiteralPath $sourcePath -Raw | ConvertFrom-Json
$hud.referenceSize = @(720.0, 1280.0)
$hud.autoFitMode = 'Contain'
function Set-Box($node, $x, $y, $width, $height) {
    $node.region = [pscustomobject]@{
        type='AnchorRegion'; anchorMin=@(0.0,0.0); anchorMax=@(0.0,0.0)
        posDelta=@([double]$x,[double]$y); sizeDelta=@([double]$width,[double]$height)
        sizeDeltaPivot=@(0.0,0.0)
    }
}
function Find-Node($nodes, $name) {
    foreach ($node in $nodes) {
        if ($node.name -eq $name) { return $node }
        $found = Find-Node $node.children $name
        if ($null -ne $found) { return $found }
    }
    return $null
}
$layout = Get-Content -LiteralPath (Join-Path $projectRoot 'kshootmania/App/mods/vertical/ui/ingame_layout.json') -Raw | ConvertFrom-Json
# Reuse the landscape metadata bindings, including custom title/artist images.
foreach ($sourceName in @('Jacket','TitlePanel')) {
    $copy = Find-Node $hud.children $sourceName | ConvertTo-Json -Depth 100 | ConvertFrom-Json
    $copy.name = 'Compact' + $sourceName
    foreach ($child in $copy.children) { $child.name = 'Compact' + $child.name }
    $hud.children += $copy
}
foreach ($property in $layout.boxes.PSObject.Properties) {
    $name = $property.Name
    $node = Find-Node $hud.children $name
    if ($null -eq $node) { throw "HUD node missing: $name" }
    $box = $property.Value
    Set-Box $node $box[0] $box[1] $box[2] $box[3]
}
$jacket = Find-Node $hud.children 'Jacket'
$jacket.components[0].preserveAspect = $true
$titlePanel = Find-Node $hud.children 'TitlePanel'
$titlePanel.components[0].textureRegionMode = 'OffsetSize'
$titlePanel.components[0].textureOffset = @(17.0,5.0)
$titlePanel.components[0].textureSize = @(448.0,77.0)
$title = Find-Node $hud.children 'Title'
$title.components[0].fontSize = 40.0
$title.components[0].minFontSize = 18.0
$artist = Find-Node $hud.children 'Artist'
$artist.components[0].fontSize = 28.0
$artist.components[0].minFontSize = 14.0
$compactJacket = Find-Node $hud.children 'CompactJacket'
$compactJacket.components[0].preserveAspect = $true
$compactPanel = Find-Node $hud.children 'CompactTitlePanel'
$compactPanel.components[0].textureRegionMode = 'OffsetSize'
$compactPanel.components[0].textureOffset = @(17.0,5.0)
$compactPanel.components[0].textureSize = @(448.0,77.0)
$compactTitle = Find-Node $hud.children 'CompactTitle'
$compactTitle.components[0].fontSize = 18.0
$compactTitle.components[0].minFontSize = 8.0
$compactArtist = Find-Node $hud.children 'CompactArtist'
$compactArtist.components[0].fontSize = 13.0
$compactArtist.components[0].minFontSize = 7.0
foreach ($numberName in @('LevelNumber','BPMNumber','HispeedNumber','HispeedNumberEffective','ScoreNumber','FPSNumber')) {
    $numberNode = Find-Node $hud.children $numberName
    $subCanvas = $numberNode.components | Where-Object type -EQ 'SubCanvas'
    $numberParams = $subCanvas.serializedParamsJSON | ConvertFrom-Json
    $sizeProperty = $layout.numberCharacterSizes.PSObject.Properties[$numberName]
    $numberParams.characterSize = if ($null -ne $sizeProperty) { @($sizeProperty.Value) } else { @(18.0,18.0) }
    $subCanvas.serializedParamsJSON = $numberParams | ConvertTo-Json -Depth 20 -Compress
}
$scoreCaption = Find-Node $hud.children 'ScoreCaption'
$songInfo = Find-Node $hud.children 'SongInfo'
$songInfo.components[0].textureRegionMode = 'OffsetSize'
$songInfo.components[0].textureOffset = @(0.0,0.0)
$songInfo.components[0].textureSize = @(480.0,92.0)
$marker = Find-Node $hud.children 'PositionMarker'
$marker.activeSelf = $false # Draw original marker with a screen-space progress rail.
$targetDir = Join-Path $projectRoot 'kshootmania/App/mods/vertical/ui'
New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
$hud | ConvertTo-Json -Depth 100 | Set-Content -LiteralPath (Join-Path $targetDir 'play_hud.noco') -Encoding utf8

& 'python' (Join-Path $PSScriptRoot 'GenerateIngameExScore.py')
if($LASTEXITCODE -ne 0){throw 'Ingame EX SCORE generation failed'}
