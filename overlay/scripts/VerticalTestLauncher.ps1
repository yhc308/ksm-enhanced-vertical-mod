param(
    [ValidateSet('Menu','Auto','Preview','Play','Prepare')][string]$Mode='Menu',
    [switch]$NoOpen,
    [switch]$IncludeDisplayTests,
    [switch]$IncludeSpinTests
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'TestWorkspace.psm1') -Force
if($Mode -eq 'Menu'){
    Write-Host 'Enhanced Vertical: 1 Auto / 2 Preview / 3 Play / 4 Prepare / 0 Exit'
    switch(Read-Host 'Select'){'1'{$Mode='Auto'}'2'{$Mode='Preview'}'3'{$Mode='Play'}'4'{$Mode='Prepare'}'0'{return}default{throw 'Invalid selection'}}
}
$session=New-KsmTestRun -Mode $Mode
Write-Host "Test workspace: $($session.root)"
if($Mode -eq 'Prepare'){$session;return}
if($Mode -eq 'Play'){
    Write-Host "Persistent Play workspace: $($session.appRoot)"
    Write-Host "Test songs: $(Join-Path $session.appRoot 'songs')"
    if($session.isRunning){
        Write-Host "The Play executable is in use. No runtime files were changed. Close the game when convenient, then run Play again to apply build $($session.pendingBuildId)."
        $session;return
    }
    Write-Host "Updated $($session.updatedFiles) managed files. Settings, songs, scores and cache remain here."
    if(-not $NoOpen){Start-KsmOwnedProcess -Session $session -Interactive|Out-Null}
    $session;return
}
$results=@()
try {
    if($Mode -eq 'Auto'){
        $cases=@(
            @{flag='--vertical-self-test';report='vertical-self-test.csv'},
            @{flag='--vertical-replay-test';report='vertical-judgment-replay.csv'},
            @{flag='--vertical-menu-input-test';report='vertical-menu-input.csv'},
            @{flag='--vertical-options-test';report='vertical-options-flow-test.csv'}
        )
        if($IncludeSpinTests){$cases+=@{flag='--vertical-spin-test';report='vertical-spin-regression.csv'}}
        if($IncludeDisplayTests){$cases+=@{flag='--vertical-display-test';report='vertical-display-test.csv'}}
        foreach($case in $cases){
            $report=Join-Path $session.appRoot $case.report
            if(Test-Path -LiteralPath $report){throw "Unexpected stale output: $report"}
            Write-Host "Running $($case.flag)"
            $arguments=@($case.flag)
            if($case.flag -eq '--vertical-spin-test'){
                $chart=Join-Path $session.root 'fixtures/spin/Tornaido-Test.kson'
                if(-not(Test-Path -LiteralPath $chart)){throw 'Spin fixture missing.'}
                $arguments+=@('--vertical-preview-chart',$chart)
            }
            $configPath=Join-Path $session.appRoot 'config.ini'
            $configBefore=(Get-FileHash -LiteralPath $configPath -Algorithm SHA256).Hash
            Start-KsmOwnedProcess -Session $session -Arguments $arguments -TimeoutSeconds 60
            if($case.flag -eq '--vertical-options-test' -and (Get-FileHash -LiteralPath $configPath -Algorithm SHA256).Hash -ne $configBefore){throw 'Options diagnostic changed its config.'}
            if($case.flag -eq '--vertical-self-test'){
                foreach($subreport in @('vertical-guide-calibration.csv','vertical-tunnel-test.csv','vertical-projective-wall-test.csv','vertical-wall-thickness-test.csv','vertical-projective-panel-test.csv','vertical-depth-strength-test.csv','vertical-lane-anchor-test.csv')){
                    $subpath=Join-Path $session.appRoot $subreport
                    if(-not(Test-Path -LiteralPath $subpath)){throw "Missing self-test subreport: $subreport"}
                    $subtext=Get-Content -LiteralPath $subpath -Raw
                    if($subtext -notmatch 'PASS' -or $subtext -match '(?m)(^|,)FAIL(,|\r?$)'){throw "Self-test subreport failed: $subreport"}
                    $results+=[pscustomobject]@{flag=$case.flag;status='PASS';report=$subpath}
                }
            }
            if(-not(Test-Path -LiteralPath $report)){throw "Missing diagnostic report: $report"}
            $text=Get-Content -LiteralPath $report -Raw
            if($case.report.EndsWith('.csv')){
                if(($text -split '\r?\n'|Where-Object {$_ -match '^overall,'}|Select-Object -Last 1) -notmatch 'PASS\s*$'){throw "Diagnostic FAIL: $report"}
                if($text -match '(?m)(^|,)FAIL(,|\r?$)'){throw "Failed diagnostic row: $report"}
            }elseif($text -notmatch 'PASS'){throw "Spin FAIL: $report"}
            $results+=[pscustomobject]@{flag=$case.flag;status='PASS';report=$report}
        }
        # Installer compatibility tests run separately against synthetic fixtures, never a live game.
        $results|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $session.root 'results.json') -Encoding UTF8
        Write-Host 'AUTO PASS (display/spin only when explicitly selected).'
    }else{
        foreach($flag in @('--vertical-preview-title','--vertical-preview-options','--vertical-preview-select','--vertical-preview-hud','--vertical-preview-result')){
            Start-KsmOwnedProcess -Session $session -Arguments @($flag,'--vertical-preview-720') -TimeoutSeconds 60
        }
        if(-not $NoOpen){Invoke-Item -LiteralPath (Join-Path $session.appRoot 'Screenshot')}
    }
} catch {
    $_.Exception.Message|Set-Content -LiteralPath (Join-Path $session.root 'failure.txt') -Encoding UTF8
    throw
} finally {Write-Host "Reports retained: $($session.root)"}