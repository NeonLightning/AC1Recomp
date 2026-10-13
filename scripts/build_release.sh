#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${1:-build-release}"
ARTIFACT="${2:-linux-x64}"
RECOMPILER_BUILD="${3:-psxrecomp/recompiler/build}"
BUILD_JOBS="${BUILD_JOBS:-$(nproc)}"

cd "${ROOT}"
cmake -S . -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DPSX_ENABLE_VULKAN=ON \
  -DPSXRECOMP_REQUIRE_GAME_C=ON
cmake --build "${BUILD_DIR}" --target psx-runtime -j"${BUILD_JOBS}"

export RELEASE_VERSION="$(tr -d '[:space:]' <VERSION)"
exec scripts/package_release.sh \
  "${BUILD_DIR}" "${ARTIFACT}" "${RECOMPILER_BUILD}"