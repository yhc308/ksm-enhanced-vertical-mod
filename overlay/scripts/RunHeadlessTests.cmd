@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0.."
if not exist build\headless mkdir build\headless
cl /nologo /std:c++20 /EHsc /MT /utf-8 /Ibuild\deps\include /Ikshootmania\src tests\headless\RecentTimingWindowTests.cpp /Fobuild\headless\RecentTimingWindowTests.obj /Febuild\headless\recent-timing-tests.exe
if errorlevel 1 exit /b 1
build\headless\recent-timing-tests.exe --reporter junit --out build\headless\results.xml
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /MT /utf-8 /Ibuild\deps\include /Ikshootmania\src tests\headless\WindowsWaitableTimerTests.cpp /Fobuild\headless\WindowsWaitableTimerTests.obj /Febuild\headless\windows-timer-tests.exe
if errorlevel 1 exit /b 1
build\headless\windows-timer-tests.exe --reporter junit --out build\headless\windows-timer-results.xml
exit /b %errorlevel%
