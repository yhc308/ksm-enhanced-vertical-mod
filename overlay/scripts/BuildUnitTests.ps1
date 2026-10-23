param([ValidateSet('Debug','Release')][string]$Configuration='Release')
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$python=(Get-Command python -ErrorAction Stop).Source
$include=Join-Path $root 'build/deps/include/catch2/catch.hpp'
if(-not(Test-Path -LiteralPath $include)){
    New-Item -ItemType Directory -Path (Split-Path $include -Parent) -Force|Out-Null
    Invoke-WebRequest 'https://raw.githubusercontent.com/catchorg/Catch2/v2.13.6/single_include/catch2/catch.hpp' -OutFile $include
}
& $python (Join-Path $PSScriptRoot 'GenerateUnitTestProject.py')
if($LASTEXITCODE){throw 'Test project generation failed'}
Import-Module (Join-Path $PSScriptRoot 'WindowsBuildTools.psm1') -Force
$msbuild=Get-KsmMSBuild
$output=Join-Path $root ('build/unit-tests/'+$Configuration)
New-Item -ItemType Directory -Path $output -Force|Out-Null
& $msbuild (Join-Path $root 'build/unit-project/ksm-v2-tests.vcxproj') /m:2 /p:Platform=x64 "/p:Configuration=$Configuration" "/p:SolutionDir=$root/" "/p:OutDir=$output/" /verbosity:minimal /nologo 2>&1|Tee-Object -FilePath (Join-Path $output 'build.log')
if($LASTEXITCODE){throw 'Unit test build failed'}
foreach($name in @('bass.dll','bass_fx.dll','bassasio.dll','bassmix.dll')){
    $source=Join-Path $root ('kshootmania/App/'+$name)
    if(Test-Path -LiteralPath $source){Copy-Item -LiteralPath $source -Destination (Join-Path $output $name) -Force}
}
foreach($name in @('engine','dll')){
    $source=Join-Path $root ('kshootmania/App/'+$name)
    if(Test-Path -LiteralPath $source){Copy-Item -LiteralPath $source -Destination $output -Recurse -Force}
}
$old=$env:KSM_TEST_REPORT
try{
    $env:KSM_TEST_REPORT=Join-Path $output 'results.xml'
    if(Test-Path -LiteralPath $env:KSM_TEST_REPORT){Remove-Item -LiteralPath $env:KSM_TEST_REPORT -Force}
    $process=Start-Process -FilePath (Join-Path $output 'ksm-v2-tests.exe') -WorkingDirectory $output -WindowStyle Hidden -PassThru
    if(-not $process.WaitForExit(120000)){throw "Unit test PID $($process.Id) timed out; left running"}
    if($process.ExitCode){throw "Unit tests failed; $output/results.xml"}
    if(-not(Test-Path -LiteralPath $env:KSM_TEST_REPORT)){throw 'Unit report missing'}
    [xml]$report=[IO.File]::ReadAllText($env:KSM_TEST_REPORT)
    if(@($report.testsuites.testsuite).Count -eq 0 -or [int]$report.testsuites.testsuite.tests -le 0){throw 'Unit report contains no tests'}
    if(@($report.testsuites.testsuite|Where-Object {[int]$_.failures -gt 0 -or [int]$_.errors -gt 0}).Count){throw "Unit assertions failed; $output/results.xml"}
    Write-Output "UNIT TEST PASS: $output/results.xml"
}finally{$env:KSM_TEST_REPORT=$old}
