# UI Assets Specification

This document details the asset names, search paths, supported image formats, and dimension constraints (minimum / maximum sizes) for custom icons, banners, and cursors used by the **Armored Core** Launcher and In-Game Settings Overlay.

---

## Asset Storage Locations

The runtime searches for custom UI assets in any of the following directories (relative to the executable or project root):

1. `assets/ui/` (Recommended dedicated location)
2. `assets/img/`
3. `assets/`

---

## Image Formats

- **PNG**: 32-bit RGBA (with alpha transparency). Preferred format.
- **TGA**: 32-bit uncompressed RGBA or 24-bit RGB with top-left color-key transparency.

All images are dynamically resampled with aspect-preserving nearest-neighbor or linear scaling, so retro PS1 pixel art and modern high-resolution sprites both render cleanly across any window scale and display resolution.

---

## In-Game Overlay Assets

| Identifier / Filename | Description | Min Size | Max Size | Recommended Size | Notes |
|---|---|---|---|---|---|
| `cursor.png` (or `.tga`) | Active selection cursor / chevron in the menu rows | 8×8 px | 128×128 px | 16×16 to 32×32 px | Alpha-masked pointer; drawn to the left of the selected row. Automatically centered vertically. |
| `panel_header.png` | Banner / insignia displayed above the title in the overlay header | 32×8 px | 1024×256 px | 240×32 to 480×64 px | Displayed at the top of the overlay card; width clamps to card width while preserving aspect. |
| `icon_display.png` | Icon for the **Display** menu section | 8×8 px | 128×128 px | 16×16 to 64×64 px | Represents display / monitor / aspect settings. |
| `icon_graphics.png` | Icon for the **Graphics** menu section | 8×8 px | 128×128 px | 16×16 to 64×64 px | Represents rendering, PGXP, and filter settings. |
| `icon_audio.png` | Icon for the **Audio** menu section | 8×8 px | 128×128 px | 16×16 to 64×64 px | Represents volume and sound output. |
| `icon_input.png` | Icon for the **Input** menu section | 8×8 px | 128×128 px | 16×16 to 64×64 px | Represents gamepad / twin-stick controls. (Fallback: `icon_controller.png`) |
| `icon_system.png` | Icon for the **System** menu section | 8×8 px | 128×128 px | 16×16 to 64×64 px | Represents core machine, BIOS, and launch actions. |
| `icon_saves.png` | Icon for the **Saves** / memory card menu section | 8×8 px | 128×128 px | 16×16 to 64×64 px | Represents save states and memory card slots. |

*Note: If any icon or cursor file is not present, the overlay automatically renders a built-in Armored Core-themed procedural tactical glyph in its place.*

---

## Desktop Launcher Assets

| Identifier / Filename | Description | Min Size | Max Size | Recommended Size | Notes |
|---|---|---|---|---|---|
| `icon_display.png` | Category header icon for Display | 8×8 px | 128×128 px | 24×24 to 64×64 px | Scaled to section header height. |
| `icon_graphics.png` | Category header icon for Graphics | 8×8 px | 128×128 px | 24×24 to 64×64 px | Scaled to section header height. |
| `icon_audio.png` | Category header icon for Audio | 8×8 px | 128×128 px | 24×24 to 64×64 px | Scaled to section header height. |
| `icon_input.png` | Category header icon for Input / Controllers | 8×8 px | 128×128 px | 24×24 to 64×64 px | Scaled to section header height. |
| `icon_system.png` | Category header icon for System / BIOS | 8×8 px | 128×128 px | 24×24 to 64×64 px | Scaled to section header height. |
| `icon_saves.png` | Category header icon for Saves / Memory Cards | 8×8 px | 128×128 px | 24×24 to 64×64 px | Scaled to section header height. |
| `icon_hotkeys.png` | Category header icon for Hotkeys | 8×8 px | 128×128 px | 24×24 to 64×64 px | Scaled to section header height. |
| `boxart.tga` (or `.png`) | Game cover artwork on dashboard | 64×64 px | 2048×2048 px | 512×512 px | Displayed centered on the dashboard. |
| `brand_psx.tga` (or `.png`) | Platform emblem in top-left header | 16×16 px | 256×256 px | 44×33 or 48×48 px | Scaled proportionally to header height. |
| `memcard.tga` (or `.png`) | Memory card graphical icon | 16×16 px | 256×256 px | 64×64 px | Used on the dashboard save card. |
