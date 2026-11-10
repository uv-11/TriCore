REM Purpose: Put a CommunityBridge icon on the Desktop. It starts Phase3\dist\CommunityBridge\communitybridge_gui.exe.
@echo off
setlocal

set "CB_TARGET=%~dp0dist\CommunityBridge\communitybridge_gui.exe"
set "CB_WORKDIR=%~dp0dist\CommunityBridge"

if not exist "%CB_TARGET%" (
    echo The program is not in the dist folder yet. Run make_dist.bat first.
    goto :finish
)

REM The shortcut goes to the Desktop of this person, unless the variable CB_SHORTCUT_DIR names another folder (used for tests).
powershell -NoProfile -ExecutionPolicy Bypass -Command "$dir = $env:CB_SHORTCUT_DIR; if (-not $dir) { $dir = [Environment]::GetFolderPath('Desktop') }; if (-not (Test-Path -LiteralPath $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }; $link = Join-Path $dir 'CommunityBridge.lnk'; $shell = New-Object -ComObject WScript.Shell; $s = $shell.CreateShortcut($link); $s.TargetPath = $env:CB_TARGET; $s.WorkingDirectory = $env:CB_WORKDIR; $s.IconLocation = $env:CB_TARGET + ',0'; $s.Description = 'CommunityBridge'; $s.Save(); Write-Host ('Shortcut made: ' + $link)"
if errorlevel 1 echo The shortcut could not be made.

:finish
pause