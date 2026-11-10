REM Purpose: Build the Release version of CommunityBridge and put it, with its sample data, in Phase3\dist\CommunityBridge.
@echo off
setlocal

REM The release build has its own folder (Phase3\build-dist) and uses CMake and Ninja.
echo Preparing the release build ...
cmake -G Ninja -S "%~dp0." -B "%~dp0build-dist" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto :fail

echo Building ...
cmake --build "%~dp0build-dist"
if errorlevel 1 goto :fail

REM The install step copies the program, the sample data and reset_data.bat. The data folder is only made when it is
REM not there yet, so making the dist folder again never overwrites the changes a person made in the program.
echo Putting the program in Phase3\dist\CommunityBridge ...
cmake --install "%~dp0build-dist" --prefix "%~dp0dist\CommunityBridge"
if errorlevel 1 goto :fail

echo.
echo Done. Start Phase3\dist\CommunityBridge\communitybridge_gui.exe, or run create_desktop_shortcut.bat.
goto :finish

:fail
echo The build failed. Nothing was changed in the dist folder.
pause
exit /b 1

:finish
pause