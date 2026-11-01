#!/usr/bin/env bash
# Linux release: builds the launcher and all three games, then packages them.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${1:-build/pkg-linux}"
BUILD_JOBS="${BUILD_JOBS:-$(nproc)}"

cd "${ROOT}"
cmake -S . -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DPSX_ENABLE_VULKAN=ON \
  -DPSXRECOMP_REQUIRE_GAME_C=ON
# One target at a time: their shared asset staging races when linked in parallel.
for target in psx-armored-core psx-project-phantasma psx-master-of-arena psx-launcher; do
  cmake --build "${BUILD_DIR}" --target "${target}" -j"${BUILD_JOBS}"
done

exec scripts/package_release.sh "${BUILD_DIR}" linux-x64
