@echo off
rem RecompLauncher setup step: builds the game from the player's disc without a window or prompts.
rem Players installing by hand use Install.cmd instead.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-MercCheatBuild.ps1" -Launcher -InstallDir "%~dp0." %*
exit /b %ERRORLEVEL%
