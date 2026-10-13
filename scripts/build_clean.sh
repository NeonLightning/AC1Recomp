#!/usr/bin/env bash
# Removes build outputs, packaging output, caches, logs and run artifacts, then empty directories.
# Never touches disc/, saves/, bios/, generated/, ghidra/ or .git. Use --dry-run to preview.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "${ROOT}"

DRY=0
[[ "${1:-}" == "--dry-run" ]] && DRY=1

rm_path() {
  if [[ "${DRY}" == 1 ]]; then echo "would remove ${1}"; else rm -rf -- "${1}"; echo "removed ${1}"; fi
}

# Build / packaging output
build_dirs=()
for d in build build-* out release-stage dist .cache .happypath \
         psxrecomp/recompiler/build psxrecomp/build; do
  if [[ -e "${d}" ]]; then rm_path "${d}"; build_dirs+=( -o -path "./${d}" ); fi
done

# Caches, logs and run artifacts anywhere in the tree (skipping protected dirs)
prune=( -path ./.git -o -path ./disc -o -path ./saves -o -path ./bios -o -path ./generated -o -path ./ghidra "${build_dirs[@]}" )
while IFS= read -r -d '' p; do
  rm_path "${p}"
done < <(find . \( "${prune[@]}" \) -prune -o \
  \( -name __pycache__ -o -name '*.pyc' -o -name '*.log' -o -name psx_crash.txt \
     -o -name psx_last_run_report.json -o -name psx_freeze_heartbeat.json \
     -o -name psx_screenshot.bmp -o -name starvation_dump.jsonl \
     -o -name .psxrecomp_last_generate -o -name .psxrecomp_write_probe \) \
  -print0 | sort -z)

# Empty directories, leaving uninitialised submodule paths alone
submodules=" "
if [[ -f .gitmodules ]]; then
  while read -r _ sp; do submodules+="./${sp} "; done \
    < <(git config -f .gitmodules --get-regexp 'submodule\..*\.path')
fi
while IFS= read -r -d '' d; do
  [[ "${submodules}" == *" ${d} "* ]] && continue
  if [[ "${DRY}" == 1 ]]; then echo "would remove ${d}"; else rmdir -- "${d}" && echo "removed ${d}"; fi
done < <(find . -mindepth 1 \( "${prune[@]}" \) -prune -o \
  -type d -empty -print0 | sort -rz)
