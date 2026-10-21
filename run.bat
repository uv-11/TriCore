c

REM Purpose: Build and run CommunityBridge Phase 2 main application.
@echo off
setlocal

set DATA_DIR=Phase2\data\
set DO_PAUSE=1

if /i "%~1"=="nopause" set DO_PAUSE=0
if /i "%~2"=="nopause" set DO_PAUSE=0

if not "%~1"=="" if /i not "%~1"=="nopause" set DATA_DIR=%~1
if not "%~2"=="" if /i not "%~2"=="nopause" set DATA_DIR=%~2

echo Building CommunityBridge...
g++ -std=c++17 -Wall -Wextra -IPhase2/include Phase2/src/ConsoleView.cpp Phase2/src/Datamanager.cpp Phase2/src/Duration.cpp Phase2/src/EntityId.cpp Phase2/src/Exceptions.cpp Phase2/src/Models.cpp Phase2/src/PriorityQueue.cpp Phase2/src/RecommendationEngine.cpp Phase2/src/allocationmanager.cpp Phase2/src/graph.cpp Phase2/src/hashindex.cpp Phase2/src/intstack.cpp Phase2/src/main.cpp Phase2/src/traveltime.cpp -o Phase2/CommunityBridge.exe
if errorlevel 1 goto :build_fail

echo Running CommunityBridge...
Phase2\CommunityBridge.exe %DATA_DIR%
goto :finish

:build_fail
echo Build failed.

:finish
if "%DO_PAUSE%"=="1" pause
