#!/bin/sh
# Builds the launcher and all three games for Linux and Windows (MinGW cross-compile).
# Usage: ./build.sh [linux|windows|all]   (default: all)
# Outputs: build-release/ (Linux) and build-windows/ (Windows), runnable files only.
# CMake/Ninja state stays in build/linux and build/windows.
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TARGETS="psx-armored-core psx-project-phantasma psx-master-of-arena psx-launcher"
JOBS=$(nproc)

# One target at a time: their shared asset staging races when linked in parallel.
build_targets() {
    for t in $TARGETS; do
        cmake --build "$1" --target "$t" -j"$JOBS"
    done
}

# Only runnable files go to the output dir.
stage() {
    mkdir -p "$2"
    rsync -a \
        --exclude='CMakeFiles/' --exclude='CMakeCache.txt' --exclude='build.ninja' \
        --exclude='cmake_install.cmake' --exclude='.ninja_*' --exclude='*_vkgen/' \
        --exclude='*.a' --exclude='*.c' --exclude='*.rc' --exclude='*_generated_sources.txt' \
        --exclude='psx_mod_catalog_*.txt' --exclude='psxrecomp_*.txt' \
        --exclude='Testing/' --exclude='CTestTestfile.cmake' --exclude='_deps/' \
        "$1"/ "$2"/
}

build_linux() {
    work="${AC1RECOMP_WORK_DIR:-$ROOT_DIR/build/linux}"
    out="${AC1RECOMP_BUILD_DIR:-$ROOT_DIR/build-linux}"
    cmake -S "$ROOT_DIR" -B "$work" -G Ninja -DCMAKE_BUILD_TYPE=Release
    build_targets "$work"
    stage "$work" "$out"
}

build_windows() {
    work="$ROOT_DIR/build/windows"
    out="${AC1RECOMP_WINDOWS_DIR:-$ROOT_DIR/build-windows}"
    cmake -S "$ROOT_DIR" -B "$work" -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DPSX_SDL3_FETCH=ON \
        -DCMAKE_TOOLCHAIN_FILE="$ROOT_DIR/psxrecomp/cmake/toolchain-mingw-w64.cmake"
    build_targets "$work"
    stage "$work" "$out"
    for dll in libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll; do
        [ -f "/usr/x86_64-w64-mingw32/bin/$dll" ] && cp "/usr/x86_64-w64-mingw32/bin/$dll" "$out/"
    done
}

case "${1:-all}" in
    linux) build_linux ;;
    windows) build_windows ;;
    all) build_linux; build_windows ;;
    *) echo "usage: $0 [linux|windows|all]" >&2; exit 2 ;;
esac
