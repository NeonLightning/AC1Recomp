#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
WORK_DIR=${AC1RECOMP_WORK_DIR:-"$ROOT_DIR/build"}
OUT_DIR=${AC1RECOMP_BUILD_DIR:-"$ROOT_DIR/build-release"}

cmake -S "$ROOT_DIR" -B "$WORK_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$WORK_DIR" --target psx-armored-core psx-project-phantasma psx-master-of-arena psx-launcher -j"$(nproc)"

# Only runnable files go to the output dir; CMake/Ninja state stays in $WORK_DIR.
mkdir -p "$OUT_DIR"
rsync -a \
    --exclude='CMakeFiles/' --exclude='CMakeCache.txt' --exclude='build.ninja' \
    --exclude='cmake_install.cmake' --exclude='.ninja_*' --exclude='*_vkgen/' \
    --exclude='*.a' --exclude='*.c' --exclude='*_generated_sources.txt' \
    --exclude='psx_mod_catalog_*.txt' --exclude='psxrecomp_*.txt' \
    --exclude='Testing/' --exclude='CTestTestfile.cmake' --exclude='_deps/' \
    "$WORK_DIR"/ "$OUT_DIR"/
