@echo off
echo %*
setlocal
cd /d %~dp0

call python %CD%\Process.py

del %CD%\In\*.* /Q

pause