python3 psxrecomp/psxrecomp_cli.py generate --config game.toml --project-root . --disc disc/AC1.cue
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target psx-runtime -j"$(nproc)"
