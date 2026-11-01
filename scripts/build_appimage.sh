#!/usr/bin/env bash
# Build the Linux runtime and wrap it as a self-contained AppImage.
# Usage: scripts/build_appimage.sh [build-dir] [output.AppImage]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${1:-build/pkg-linux}"
VERSION="$(tr -d '[:space:]' <"${ROOT}/VERSION")"
if [[ -n "${RELEASE_VERSION:-}" ]]; then VERSION="${RELEASE_VERSION}"; fi
NAME="armored-core-recomp-${VERSION}-linux-x64"
OUTPUT="${2:-${ROOT}/dist/${NAME}.AppImage}"
JOBS="${BUILD_JOBS:-$(nproc)}"
cd "${ROOT}"

cmake -S . -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DPSX_ENABLE_VULKAN=ON \
  -DPSXRECOMP_REQUIRE_GAME_C=ON
for target in psx-armored-core psx-project-phantasma psx-master-of-arena psx-launcher; do
  cmake --build "${BUILD_DIR}" --target "${target}" -j"${JOBS}"
done

scripts/package_release.sh "${BUILD_DIR}" linux-x64 --stage-only
PAYLOAD="${ROOT}/dist/stage/${NAME}"
for file in bios/openbios.bin bios/OpenBIOS.LICENSE; do
    [[ -s "${PAYLOAD}/${file}" ]] || {
        echo "error: required bundled BIOS file ${file} is missing from ${PAYLOAD}" >&2
        exit 1
    }
done
TEMP="$(mktemp -d "${TMPDIR:-/tmp}/ac1-appimage.XXXXXXXX")"
trap 'rm -rf "${TEMP}"' EXIT

BUILD_ID="$( { printf 'appimage-runtime-layout-v2\n'; sha256sum "${PAYLOAD}/ArmoredCoreLauncher" \
    "${PAYLOAD}/AC1" "${PAYLOAD}/ArmoredCoreProjectPhantasma" \
    "${PAYLOAD}/ArmoredCoreMasterOfArena"; } | sha256sum | cut -c1-16)"
cat >"${TEMP}/AppRun" <<EOF
#!/bin/sh
set -eu
APPDIR=\$(CDPATH= cd -- "\$(dirname -- "\$0")" && pwd)
PAYLOAD="\$APPDIR/usr/share/${NAME}"
DATA_ROOT="\${XDG_DATA_HOME:-\${HOME:?HOME must be set}/.local/share}/armored-core-recomp"
RUNTIME="\$DATA_ROOT/runtime-${VERSION}-${BUILD_ID}"
APP_EXE="AC1"
mkdir -p "\$DATA_ROOT"

if [ ! -d "\$RUNTIME" ]; then
    TEMP_RUNTIME=\$(mktemp -d "\$DATA_ROOT/.runtime.XXXXXXXX")
    cp -a "\$PAYLOAD/." "\$TEMP_RUNTIME/"
    cp -a "\$APPDIR/usr/bin/\$APP_EXE" "\$TEMP_RUNTIME/\$APP_EXE"
    cp -a "\$APPDIR/usr/bin/\$APP_EXE.execution.json" "\$TEMP_RUNTIME/\$APP_EXE.execution.json"
    PREVIOUS=\$(find "\$DATA_ROOT" -mindepth 1 -maxdepth 1 -type d -name 'runtime-*' ! -path "\$RUNTIME" -printf '%T@ %p\\n' | sort -nr | sed -n '1s/^[^ ]* //p')
    if [ -n "\$PREVIOUS" ]; then
        for game in armored-core project-phantasma master-of-arena; do
            for file in game.toml game_options.toml settings.toml keybinds.ini input.ini; do
                if [ -f "\$PREVIOUS/games/\$game/\$file" ]; then
                    cp -a "\$PREVIOUS/games/\$game/\$file" "\$TEMP_RUNTIME/games/\$game/\$file"
                fi
            done
            if [ -d "\$PREVIOUS/games/\$game/disc" ]; then
                mkdir -p "\$TEMP_RUNTIME/games/\$game/disc"
                cp -a "\$PREVIOUS/games/\$game/disc/." "\$TEMP_RUNTIME/games/\$game/disc/"
            fi
        done
        for file in launcher.ini launcher-window.ini; do
            if [ -f "\$PREVIOUS/\$file" ]; then cp -a "\$PREVIOUS/\$file" "\$TEMP_RUNTIME/\$file"; fi
        done
        if [ -d "\$PREVIOUS/saves" ]; then
            mkdir -p "\$TEMP_RUNTIME/saves"
            cp -a "\$PREVIOUS/saves/." "\$TEMP_RUNTIME/saves/"
        fi
    fi
    if ! mv -T "\$TEMP_RUNTIME" "\$RUNTIME" 2>/dev/null; then
        rm -rf "\$TEMP_RUNTIME"
    fi
fi

export LD_LIBRARY_PATH="\$APPDIR/usr/lib\${LD_LIBRARY_PATH:+:\$LD_LIBRARY_PATH}"
unset APPIMAGE
exec "\$RUNTIME/ArmoredCoreLauncher" "\$@"
EOF
chmod 0755 "${TEMP}/AppRun"
cat >"${TEMP}/armored-core-recomp.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=Armored Core Recomp
Exec=AppRun
Icon=armored-core-recomp
Categories=Game;
Terminal=false
EOF

mkdir -p "$(dirname "${OUTPUT}")"
"${ROOT}/psxrecomp/tools/package_appimage.sh" \
  --framework "${ROOT}/psxrecomp" \
  --payload "${PAYLOAD}" \
  --exe-name AC1 \
  --payload-name "${NAME}" \
  --app-run "${TEMP}/AppRun" \
  --desktop-file "${TEMP}/armored-core-recomp.desktop" \
  --icon "${ROOT}/assets/psxrecomp.png" \
  --output "${OUTPUT}" \
  --jobs "${JOBS}"

rm -rf "${PAYLOAD}"
echo "Built ${OUTPUT}"