@echo off
setlocal

rem Run from a shell where cmake, ninja and a compiler (MSVC "x64 Native Tools" prompt or MinGW) are on PATH.
set "ROOT=%~dp0.."
set "BUILD_DIR=%~1"
if "%BUILD_DIR%"=="" set "BUILD_DIR=build-windows"
if "%BUILD_JOBS%"=="" set "BUILD_JOBS=%NUMBER_OF_PROCESSORS%"

cd /d "%ROOT%" || exit /b 1

cmake -S . -B "%BUILD_DIR%" -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DPSX_ENABLE_VULKAN=ON ^
  -DPSXRECOMP_REQUIRE_GAME_C=ON || exit /b 1

cmake --build "%BUILD_DIR%" --target psx-runtime -j %BUILD_JOBS% || exit /b 1

rem Packaging is a bash script; needs Git Bash or MSYS2 bash on PATH.
set "RECOMPILER_BUILD=%~2"
if "%RECOMPILER_BUILD%"=="" set "RECOMPILER_BUILD=psxrecomp/recompiler/build"
set /p RELEASE_VERSION=<VERSION
bash scripts/package_release.sh "%BUILD_DIR%" windows-x64 "%RECOMPILER_BUILD%" || exit /b 1

endlocal
