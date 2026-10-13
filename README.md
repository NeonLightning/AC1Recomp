# Armored Core (PSX Recompilation)

A native PC port of **Armored Core** (USA / SCUS-94182) generated via static recompilation with enhanced rendering, widescreen support, customizable controls, and in-game settings.

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

# Compile executable
cmake --build build-release --target psx-runtime -j$(nproc)
```

The resulting executable will be at `build-release/AC1`.

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
- **Return to launcher**: Save active settings and reopen the graphical launcher.
- **Quit game**: Close the application.

---

## Launcher Configuration

When starting the application without skipping the launcher, the GUI offers additional configuration tabs:

- **Display Panel**: Select active graphics renderer (**OpenGL**, **Vulkan**, or **Software**), internal rendering resolution up to 8K, dynamic resolution fallback, and presentation pipeline settings.
- **Controller Configuration**:
  - Rebind individual keyboard keys and controller buttons.
  - Set per-controller stick deadzone percentage.
  - Toggle **Dual analog mode** and **Duplicate face buttons on shoulders** with an on-screen reference legend.
- **Audio & System**: Select host audio frequency, manage memory card files (Slot 1 and Slot 2), and switch disc images.

Settings are saved automatically to [game.toml](game.toml) defaults and your user profile in `settings.toml`.
