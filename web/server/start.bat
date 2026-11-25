@echo off
cd /d "%~dp0"
set DOTNET=dotnet
if exist "%LOCALAPPDATA%\Microsoft\dotnet\dotnet.exe" set DOTNET="%LOCALAPPDATA%\Microsoft\dotnet\dotnet.exe"
start "" /min cmd /c "timeout /t 10 >nul & start http://localhost:5080"
%DOTNET% run
