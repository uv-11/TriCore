REM Purpose: Build the CommunityBridge window program and start it on Phase3\data. Extra words go on to the program (for example --dpi-log).
@echo off
setlocal
set "EXE=%~dp0build\communitybridge_gui.exe"

REM The first time the build folder does not exist yet, so it is made with CMake and Ninja.
if not exist "%~dp0build\CMakeCache.txt" (
    echo Preparing the build folder ...
    cmake -G Ninja -S "%~dp0." -B "%~dp0build"
    if errorlevel 1 goto :build_fail
)

echo Building ...
cmake --build "%~dp0build"
if errorlevel 1 goto :build_fail
if not exist "%EXE%" goto :build_fail

REM Only a program that was just built is started, and the console says which file it is.
for %%F in ("%EXE%") do echo Starting %%~fF built %%~tF
start "" "%EXE%" "%~dp0data" %*
exit /b 0

:build_fail
echo The build failed. Nothing was started.
pause
exit /b 1
