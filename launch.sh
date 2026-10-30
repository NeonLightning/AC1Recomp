#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILD_DIR=${AC1RECOMP_BUILD_DIR:-"$ROOT_DIR/build-release"}
AC1_EXE="$BUILD_DIR/AC1"
PHANTASMA_EXE="$BUILD_DIR/ArmoredCoreProjectPhantasma"

printf '%s\n' 'Armored Core Recomp' '' '1) Armored Core' '2) Project Phantasma' 'q) Quit'
printf '%s' '> '
IFS= read -r choice

case "$choice" in
    1)
        [ -x "$AC1_EXE" ] || { printf 'Missing executable: %s\n' "$AC1_EXE" >&2; exit 1; }
        exec "$AC1_EXE" "$@"
        ;;
    2)
        [ -x "$PHANTASMA_EXE" ] || { printf 'Missing executable: %s\n' "$PHANTASMA_EXE" >&2; exit 1; }
        exec "$PHANTASMA_EXE" "$@"
        ;;
    q|Q)
        exit 0
        ;;
    *)
        printf 'Unknown selection: %s\n' "$choice" >&2
        exit 2
        ;;
esac
