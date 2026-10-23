Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$target = Join-Path $projectRoot 'kshootmania/App/mods/vertical/ui'
New-Item -ItemType Directory -Force -Path $target | Out-Null
function Scene-Source([string]$scene) {
    $enhanced=Join-Path $projectRoot "kshootmania/App/mods/enhanced/ui/$scene.noco"
    if(Test-Path -LiteralPath $enhanced){return $enhanced}
    return Join-Path $projectRoot "kshootmania/App/ui/scene/$scene.noco"
}
function Box($n,$x,$y,$w,$h) {
    $n.region=[pscustomobject]@{type='AnchorRegion';anchorMin=@(0.0,0.0);anchorMax=@(0.0,0.0);
        posDelta=@([double]$x,[double]$y);sizeDelta=@([double]$w,[double]$h);sizeDeltaPivot=@(0.0,0.0)}
}
function Find($nodes,$name) {
    foreach ($n in $nodes) { if ($n.name -eq $name) {return $n}; $found=Find $n.children $name; if ($null -ne $found) {return $found} }
    return $null
}
function Scale-Nodes($nodes,[double]$sx,[double]$sy) {
    foreach ($node in $nodes) {
        if ($node.region.PSObject.Properties.Name -contains 'posDelta') {
            $node.region.posDelta=@(($node.region.posDelta[0]*$sx),($node.region.posDelta[1]*$sy))
            $node.region.sizeDelta=@(($node.region.sizeDelta[0]*$sx),($node.region.sizeDelta[1]*$sy))
        }
        foreach ($component in $node.components) {
            if ($component.type -eq 'Label') { $component.fontSize=[Math]::Max(18.0,$component.fontSize*[Math]::Min($sx,$sy)) }
        }
        Scale-Nodes $node.children $sx $sy
    }
}
foreach ($part in @('select_item_top','select_item_bottom','select_item_center')) {
    $card=Get-Content -LiteralPath (Join-Path $projectRoot "kshootmania/App/ui/parts/$part.noco") -Raw | ConvertFrom-Json
    $card | ConvertTo-Json -Depth 100 | Set-Content -LiteralPath (Join-Path $target "$part.noco") -Encoding utf8
}
# Portrait selection reuses the landscape cards and their bindings.
$select=Get-Content -LiteralPath (Scene-Source 'select') -Raw | ConvertFrom-Json
$select.referenceSize=@(720.0,1280.0)
$select.autoFitMode='Contain'
Box (Find $select.children 'Background') 0 0 720 1280
(Find $select.children 'Background').components[0].textureFilePath='imgs/bg2_vertical.png'
Box (Find $select.children 'BgAnimationTop') 0 119 720 16
Box (Find $select.children 'BgAnimationBottom') 0 1120 720 16
$topTemplate=Find $select.children 'TopItem1'
$bottomTemplate=Find $select.children 'BottomItem1'
# Keep native card coordinates: a single uniform scale preserves all child proportions.
$cards=@()
foreach ($side in @('top','bottom')) {
    $count=if($side -eq 'top'){6}else{7}
    for($i=$count;$i -ge 1;$i--) {
        $template=if($side -eq 'top'){$topTemplate}else{$bottomTemplate}
        $card=$template | ConvertTo-Json -Depth 100 | ConvertFrom-Json
        $card.name=if($side -eq 'top'){"TopItem$i"}else{"BottomItem$i"}
        $card.components[0].tag="$side$i"
        $card.components[0].canvasPath="mods/vertical/ui/select_item_$side.noco"
        $y=if($side -eq 'top'){508.6-54*$i}else{667.4+57*$i}
        # Reference ellipse: center (590,636), radii (426,599).
        # Align the visible upper-left / lower-left border corner, not the card center.
        # The 816x228 card has a 2px border inset and is drawn at 0.4 scale.
        $cornerY=$y+$(if($side -eq 'top'){-44.8}else{44.8})
        $ellipseY=($cornerY-636.0)/599.0
        $cornerX=590.0-426.0*[Math]::Sqrt(1.0-$ellipseY*$ellipseY)
        $x=$cornerX+162.4
        $card.region.anchorMin=@(0.0,0.0);$card.region.anchorMax=@(0.0,0.0)
        $card.region.posDelta=@($x,$y);$card.region.sizeDeltaPivot=@(0.5,0.5)
        $card.transform.scale=@(0.4,0.4)
        # Preserve the original center pulse; neighbor shake follows the same ellipse.
        foreach($direction in @('Up','Down')) {
            $offset=if($direction -eq 'Up'){-20.0}else{20.0}
            $shakeY=($cornerY+$offset-636.0)/599.0
            $dx=(590.0-426.0*[Math]::Sqrt(1.0-$shakeY*$shakeY))-$cornerX
            $card.components+= [pscustomobject]@{
                type='Tween';tag="shake$direction";active=$false;restartOnActive=$true
                duration=0.10;delay=0.0;easing='EaseOutSine';loopType='None'
                translateEnabled=$true;translateFrom=@($dx,$offset);translateTo=@(0.0,0.0)
                scaleEnabled=$false;rotationEnabled=$false;colorEnabled=$false
            }
        }
        $cards+=$card
    }
}
$select.children=@($select.children | Where-Object {$_.name -notmatch '^(Top|Bottom)Item[0-9]+$'})
$center=Find $select.children 'CenterItem'
$center.region.anchorMin=@(0.0,0.0);$center.region.anchorMax=@(0.0,0.0)
$center.region.posDelta=@(360.0,588.0)
$center.transform.scale.default=@(0.6632,0.6632);$center.transform.scale.hovered=@(0.6665,0.6665)
$center.components[0].canvasPath='mods/vertical/ui/select_item_center.noco'
# Draw far cards first and the selected card last; original overlap and hit tests are retained.
$select.children=@($select.children[0..2])+$cards+@($select.children | Select-Object -Skip 3)
$bar=Find $select.children 'ScrollBar'
Box $bar 692 108 20 1067
$bar.components[0].textureFilePath='imgs/msel_bar_vertical.png'
$cursor=Find $bar.children 'Cursor'
Box $cursor 0 4 20 20
($cursor.components | Where-Object type -eq 'Tween').translateTo=@(0.0,1039.0)
Box (Find $select.children 'Guide') 224 1143 458 24
foreach($guideImage in (Find $select.children 'Guide').children) {
    $guideImage.region.sizeDelta=@(458.0,24.0)
}
Box (Find $select.children 'PlayerName') 10 1140 180 30
(Find $select.children 'PlayerName').components[0].fontSize=20.0
Box (Find $select.children 'SearchResult') 8 140 180 108
$select | ConvertTo-Json -Depth 100 | Set-Content -LiteralPath (Join-Path $target 'select.noco') -Encoding utf8
$prepare=Get-Content -LiteralPath (Join-Path $projectRoot 'kshootmania/App/ui/scene/play_prepare.noco') -Raw | ConvertFrom-Json
$prepare.referenceSize=@(720.0,1280.0)
$prepare.autoFitMode='Contain'
Box (Find $prepare.children 'Background') 0 0 720 1280
(Find $prepare.children 'Background').components[0].textureFilePath='imgs/bg3_vertical.png'
$jacket=Find $prepare.children 'Jacket'
Box $jacket 360 426 344 344
$jacket.region.sizeDeltaPivot=@(0.5,0.5)
$info=Find $prepare.children 'SongInfo'
Box $info 360 710 720 180
$info.region.sizeDeltaPivot=@(0.5,0.5);$info.transform.scale=@((32.0/45),(32.0/45))
$hs=Find $prepare.children 'HispeedPanel'
Box $hs 0 848 452 54
$hs.region.anchorMin=@(1.0,0.0);$hs.region.anchorMax=@(1.0,0.0)
$hs.region.sizeDeltaPivot=@(1.0,1.0);$hs.transform.scale=@(0.56,0.56)
$prepare | ConvertTo-Json -Depth 100 | Set-Content -LiteralPath (Join-Path $target 'play_prepare.noco') -Encoding utf8
foreach ($scene in @('option','result')) {
    $doc=Get-Content -LiteralPath (Scene-Source $scene) -Raw | ConvertFrom-Json
    $doc.referenceSize=@(720.0,1280.0)
    $doc.autoFitMode='Contain'
    Box (Find $doc.children 'Background') 0 0 720 1280
    switch ($scene) {
        option {
            (Find $doc.children 'Background').components[0].textureFilePath='imgs/bg_vertical.png'
            Box (Find $doc.children 'TopMenu') 48 330 624 560
            $menu=Find $doc.children 'TopMenu'
            foreach ($n in $menu.children) { $n.region.sizeDelta=@(600.0,104.0) }
            Box (Find $doc.children 'SettingMenu') 40 160 640 970
            (Find $doc.children 'Guide').region.posDelta[1]=-110.0
            foreach($group in @('Setting','KeyConfig')){(Find (Find $doc.children $group).children 'Header').region.posDelta[1]=122.0}
            $settings=Find $doc.children 'SettingMenu'
            $settings.clippingEnabled=$true
            $settings.verticalScrollable=$true
            $settings.childrenLayout.spacing=@(0.0,8.0)
        }
        result {
            # Same ASIO+Sync components/bindings, portrait placement only.
            $panel=Find $doc.children 'ResultPanel'
            $panel.region.posDelta=@(235.0,641.0)
            $panel.region.anchorMin=@(0.0,0.0);$panel.region.anchorMax=@(0.0,0.0)
            $panel.transform.scale=@(0.785,0.785)
            $detail=Find $doc.children 'DetailedJudgmentPanel'
            $detail.region.posDelta=@(586.0,541.0)
            $detail.region.anchorMin=@(0.0,0.0);$detail.region.anchorMax=@(0.0,0.0)
            $detail.transform.scale=@(0.78,0.78)
            $top=Find $doc.children 'ResultTop'
            Box $top 49 288 608 50
            $top.transform.scale=@(1.0,1.0)
            (Find $doc.children 'Background').components[0].textureFilePath='imgs/bg2_vertical.png'
            $record=Find $doc.children 'NewRecordPanel'
            $record.region.anchorMin=@(0.0,0.0);$record.region.anchorMax=@(0.0,0.0)
            $record.region.posDelta=@(250.0,507.0)
            (Find $record.children 'Body').transform.scale=@(0.79,0.79)
            Box (Find $doc.children 'BottomLeftText') 24 1120 500 100
            Box (Find $doc.children 'BottomRightText') 580 1236 116 24
        }
    }
    $doc | ConvertTo-Json -Depth 100 | Set-Content -LiteralPath (Join-Path $target "$scene.noco") -Encoding utf8
}
$item=Get-Content -LiteralPath (Join-Path $projectRoot 'kshootmania/App/ui/parts/option_setting_item.noco') -Raw | ConvertFrom-Json
$item.referenceSize=@(640.0,48.0)
function Grow-Fonts($nodes) {
    foreach ($node in $nodes) {
        foreach ($component in $node.components) {
            if ($component.type -eq 'Label') { $component.fontSize=22.0 }
        }
        Grow-Fonts $node.children
    }
}
Grow-Fonts $item.children
$item | ConvertTo-Json -Depth 100 | Set-Content -LiteralPath (Join-Path $target 'option_setting_item.noco') -Encoding utf8

# Fixed screen HUD positions in the same 720x1280 coordinates as Vertical gameplay.
$timing=Get-Content -LiteralPath (Scene-Source 'timing_feedback') -Raw | ConvertFrom-Json
$timing.referenceSize=@(720.0,1280.0)
$timing.autoFitMode='Contain'
$centers=@(377.0,527.0,703.0,856.0)
for($i=0;$i -lt 4;$i++) {
    foreach($field in @('Word','Ms')) {
        $node=Find $timing.children "Timing$i$field"
        $x=if($field -eq 'Word'){260.0}else{380.0}
        Box $node $x ($centers[$i]-10) 80 20
        $node.components[0].fontSize=16.0
    }
}
$timing|ConvertTo-Json -Depth 100|Set-Content -LiteralPath (Join-Path $target 'timing_feedback.noco') -Encoding utf8

# Portrait title retains the stock four original menu sprites and their bindings.
$title=Get-Content -LiteralPath (Join-Path $projectRoot 'kshootmania/App/ui/scene/title.noco') -Raw|ConvertFrom-Json
$title.referenceSize=@(720.0,1280.0);$title.autoFitMode='Contain'
Box (Find $title.children 'Background') 0 0 720 1280
(Find $title.children 'Background').components[0].textureFilePath='imgs/standby_vertical.png'
$menu=Find $title.children 'Menu'
$menu.region.posDelta=@(360.0,760.0);$menu.region.anchorMin=@(0.0,0.0);$menu.region.anchorMax=@(0.0,0.0)
$title|ConvertTo-Json -Depth 100|Set-Content -LiteralPath (Join-Path $target 'title.noco') -Encoding utf8

& 'python' (Join-Path $PSScriptRoot 'GenerateTimingMenus.py')
if($LASTEXITCODE -ne 0){throw 'Timing menu generation failed'}

& 'python' (Join-Path $PSScriptRoot 'GenerateExScoreUI.py')

& 'python' (Join-Path $PSScriptRoot 'GenerateIngameExScore.py')
if($LASTEXITCODE -ne 0){throw 'Ingame EX SCORE generation failed'}
