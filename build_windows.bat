@echo off
REM ==========================================================
REM  Velvet Verb - compilation locale sous Windows
REM  Prerequis : Visual Studio 2022 (charge "Developpement Desktop en C++"),
REM              CMake et Git installes.
REM ==========================================================
cd /d "%~dp0"
cmake -B build -G "Visual Studio 17 2022" -A x64 || goto :err
cmake --build build --config Release --target VelvetVerb_VST3 --parallel || goto :err
echo.
echo  OK ! Le plugin est ici :
echo  %~dp0build\VelvetVerb_artefacts\Release\VST3\Velvet Verb.vst3
echo  Copie ce dossier dans C:\Program Files\Common Files\VST3\
echo.
pause
exit /b 0
:err
echo.
echo  La compilation a echoue. Verifie que Visual Studio 2022, CMake et Git sont installes.
pause
exit /b 1
