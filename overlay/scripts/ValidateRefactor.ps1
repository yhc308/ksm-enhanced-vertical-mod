param(
    [ValidateSet('Fast','Full')][string]$Tier='Fast',
    [switch]$SkipBuild,
    [string]$AuthoringRoot='',
    [string]$BaselineReport='',
    [string]$ResourceBaseline='',
    [string]$ResumeAfter=''
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$logRoot=Join-Path $root ('build/validation/'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $logRoot -Force|Out-Null
$results=[Collections.Generic.List[object]]::new()
$script:resumeReached=[string]::IsNullOrEmpty($ResumeAfter)
function Run([string]$Name,[scriptblock]$Operation){
    if(-not $script:resumeReached){
        if($Name -eq $ResumeAfter){$script:resumeReached=$true}
        Write-Output "PREVIOUS RUN REQUIRED: $Name"
        return
    }
    Write-Output "RUN $Name"
    $clock=[Diagnostics.Stopwatch]::StartNew()
    try{
        & $Operation *> (Join-Path $logRoot "$Name.log")
        $results.Add([ordered]@{name=$Name;status='PASS';seconds=$clock.Elapsed.TotalSeconds})
        Write-Output "PASS $Name"
    }catch{
        $results.Add([ordered]@{name=$Name;status='FAIL';seconds=$clock.Elapsed.TotalSeconds;error=$_.Exception.Message})
        throw
    }finally{
        $results|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $logRoot 'results.json') -Encoding UTF8
    }
}
Push-Location $root
try{
    if(-not $SkipBuild){Run 'Release-build' {& scripts/Build.ps1 -Configuration Release -Diagnostics}}
    Run 'Cpp-style' {python scripts/CheckCppStyle.py; if($LASTEXITCODE){throw 'C++ style check failed'}}
    Run 'Release-units' {& scripts/BuildUnitTests.ps1 -Configuration Release}
    Run 'Headless-units' {& scripts/RunHeadlessTests.cmd; if($LASTEXITCODE){throw 'Headless tests failed'}}
    Run 'EX' {& scripts/RunExScoreTests.ps1}
    Run 'APS' {& scripts/RunApsTests.ps1}
    Run 'Judge-setting' {& scripts/RunJudgeSettingTests.ps1}
    if($AuthoringRoot){Run 'Judge-pixels-resources' {python tests/JudgeSettingVerifier.py --authoring-root $AuthoringRoot; if($LASTEXITCODE){throw 'Judge pixel/resource checks failed'}}}else{Run 'Judge-pixels' {python tests/JudgeSettingVerifier.py --skip-authoring;if($LASTEXITCODE){throw 'Judge pixel checks failed'}}}
    if($ResourceBaseline){Run 'Runtime-resources' {python scripts/VerifyResourceBaseline.py kshootmania/App $ResourceBaseline --allow-checkout-line-endings; if($LASTEXITCODE){throw 'Runtime resources changed'}}}
    if($BaselineReport){Run 'Judge-baseline' {python scripts/CompareJudgeSettingBaseline.py $BaselineReport build/judge-setting-tests.json --output (Join-Path $logRoot 'judge-baseline.json');if($LASTEXITCODE){throw 'Baseline comparison failed'}}}
    if($Tier -eq 'Full'){
        Run 'Debug-units' {& scripts/BuildUnitTests.ps1 -Configuration Debug}
        Run 'Audio-engine' {& scripts/RunAudioTests.ps1}
        Run 'Audio-integration' {& scripts/RunAudioIntegrationTests.ps1}
        Run 'Audio-game-EOF' {& scripts/RunAsioEofGameTests.ps1}
        Run 'Menus' {& scripts/RunEnhancedMenuTests.ps1}
        Run 'Skins' {& scripts/RunSkinTests.ps1}
        Run 'Skins-1080x1920' {& scripts/RunSkinTests.ps1 -Cases vertical -PortraitWidth 1080}
        Run 'Viewport' {& scripts/RunViewportOptionsTests.ps1}
        Run 'Viewport-640x480' {& scripts/RunViewportOptionsTests.ps1 -LandscapeWidth 640 -LandscapeOnly}
        Run 'Viewport-1920x1080' {& scripts/RunViewportOptionsTests.ps1 -LandscapeWidth 1920 -LandscapeHeight 1080 -LandscapeOnly}
        Run 'Viewport-1080x1920' {& scripts/RunViewportOptionsTests.ps1 -PortraitWidth 1080}
        Run 'Render-geometry' {& scripts/RunRenderGeometryTests.ps1}
        Run 'Portrait-scenes' {& scripts/RunPortraitSceneTests.ps1}
        Run 'Timing-choices' {& scripts/RunTimingChoiceTests.ps1}
        Run 'EX-landscape' {& scripts/RunExScoreTests.ps1 -Group landscape}
        Run 'EX-portrait' {& scripts/RunExScoreTests.ps1 -Group portrait}
        Run 'EX-ingame' {& scripts/RunIngameExHudTests.ps1}
    }
    if(-not $script:resumeReached){throw "Resume checkpoint was not found: $ResumeAfter"}
    Write-Output "VALIDATION PASS: $logRoot"
}finally{Pop-Location}
