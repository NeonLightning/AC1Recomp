#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILD_DIR=${AC1RECOMP_BUILD_DIR:-"$ROOT_DIR/build-release"}
LAUNCHER="$BUILD_DIR/ArmoredCoreLauncher"

[ -x "$LAUNCHER" ] || { printf 'Missing executable: %s\n' "$LAUNCHER" >&2; exit 1; }
exec "$LAUNCHER" "$@"
