@echo off
setlocal
set EXE=%~dp0Builds\Release\TwoSpeakerExplorer.exe
if not exist "%EXE%" (
  echo Building Release...
  "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" "%~dp0TwoSpeakerExplorer.vcxproj" /p:Configuration=Release /p:Platform=x64 /v:minimal || exit /b 1
)
powershell -NoProfile -ExecutionPolicy Bypass -Command "Unblock-File -LiteralPath '%EXE%' -ErrorAction SilentlyContinue"
start "" "%EXE%"
