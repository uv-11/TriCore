REM Purpose: Remove the CommunityBridge icon from the Desktop. Only the shortcut file is removed; the program stays.
@echo off
setlocal

REM The shortcut is looked for on the Desktop of this person, unless the variable CB_SHORTCUT_DIR names another folder (used for tests).
powershell -NoProfile -ExecutionPolicy Bypass -Command "$dir = $env:CB_SHORTCUT_DIR; if (-not $dir) { $dir = [Environment]::GetFolderPath('Desktop') }; $link = Join-Path $dir 'CommunityBridge.lnk'; if (Test-Path -LiteralPath $link) { Remove-Item -LiteralPath $link -Force; Write-Host ('Shortcut removed: ' + $link) } else { Write-Host ('There is no shortcut at ' + $link) }"

pause