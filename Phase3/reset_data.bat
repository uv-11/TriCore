REM Purpose: Put the sample data back into Phase3\data (removes offers.csv and every change made in the program).
@echo off
setlocal

REM The six sample files come from Phase2\data. That folder is only read, never written.
if not exist "%~dp0..\Phase2\data\donors.csv" (
    echo Cannot find ..\Phase2\data. Nothing was changed.
    goto :finish
)

if not exist "%~dp0data" mkdir "%~dp0data"

echo Removing the files in Phase3\data ...
del /q "%~dp0data\*.*"

echo Copying the sample data from Phase2\data ...
copy /y "%~dp0..\Phase2\data\donors.csv" "%~dp0data\" > nul
copy /y "%~dp0..\Phase2\data\recipients.csv" "%~dp0data\" > nul
copy /y "%~dp0..\Phase2\data\donations.csv" "%~dp0data\" > nul
copy /y "%~dp0..\Phase2\data\transactions.csv" "%~dp0data\" > nul
copy /y "%~dp0..\Phase2\data\graph_nodes.csv" "%~dp0data\" > nul
copy /y "%~dp0..\Phase2\data\graph_edges.csv" "%~dp0data\" > nul

echo Done. Phase3\data holds the six sample files and no offers.

:finish
pause
