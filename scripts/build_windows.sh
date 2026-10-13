#!/usr/bin/env bash
# Windows release build. On Linux it cross-compiles with MinGW-w64; under MSYS2/Git Bash it builds natively.
# Linux needs: mingw-w64-gcc, a MinGW SDL2, cmake, ninja. Override with TOOLCHAIN_FILE.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${1:-build-windows}"
BUILD_JOBS="${BUILD_JOBS:-$(nproc)}"

cd "${ROOT}"
args=(-S . -B "${BUILD_DIR}" -G Ninja
  -DCMAKE_BUILD_TYPE=Release
  -DPSX_ENABLE_VULKAN=ON
  -DPSXRECOMP_REQUIRE_GAME_C=ON)
TOOLCHAIN_FILE="${TOOLCHAIN_FILE:-}"
if [[ -z "${TOOLCHAIN_FILE}" && "$(uname -s)" == "Linux" ]]; then
  TOOLCHAIN_FILE="${ROOT}/psxrecomp/cmake/toolchain-mingw-w64.cmake"
fi
if [[ -n "${TOOLCHAIN_FILE}" ]]; then
  args+=(-DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}")
fi

cmake "${args[@]}"
cmake --build "${BUILD_DIR}" --target psx-runtime -j"${BUILD_JOBS}"

# The bundled overlay toolchain needs a Windows psxrecomp-game.exe.
RECOMPILER_BUILD="${2:-build-recompiler-windows}"
rargs=(-S psxrecomp/recompiler -B "${RECOMPILER_BUILD}" -G Ninja -DCMAKE_BUILD_TYPE=Release)
if [[ -n "${TOOLCHAIN_FILE}" ]]; then
  rargs+=(-DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}")
fi
cmake "${rargs[@]}"
cmake --build "${RECOMPILER_BUILD}" --target psxrecomp-game psxrecomp-bios -j"${BUILD_JOBS}"

export RELEASE_VERSION="$(tr -d '[:space:]' <VERSION)"
exec scripts/package_release.sh "${BUILD_DIR}" windows-x64 "${RECOMPILER_BUILD}"
