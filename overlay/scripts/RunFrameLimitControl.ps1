param([Parameter(Mandatory=$true)][string]$BaselineBuild,[Parameter(Mandatory=$true)][string]$CandidateBuild)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
Import-Module (Join-Path $root 'scripts/TestWorkspace.psm1') -Force
$baseline=Get-Content -LiteralPath $BaselineBuild -Raw|ConvertFrom-Json
$candidate=Get-Content -LiteralPath $CandidateBuild -Raw|ConvertFrom-Json
foreach($build in @($baseline,$candidate)){if($build.configuration -ne 'Release' -or (Get-FileHash -LiteralPath $build.executable).Hash -ne $build.sha256){throw 'Control requires saved Release executable hashes.'}}
$latest=Join-Path $root 'build/latest.json';$saved=[IO.File]::ReadAllBytes($latest)
$quiet=$env:KSM_TEST_QUIET;$metrics=$env:KSM_TEST_PROCESS_METRICS
$env:KSM_TEST_QUIET='1';$env:KSM_TEST_PROCESS_METRICS=$null
$output=Join-Path $root ('build/frame-limit-control/'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $output -Force|Out-Null
$rows=[Collections.Generic.List[object]]::new()
[ordered]@{purpose='Diagnose the retained capped portrait-hidden p99 failure; this does not replace the full performance gate';repetitions=3;labels=@('original-a','original-b-same-executable','candidate');frameLimits=@(300,10000);scene='portrait 1080x1920 APS hidden';samplesPerRun=960;originalSameHashControl=$baseline.sha256;candidateHash=$candidate.sha256}|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $output 'protocol.json') -Encoding UTF8
try{
    [IO.File]::WriteAllBytes($latest,[IO.File]::ReadAllBytes($CandidateBuild))
    $session=New-AsioFixture -Values @{vertical_enabled=1;windowsize='1080,1920';vertical_resolution='1080,1920';vsync='0;300'}
    $configPath=Join-Path $session.appRoot 'config.ini';$config=[IO.File]::ReadAllText($configPath)
    $skinPath=Join-Path $session.appRoot 'skin_preset.json';$skin=if(Test-Path -LiteralPath $skinPath){[IO.File]::ReadAllBytes($skinPath)}else{$null}
    $flags=@('--vertical-preview-hud','--vertical-preview-benchmark','--aps-performance-fixture','--vertical-preview-1080','--aps-performance-hidden')
    foreach($limit in @(300,10000)){
        foreach($iteration in 1..3){
            $labels=if($iteration%2){@('original-a','candidate','original-b-same-executable')}else{@('original-b-same-executable','candidate','original-a')}
            foreach($label in $labels){
                $build=if($label -eq 'candidate'){$candidate}else{$baseline}
                $expectedRoot=[IO.Path]::GetFullPath($session.appRoot).TrimEnd([IO.Path]::DirectorySeparatorChar)+[IO.Path]::DirectorySeparatorChar
                if(-not [IO.Path]::GetFullPath($session.executable).StartsWith($expectedRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'Control executable escapes owned fixture.'}
                Copy-Item -LiteralPath $build.executable -Destination $session.executable -Force
                if((Get-FileHash -LiteralPath $session.executable).Hash -ne $build.sha256){throw 'Control copy hash differs.'}
                $session.buildId=$build.buildId
                [IO.File]::WriteAllText($configPath,($config -replace '(?m)^vsync=.*$',"vsync=0;$limit"),[Text.UTF8Encoding]::new($false))
                if($null -ne $skin){[IO.File]::WriteAllBytes($skinPath,$skin)}elseif(Test-Path -LiteralPath $skinPath){Remove-Item -LiteralPath $skinPath}
                $csv=Join-Path $session.appRoot 'vertical-warp-benchmark.csv'
                if(Test-Path -LiteralPath $csv){Remove-Item -LiteralPath $csv}
                Start-KsmOwnedProcess -Session $session -Arguments $flags -TimeoutSeconds 60
                $data=Import-Csv -LiteralPath $csv|Select-Object -First 1
                if([int]$data.samples -ne 960){throw 'Incomplete control measurement.'}
                $rows.Add([ordered]@{limit=$limit;iteration=$iteration;label=$label;sha256=$build.sha256;mean_ms=[double]$data.mean_ms;p99_ms=[double]$data.p99_ms;longFrames=[int]$data.over_8_333ms})
                Copy-Item -LiteralPath $csv -Destination (Join-Path $output "$limit-$iteration-$label.csv")
                $rows|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $output 'raw.json') -Encoding UTF8
            }
        }
        Write-Output "FRAME LIMIT CONTROL $limit complete"
    }
}finally{[IO.File]::WriteAllBytes($latest,$saved);$env:KSM_TEST_QUIET=$quiet;$env:KSM_TEST_PROCESS_METRICS=$metrics}
Write-Output "FRAME LIMIT CONTROL RECORDED: $output"
