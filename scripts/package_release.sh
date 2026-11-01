#!/usr/bin/env bash
# Packages the launcher and all three games from a built runtime folder into
# <name>.zip (Windows) or <name>.tar.gz (Linux) under dist/.
#
# Usage: scripts/package_release.sh <runtime-dir> <artifact-tag>
#   runtime-dir   folder holding the built executables (CMake build dir or build-release/)
#   artifact-tag  linux-x64 | windows-x64
#
# Windows DLLs are taken from the runtime dir plus any dirs listed in
# PSXRECOMP_RUNTIME_BIN_DIR (":"-separated) and the MinGW runtime.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RUNTIME_DIR="${1:?usage: $0 <runtime-dir> <artifact-tag>}"
TAG="${2:?usage: $0 <runtime-dir> <artifact-tag>}"
cd "${ROOT}"

VERSION="${RELEASE_VERSION:-$(tr -d '[:space:]' <VERSION)}"
NAME="armored-core-recomp-${VERSION}-${TAG}"
STAGE="${ROOT}/dist/stage/${NAME}"
EXT=""
[[ "${TAG}" == windows* ]] && EXT=".exe"

EXES=(ArmoredCoreLauncher AC1 ArmoredCoreProjectPhantasma ArmoredCoreMasterOfArena)
for exe in "${EXES[@]}"; do
  [[ -f "${RUNTIME_DIR}/${exe}${EXT}" ]] || { echo "error: ${RUNTIME_DIR}/${exe}${EXT} not found" >&2; exit 1; }
done

rm -rf "${STAGE}"
mkdir -p "${STAGE}"

# Runnable files only: no CMake/Ninja state, build metadata, or this machine's settings.
rsync -a \
  --exclude='CMakeFiles/' --exclude='CMakeCache.txt' --exclude='build.ninja' \
  --exclude='cmake_install.cmake' --exclude='.ninja_*' --exclude='*_vkgen/' \
  --exclude='*.a' --exclude='*.c' --exclude='*.rc' --exclude='*_generated_sources.txt' \
  --exclude='psx_mod_catalog_*.txt' --exclude='psxrecomp_*.txt' \
  --exclude='Testing/' --exclude='CTestTestfile.cmake' --exclude='_deps/' --exclude='psx_execution/' \
  --exclude='launcher.ini' --exclude='launcher-window.ini' --exclude='config.ini' \
  --exclude='settings.toml' --exclude='keybinds.ini' --exclude='input.ini' \
  --exclude='disc.cfg' --exclude='bios.cfg' --exclude='rom.cfg' \
  --exclude='saves/' --exclude='*.log' --exclude='psx_crash.txt' \
  --exclude='psx_last_run_report.json' \
  "${RUNTIME_DIR}"/ "${STAGE}"/

if [[ -n "${EXT}" ]]; then
  IFS=':' read -r -a dll_dirs <<<"${PSXRECOMP_RUNTIME_BIN_DIR:-}"
  dll_dirs+=("${RUNTIME_DIR}" /usr/x86_64-w64-mingw32/bin)
  for d in "${dll_dirs[@]}"; do
    [[ -d "${d}" ]] || continue
    for dll in SDL3.dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll; do
      [[ -f "${d}/${dll}" && ! -f "${STAGE}/${dll}" ]] && cp "${d}/${dll}" "${STAGE}/"
    done
  done
fi

# Discs are the player's own: each game gets an empty disc folder with a note.
declare -A DISC_NOTE=(
  [armored-core]="Armored Core (USA, SCUS-94182): Armored Core.cue + Armored Core.bin"
  [project-phantasma]="Armored Core: Project Phantasma (USA, SLUS-00670): Armored Core - Project Phantasma (USA).cue + .bin"
  [master-of-arena]="Armored Core: Master of Arena (USA, SLUS-01030 and SLUS-01081): Armored Core - Master of Arena (USA) (Disc 1).cue/.bin and (Disc 2).cue/.bin"
)
for id in "${!DISC_NOTE[@]}"; do
  [[ -f "${STAGE}/games/${id}/game.toml" ]] || { echo "error: games/${id}/game.toml missing from ${RUNTIME_DIR}" >&2; exit 1; }
  mkdir -p "${STAGE}/games/${id}/disc"
  printf 'Put your legally owned disc image here, or pick it in the launcher.\n\n%s\n' \
    "${DISC_NOTE[$id]}" >"${STAGE}/games/${id}/disc/README.txt"
done

mkdir -p "${STAGE}/licenses"
cp psxrecomp/LICENSE "${STAGE}/licenses/psxrecomp-LICENSE.txt"
cp psxrecomp/THIRD_PARTY_ATTRIBUTION.md "${STAGE}/licenses/THIRD_PARTY_ATTRIBUTION.md"
cp recomp-ui/LICENSE "${STAGE}/licenses/recomp-ui-LICENSE.txt"

LAUNCH="ArmoredCoreLauncher${EXT}"
cat >"${STAGE}/README.txt" <<EOF
Armored Core Recomp ${VERSION} (${TAG})

Run ${LAUNCH}. Pick a game, set its disc image, adjust the settings and
controls shared by every game, then press PLAY.

Games: Armored Core, Armored Core: Project Phantasma, Armored Core: Master of Arena.
You need your own legally owned discs; none are included. Disc folders are in
games/<game>/disc/. Memory cards are stored in saves/ and shared by all games.

Each game also has its own executable (${EXES[1]}${EXT}, ${EXES[2]}${EXT}, ${EXES[3]}${EXT}).
EOF

mkdir -p dist
if [[ -n "${EXT}" ]]; then
  ARCHIVE="${NAME}.zip"
  rm -f "dist/${ARCHIVE}"
  (cd dist/stage && zip -qr "../${ARCHIVE}" "${NAME}")
else
  ARCHIVE="${NAME}.tar.gz"
  rm -f "dist/${ARCHIVE}"
  (cd dist/stage && tar -czf "../${ARCHIVE}" "${NAME}")
fi
rm -rf dist/stage
echo "Wrote dist/${ARCHIVE}"
