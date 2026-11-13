@echo off
rem Starts the game and keeps what it prints in splashdown-log.txt.
rem Drop a disc image on this file to use it; otherwise the first .iso
rem (or .bin) in this folder is used.
cd /d "%~dp0"
echo Starting Splashdown... (this window closes when the game exits)
splashdown.exe %* > splashdown-log.txt 2>&1
if errorlevel 1 (
  echo.
  echo The game stopped with an error. The last lines of splashdown-log.txt:
  echo.
  powershell -NoProfile -Command "Get-Content -Tail 25 splashdown-log.txt"
  echo.
  echo Please send splashdown-log.txt back with a note of what you were doing.
  pause
)
