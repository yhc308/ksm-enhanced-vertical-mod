param([switch]$TestEngine)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$output=Join-Path $root 'build/native-installer'
New-Item -ItemType Directory -Path $output -Force|Out-Null
$vc='C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
$name=if($TestEngine){'SetupTestStub'}else{'SetupStub'}
$define=if($TestEngine){'/DKSM_INSTALLER_TEST=1'}else{''}
$testCommand=if($TestEngine){'
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /MT /O2 /utf-8 /W4 /DUNICODE /D_UNICODE /DNOMINMAX /DKSM_INSTALLER_TEST=1 /Isrc src\InstallerCore.cpp tests\CancelEngine.cpp /Fe"'+$output+'\CancelEngine.exe" /Fo"'+$output+'\\" /link /SUBSYSTEM:CONSOLE bcrypt.lib cabinet.lib ole32.lib
'}else{''}
$command=@"
@echo off
call "$vc" >nul
if errorlevel 1 exit /b 1
cd /d "$PSScriptRoot"
rc /nologo /fo "$output\$name.res" Setup.rc
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /MT /O2 /utf-8 /W4 /DUNICODE /D_UNICODE /DNOMINMAX $define /Isrc src\InstallerCore.cpp src\Main.cpp /Fo"$output\\" /Fe"$output\$name.exe" "$output\$name.res" /link /SUBSYSTEM:WINDOWS /MANIFEST:NO /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA bcrypt.lib cabinet.lib ole32.lib shell32.lib user32.lib gdi32.lib comctl32.lib
$testCommand
exit /b %errorlevel%
"@
$script=Join-Path $output "build-$name.cmd"
[IO.File]::WriteAllText($script,$command,[Text.Encoding]::ASCII)
& cmd.exe /d /c $script 2>&1|Tee-Object (Join-Path $output "build-$name.log")
if($LASTEXITCODE){throw 'Native installer compilation failed'}
Write-Output (Join-Path $output "$name.exe")
