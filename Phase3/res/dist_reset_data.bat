REM Purpose: Put the sample data back in the data folder next to this file (removes offers.csv and every change made in the program).
@echo off
setlocal

REM The six sample files are kept in sample_data next to this file.
if not exist "%~dp0sample_data\donors.csv" (
    echo Cannot find the sample_data folder next to this file. Nothing was changed.
    goto :finish
)

if not exist "%~dp0data" mkdir "%~dp0data"

echo Copying the sample data ...
copy /y "%~dp0sample_data\donors.csv" "%~dp0data\" > nul
copy /y "%~dp0sample_data\recipients.csv" "%~dp0data\" > nul
copy /y "%~dp0sample_data\donations.csv" "%~dp0data\" > nul
copy /y "%~dp0sample_data\transactions.csv" "%~dp0data\" > nul
copy /y "%~dp0sample_data\graph_nodes.csv" "%~dp0data\" > nul
copy /y "%~dp0sample_data\graph_edges.csv" "%~dp0data\" > nul

REM The offers are not part of the sample data.
if exist "%~dp0data\offers.csv" del /q "%~dp0data\offers.csv"

echo Done. The data folder holds the six sample files and no offers.

:finish
pause