@echo off
REM Purpose: refresh the demo data copy and run CommunityBridge in one step.
setlocal
if exist Phase2\data_check rmdir /s /q Phase2\data_check
xcopy /E /I /Y Phase2\data Phase2\data_check >nul
g++ -std=c++17 -Wall -Wextra -IPhase2/include Phase2/src/ConsoleView.cpp Phase2/src/Datamanager.cpp Phase2/src/Duration.cpp Phase2/src/EntityId.cpp Phase2/src/Exceptions.cpp Phase2/src/Models.cpp Phase2/src/PriorityQueue.cpp Phase2/src/RecommendationEngine.cpp Phase2/src/allocationmanager.cpp Phase2/src/graph.cpp Phase2/src/hashindex.cpp Phase2/src/intstack.cpp Phase2/src/main.cpp Phase2/src/traveltime.cpp -o Phase2/CommunityBridge.exe
if errorlevel 1 (
    echo Build failed.
    pause
    exit /b 1
)
Phase2\CommunityBridge.exe Phase2\data_check\
pause