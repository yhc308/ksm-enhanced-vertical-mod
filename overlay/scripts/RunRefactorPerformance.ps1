param(
    [Parameter(Mandatory=$true)][string]$BaselineBuild,
    [Parameter(Mandatory=$true)][string]$CandidateBuild,
    [ValidateSet(1,5)][int]$SubrunsPerBatch=5
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
Import-Module (Join-Path $root 'scripts/TestWorkspace.psm1') -Force
$states=@{baseline=(Get-Content -LiteralPath $BaselineBuild -Raw|ConvertFrom-Json);candidate=(Get-Content -LiteralPath $CandidateBuild -Raw|ConvertFrom-Json)}
foreach($build in $states.Values){
    if($build.configuration -ne 'Release'){throw 'Performance comparison requires Release builds.'}
    if((Get-FileHash -LiteralPath $build.executable).Hash -ne $build.sha256){throw 'A benchmark executable differs from its saved build hash.'}
}
$latest=Join-Path $root 'build/latest.json'
$savedLatest=[IO.File]::ReadAllBytes($latest)
$oldQuiet=$env:KSM_TEST_QUIET;$oldMetrics=$env:KSM_TEST_PROCESS_METRICS
$env:KSM_TEST_QUIET='1';$env:KSM_TEST_PROCESS_METRICS=$null
$output=Join-Path $root ('build/refactor-performance-batched/'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $output -Force|Out-Null
[ordered]@{protocol="Three predeclared batches, $SubrunsPerBatch 960-frame subruns per scene and build";batchMean='Arithmetic mean of the subrun frame means';batchP99='Median of the measured subrun p99 values';subrunFrames=960;batches=3;subruns=$SubrunsPerBatch;builds=$states;threshold='Both 5 percent and 0.1 ms must be exceeded';stdout='Buffered to isolated logs outside measured rendering';memorySampling=$false}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $output 'protocol.json') -Encoding UTF8
function Median([double[]]$Values){$ordered=@($Values|Sort-Object);if($ordered.Count%2){return $ordered[[int][Math]::Floor($ordered.Count/2)]};return ($ordered[$ordered.Count/2-1]+$ordered[$ordered.Count/2])/2}
function Reset-Config($Session,[byte[]]$Bytes,[byte[]]$Skin){
    [IO.File]::WriteAllBytes((Join-Path $Session.appRoot 'config.ini'),$Bytes)
    $path=Join-Path $Session.appRoot 'skin_preset.json'
    if($null -ne $Skin){[IO.File]::WriteAllBytes($path,$Skin)}elseif(Test-Path -LiteralPath $path){Remove-Item -LiteralPath $path}
}
try{
    [IO.File]::WriteAllBytes($latest,[IO.File]::ReadAllBytes($CandidateBuild))
    $sessions=@{}
    foreach($vertical in @(0,1)){
        $size=if($vertical){'1080,1920'}else{'1280,720'}
        $session=New-AsioFixture -Values @{vertical_enabled=$vertical;windowsize=$size;vertical_resolution=$size;vsync=0}
        $config=[IO.File]::ReadAllBytes((Join-Path $session.appRoot 'config.ini'))
        $skinPath=Join-Path $session.appRoot 'skin_preset.json'
        $skin=if(Test-Path -LiteralPath $skinPath){[IO.File]::ReadAllBytes($skinPath)}else{$null}
        $sessions[$vertical]=[pscustomobject]@{session=$session;config=$config;skin=$skin}
    }
    foreach($batch in 1..3){
        $raw=[Collections.Generic.List[object]]::new()
        foreach($vertical in @(0,1)){
            $fixture=$sessions[$vertical];$session=$fixture.session
            $sides=if(($batch+$vertical)%2){@('baseline','candidate')}else{@('candidate','baseline')}
            foreach($side in $sides){
                $build=$states[$side]
                $target=[IO.Path]::GetFullPath($session.executable)
                $expectedRoot=[IO.Path]::GetFullPath($session.appRoot).TrimEnd('\')+'\'
                if(-not $target.StartsWith($expectedRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'Benchmark target escapes its owned fixture.'}
                # Every earlier child has exited before replacing this isolated executable.
                Copy-Item -LiteralPath $build.executable -Destination $target -Force
                if((Get-FileHash -LiteralPath $target).Hash -ne $build.sha256){throw 'Copied benchmark executable hash differs.'}
                $session.buildId=$build.buildId
                $sizeFlag=if($vertical){'--vertical-preview-1080'}else{'--vertical-preview-landscape'}
                $common=@('--vertical-preview-hud','--vertical-preview-benchmark','--aps-performance-fixture',$sizeFlag)
                Reset-Config $session $fixture.config $fixture.skin
                Start-KsmOwnedProcess -Session $session -Arguments ($common+@('--aps-performance-overlap')) -TimeoutSeconds 60
                foreach($iteration in 1..$SubrunsPerBatch){
                    $modes=if($iteration%2){@('hidden','idle','overlap')}else{@('overlap','idle','hidden')}
                    foreach($mode in $modes){
                        Reset-Config $session $fixture.config $fixture.skin
                        $csv=Join-Path $session.appRoot 'vertical-warp-benchmark.csv'
                        if(Test-Path -LiteralPath $csv){Remove-Item -LiteralPath $csv}
                        Start-KsmOwnedProcess -Session $session -Arguments ($common+@("--aps-performance-$mode")) -TimeoutSeconds 60
                        $data=Import-Csv -LiteralPath $csv|Select-Object -First 1
                        if([int]$data.samples -ne 960){throw 'Incomplete performance subrun.'}
                        $raw.Add([ordered]@{batch=$batch;side=$side;iteration=$iteration;vertical=$vertical;mode=$mode;mean_ms=[double]$data.mean_ms;p99_ms=[double]$data.p99_ms;over_8_333ms=[int]$data.over_8_333ms;session=$session.root})
                        Copy-Item -LiteralPath $csv -Destination (Join-Path $output "$side-$batch-$vertical-$iteration-$mode.csv")
                        $raw|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $output "raw-$batch.json") -Encoding UTF8
                    }
                }
                Write-Output "BATCH $batch vertical=$vertical ${side}: $SubrunsPerBatch subruns per scene complete"
            }
        }
        foreach($side in @('baseline','candidate')){
            $summary=@()
            foreach($vertical in @(0,1)){foreach($mode in @('hidden','idle','overlap')){
                $values=@($raw|Where-Object {$_.side -eq $side -and $_.vertical -eq $vertical -and $_.mode -eq $mode})
                if($values.Count -ne $SubrunsPerBatch){throw 'A declared benchmark batch is incomplete.'}
                $summary+=[ordered]@{vertical=$vertical;mode=$mode;mean_ms=($values.mean_ms|Measure-Object -Average).Average;p99_ms=(Median $values.p99_ms);over_8_333ms=($values.over_8_333ms|Measure-Object -Sum).Sum;subruns=$SubrunsPerBatch;frames=(960*$SubrunsPerBatch)}
            }}
            $summary|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $output "$side-$batch.json") -Encoding UTF8
        }
    }
}finally{
    [IO.File]::WriteAllBytes($latest,$savedLatest)
    $env:KSM_TEST_QUIET=$oldQuiet;$env:KSM_TEST_PROCESS_METRICS=$oldMetrics
}
$before=@(1..3|ForEach-Object {Join-Path $output "baseline-$_.json"})
$after=@(1..3|ForEach-Object {Join-Path $output "candidate-$_.json"})
python (Join-Path $PSScriptRoot 'CompareFramePerformance.py') --baseline @before --candidate @after --output (Join-Path $output 'comparison.json')
if($LASTEXITCODE){throw "Batched performance gate failed: $output"}
Write-Output "BATCHED PERFORMANCE PASS: $output"
