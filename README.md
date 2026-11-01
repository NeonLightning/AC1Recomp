# Armored Core (PSX Recompilation)

A native PC port of all three **Armored Core** games for playstation 1. Generated via static recompilation with enhanced rendering, widescreen support, customizable controls, and in-game settings.

---

## Building the Project

### Prerequisites

#### Linux (Arch Linux / Manjaro)
```bash
sudo pacman -S base-devel cmake ninja sdl3 freetype2 harfbuzz libpng zlib openssl vulkan-headers shaderc
```

#### Linux (Ubuntu / Debian 24.04+)
```bash
sudo apt-get update
sudo apt-get install build-essential cmake ninja-build libsdl3-dev libfreetype-dev libharfbuzz-dev libpng-dev zlib1g-dev libssl-dev libvulkan-dev glslc
```

### Compiling

The engine runtime and launcher UI are fully vendored in-tree. Configure and build with CMake and Ninja:

```bash
# Configure release build with Vulkan support
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DPSX_ENABLE_VULKAN=ON

# Compile all game executables
cmake --build build-release --target psx-armored-core psx-project-phantasma psx-master-of-arena psx-launcher -j$(nproc)
```

The executables are `build-release/AC1`,
`build-release/ArmoredCoreProjectPhantasma` and
`build-release/ArmoredCoreMasterOfArena`. Run `./launch.sh` from the project
root for the graphical launcher (game, disc and shared settings), or launch any executable directly.

Armored Core uses the USA `SCUS-94182` disc.

Armored Core: Master of Arena uses the USA `SLUS-01030` (Disc 1) and
`SLUS-01081` (Disc 2) profile; pick the disc in its launcher's Disc Selection.

Project Phantasma uses the USA `SLUS-00670` profile. Keep your legally dumped
disc in the configured local disc path or select it in that title's launcher;
disc images are not included in the repository.

### Creating an End-User Release Package

Build and package the game as a standalone ZIP with:

```bash
scripts/build_release.sh
```

The ZIP is written to `dist/` and contains the game executable and required
runtime files, not CMake/Ninja metadata or compiler object files. To choose a
build directory, artifact tag, or recompiler build directory:

```bash
scripts/build_release.sh build-release linux-x64 psxrecomp/recompiler/build
```

To build a Linux x86_64 AppImage instead, run:

```bash
scripts/build_appimage.sh
```

The AppImage is written to `dist/armored-core-recomp-<version>-linux-x64.AppImage`.
It downloads the pinned AppImage packaging tools on first use and caches them
under `${XDG_CACHE_HOME:-~/.cache}/recomp-appimage-tools`. User settings, disc
selection, memory cards, and saves live under `${XDG_DATA_HOME:-~/.local/share}/armored-core-recomp`;
existing writable data is carried forward when the packaged runtime changes.
The bundled OpenBIOS image and license are included; a retail PS1 BIOS dump is
not included and must be supplied by the user if selected.

---

## In-Game Settings Overlay

During gameplay, press the **Xbox (Guide) button** on a controller or the **Escape** key on a keyboard to open the in-game settings overlay. The game pauses while the overlay is open.

- **Navigation**:
  - **D-Pad / Left Stick** or **Arrow Keys**: Navigate rows and adjust values.
  - **A Button** or **Enter / Space**: Activate folders, toggle options, or confirm actions.
  - **B Button** or **Escape**: Go back or close the overlay.
  - **Guide Button**: Toggle overlay open/close.

### Overlay Options

#### Display
- **View mode**: Toggle between **16:9 Widescreen** (native-wide 1:1 camera extension) and **4:3 Standard**.
- **Fullscreen**: Switch between **Windowed**, **Borderless** desktop fullscreen, and **Exclusive** fullscreen.
- **Window scale**: Adjust window client resolution in integer steps (1x to 8x native height) in windowed mode.
- **VSync**: Choose between **Off** (lowest input latency), **On** (tear-free), and **Adaptive**.
- **Scanlines**: Enable CRT scanline post-processing filter.
- **Scanline strength**: Adjust CRT scanline intensity from 0% to 100%.
- **Frame interpolation**: Smooth motion frame blending.

#### Graphics
- **Resolution scale**: Internal rendering resolution from 1x native up to 8x.
- **Antialiasing**: Smooth present filtering across the frame.
- **Texture filtering**: Choose between **Nearest** (authentic pixelated) and **Linear** (smoothed).
- **FMV filtering**: Select scaling algorithm for full-motion video playback (**Nearest**, **Bilinear**, **Sharp**, **Bicubic**).
- **Screen filter**: Post-processing CRT display simulation presets (**Raw**, **CRT**, **Composite**, **Trinitron**).
- **Geometry precision**: PGXP sub-pixel vertex precision to eliminate 3D polygon jitter.
- **Perspective texturing**: PGXP perspective-correct texture mapping to prevent polygon warping.
- **Skip FMVs**: Automatically bypass pre-rendered video cutscenes on load.

#### Audio
- **Audio**: Toggle master sound output on or off.
- **Volume**: Adjust in-game mixer volume (0% to 100%).
- **SPU high quality**: Enable floating-point audio mixing shadow.

#### Input
- **Dual analog mode**: Toggle modern twin-stick movement for digital PS1 controls:
  - **Left Stick**: Forward/Back (D-Pad Up/Down) and Strafe (L1/R1).
  - **Right Stick**: Aim Up/Down (L2/R2) and Turn (D-Pad Left/Right).
- **Duplicate face on shoulders**: (Available when Dual analog mode is active) Duplicates physical face buttons to shoulder buttons for modern twin-stick combat:
  - **L1**: Triangle (Switch weapon)
  - **R1**: Cross (Boost)
  - **L2**: Circle (Energy blade)
  - **R2**: Square (Fire primary weapon)

#### System
- **Resume game**: Unpause and return to gameplay.
- **State slot**: Select save-state slot (1–12).
- **Save state**: Save immediate game snapshot to selected slot.
- **Load state**: Load snapshot from selected slot.
- **Fast boot**: Skip the PlayStation BIOS sequence and jump directly into the game.
- **HLE BIOS**: Toggle high-level BIOS kernel service interception.
- **Rewind feature**: Toggle memory rewind snapshot capture.
- **Restart game**: Save active settings and restart with the current disc and launch options.
- **Return to launcher**: Save active settings and reopen the graphical launcher.
- **Quit game**: Close the application.

---

## Launcher Configuration

When starting the application without skipping the launcher, the GUI offers additional configuration tabs:

- **Display Panel**: Select the graphics renderer, aspect ratio, internal resolution, VSync, scanlines, frame interpolation, Skip FMVs, geometry precision, and other presentation settings.
- **Controller Configuration**:
  - Rebind individual keyboard keys and controller buttons.
  - Set per-controller stick deadzone percentage.
  - Toggle **Dual analog mode** and **Duplicate face buttons on shoulders** with an on-screen reference legend.
- **Audio & System**: Mute or adjust audio, select host frequency, choose the savestate slot, manage memory cards, and switch disc images.
- **System settings**: Choose **Fast boot** to skip the BIOS startup sequence and **HLE BIOS** to enable high-level BIOS services; these match the in-game controls and persist in `settings.toml`.

Settings are saved automatically to [game.toml](games/armored-core/game.toml) defaults and your user profile in `settings.toml`.
