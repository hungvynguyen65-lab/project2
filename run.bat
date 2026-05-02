@echo off
setlocal
cd /d "%~dp0"
powershell -ExecutionPolicy Bypass -File "%~dp0.vscode\run-c.ps1" "%~dp0src\main.c" %*
