@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0.."
if not defined KSM_AUDIO_TEST_LIBRARY_DIR exit /b 2
if not defined KSM_AUDIO_TEST_RUNTIME_DIR exit /b 2
if not exist "%KSM_AUDIO_TEST_RUNTIME_DIR%" mkdir "%KSM_AUDIO_TEST_RUNTIME_DIR%"
cl /nologo /std:c++20 /EHsc /MT /utf-8 /Iksmaudio\include /Iksmaudio\ThirdParty_Windows\bass /Iksmaudio\ThirdParty_Windows\bass_fx tests\asio\AudioOutputTest.cpp "/Fo%KSM_AUDIO_TEST_RUNTIME_DIR%\AudioOutputTest.obj" "/Fe%KSM_AUDIO_TEST_RUNTIME_DIR%\audio-output-test.exe" "%KSM_AUDIO_TEST_LIBRARY_DIR%\ksmaudio.lib" ksmaudio\ThirdParty_Windows\bass\bass.lib ksmaudio\ThirdParty_Windows\bass_fx\bass_fx.lib /link /LTCG user32.lib ole32.lib
if errorlevel 1 exit /b 1
cl /nologo /EHsc /MT /LD tests\asio\FakeAsio.cpp "/Fo%KSM_AUDIO_TEST_RUNTIME_DIR%\FakeAsio.obj" /link "/OUT:%KSM_AUDIO_TEST_RUNTIME_DIR%\bassasio.dll" "/IMPLIB:%KSM_AUDIO_TEST_RUNTIME_DIR%\FakeAsio.lib"
if errorlevel 1 exit /b 1
for %%F in (bass.dll bass_fx.dll bassmix.dll) do copy /y "kshootmania\App\%%F" "%KSM_AUDIO_TEST_RUNTIME_DIR%\" >nul
exit /b 0