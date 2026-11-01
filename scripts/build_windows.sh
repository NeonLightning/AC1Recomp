#!/usr/bin/env bash
# Windows release. On Linux it cross-compiles with MinGW-w64; under MSYS2/Git Bash it builds natively.
# Linux needs: mingw-w64-gcc, cmake, ninja, and either a MinGW SDL3 or -DPSX_SDL3_FETCH=ON (default here).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${1:-build/pkg-windows}"
BUILD_JOBS="${BUILD_JOBS:-$(nproc)}"

cd "${ROOT}"
args=(-S . -B "${BUILD_DIR}" -G Ninja
  -DCMAKE_BUILD_TYPE=Release
  -DPSX_ENABLE_VULKAN=ON
  -DPSX_SDL3_FETCH=ON
  -DPSXRECOMP_REQUIRE_GAME_C=ON)
TOOLCHAIN_FILE="${TOOLCHAIN_FILE:-}"
if [[ -z "${TOOLCHAIN_FILE}" && "$(uname -s)" == "Linux" ]]; then
  TOOLCHAIN_FILE="${ROOT}/psxrecomp/cmake/toolchain-mingw-w64.cmake"
fi
if [[ -n "${TOOLCHAIN_FILE}" ]]; then
  args+=(-DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}")
fi

cmake "${args[@]}"
# One target at a time: their shared asset staging races when linked in parallel.
for target in psx-armored-core psx-project-phantasma psx-master-of-arena psx-launcher; do
  cmake --build "${BUILD_DIR}" --target "${target}" -j"${BUILD_JOBS}"
done

exec scripts/package_release.sh "${BUILD_DIR}" windows-x64
