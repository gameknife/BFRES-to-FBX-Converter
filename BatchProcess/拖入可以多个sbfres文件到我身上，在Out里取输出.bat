@echo off
echo %*
setlocal
cd /d %~dp0

for %%x in (%*) do (
  copy %%x %CD%\In
)

call python %CD%\Process.py

del %CD%\In\*.* /Q

pause