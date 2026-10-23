@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0.."
cl /nologo /std:c++20 /EHsc /MT /O2 /utf-8 /Iksmaudio\include tests\asio\QADiagnosticsTest.cpp /Fotests\asio\QADiagnosticsTest.obj /Fetests\asio\qa-diagnostics-test.exe
exit /b %errorlevel%
