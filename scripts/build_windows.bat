@echo off
setlocal

rem Run from a shell with cmake, ninja and a compiler on PATH; packaging needs Git Bash or MSYS2 bash.
set "ROOT=%~dp0.."
set "BUILD_DIR=%~1"
if "%BUILD_DIR%"=="" set "BUILD_DIR=build/pkg-windows"
if "%BUILD_JOBS%"=="" set "BUILD_JOBS=%NUMBER_OF_PROCESSORS%"

cd /d "%ROOT%" || exit /b 1

cmake -S . -B "%BUILD_DIR%" -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DPSX_ENABLE_VULKAN=ON ^
  -DPSX_SDL3_FETCH=ON ^
  -DPSXRECOMP_REQUIRE_GAME_C=ON || exit /b 1

for %%T in (psx-armored-core psx-project-phantasma psx-master-of-arena psx-launcher) do (
    cmake --build "%BUILD_DIR%" --target %%T -j %BUILD_JOBS% || exit /b 1
)

bash scripts/package_release.sh "%BUILD_DIR%" windows-x64 || exit /b 1

endlocal
