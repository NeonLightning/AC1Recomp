"""Keep launcher boot preferences in sync with overlay save state."""
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MAIN = (ROOT / "runtime" / "src" / "main.cpp").read_text(encoding="utf-8")

handoff = """fast_boot = seed.fast_boot;
                g_fast_boot = fast_boot;
                bios_hle  = seed.bios_hle;
                g_bios_hle = bios_hle;"""
assert handoff in MAIN, (
    "startup launcher settings must update both boot-plan values and the "
    "overlay values that Restart saves"
)

assert "settings.fast_boot = g_fast_boot;" in MAIN
assert "settings.bios_hle = g_bios_hle;" in MAIN
assert "seed.fast_boot = ls.fast_boot != 0;                    seed.has_fast_boot = true;" in MAIN
assert "seed.bios_hle = ls.bios_hle != 0;                      seed.has_bios_hle = true;" in MAIN
