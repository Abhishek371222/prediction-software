@echo off
setlocal
cd /d "%~dp0"

set "CMAKE=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

if not exist "%CMAKE%" (
  echo CMake not found at %CMAKE%
  exit /b 1
)

call "%VCVARS%"
if errorlevel 1 exit /b 1

"%CMAKE%" -S . -B build -G "Visual Studio 18 2026" -A x64
if errorlevel 1 (
  echo Trying Visual Studio 17 2022 generator...
  "%CMAKE%" -S . -B build -G "Visual Studio 17 2022" -A x64
)
if errorlevel 1 exit /b 1

"%CMAKE%" --build build --config Release
if errorlevel 1 exit /b 1

echo.
echo Built: build\AtomikPolar_artefacts\Release\Atomik Polar.exe
start "" "build\AtomikPolar_artefacts\Release\Atomik Polar.exe"
endlocal
