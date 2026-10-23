param([switch]$SkipExecutable)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'scripts/TestWorkspace.psm1') -Force
$checks=@()
function Check([bool]$Condition,[string]$Name){if(-not $Condition){throw "FAIL: $Name"};$script:checks+=$Name}
$play=New-KsmConfiguration -Mode Play
Check ($play -match '(?m)^mastervol=100\s*$') 'Play has audible master volume'
Check ($play -match '(?m)^esckey_bt3=0\s*$') 'Fresh configuration disables BT3 plus START as BACK'
Check ((New-KsmConfiguration -Mode Play -Existing "[config]`nesckey_bt3=1`n") -match '(?m)^esckey_bt3=1\s*$') 'Explicit existing BACK shortcut preference is retained'
Check ($play -match '(?m)^windowsize=800,600\s*$') 'Landscape size remains separate from vertical size'
Check ($play -match '(?m)^key=83,68,75,76,29,28,81,87,79,80,32,13,27,122\s*$') 'Complete Windows keyboard mapping'
Check ($play -match '(?m)^enable_mouse_ui=1\s*$') 'Mouse input defaults retained'
foreach($mode in @('Auto','Preview','Prepare')){Check ((New-KsmConfiguration -Mode $mode) -match '(?m)^mastervol=0\s*$') "$mode remains muted"}
$legacy="[config]`r`nvertical_enabled=0`r`nwindowsize=540,960`r`nmastervol=0`r`n"
$repaired=New-KsmConfiguration -Mode Play -Existing $legacy
Check ($repaired -match '(?m)^vertical_enabled=0\s*$') 'Legacy repair keeps user-selected mode OFF'
Check ($repaired -match '(?m)^mastervol=100\s*$' -and $repaired -match '(?m)^windowsize=800,600\s*$') 'Legacy broken launcher seed repaired'
$custom=$play.Replace('mastervol=100','mastervol=42').Replace('windowsize=800,600','windowsize=1280,720').Replace('vertical_enabled=1','vertical_enabled=0')
$retained=New-KsmConfiguration -Mode Play -Existing $custom
Check ($retained -match '(?m)^mastervol=42\s*$' -and $retained -match '(?m)^windowsize=1280,720\s*$' -and $retained -match '(?m)^vertical_enabled=0\s*$') 'Existing Play preferences preserved'
if(-not $SkipExecutable){
 $session=New-KsmTestRun -Mode Prepare
 [IO.File]::WriteAllText((Join-Path $session.appRoot 'config.ini'),$repaired,[Text.UTF8Encoding]::new($false))
 Start-KsmOwnedProcess -Session $session -Arguments @('--play-launch-config-check') -TimeoutSeconds 60
 $rows=Import-Csv -LiteralPath (Join-Path $session.appRoot 'play-launch-config-check.csv')
 Check (@($rows|Where-Object {$_.status -eq 'FAIL'}).Count -eq 0 -and @($rows|Where-Object {$_.check -eq 'overall' -and $_.status -eq 'PASS'}).Count -eq 1) 'Actual normal startup restores 800x600 and nonzero BASS volume'
 [ordered]@{status='PASS';count=$checks.Count;checks=$checks;session=$session.root}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $root 'build/play-configuration-check.json') -Encoding UTF8
}
Write-Output "PASS $($checks.Count) Play configuration checks"