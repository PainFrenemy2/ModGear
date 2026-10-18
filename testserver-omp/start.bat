@echo off
cd /d "%~dp0"
taskkill /f /im omp-server.exe >nul 2>&1
start "" "omp-server.exe"
