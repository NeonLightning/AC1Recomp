#include "recomp_launcher.h"
#include "consoles/psx/psx_profile.h"

#include <assert.h>
#include <string.h>

int main(void) {
    RecompLauncherCGameInfo game;
    memset(&game, 0, sizeof(game));

    launcher_profile_apply_psx(&game);

    /* Widescreen is title-owned; interpolation and Skip FMVs are generic rows. */
    assert(game.widescreen_supported == 0);
    assert(game.aspect_mask == 0);
    assert(game.has_frame_interp == 1);
    assert(game.has_skip_fmv == 1);
    assert(game.has_fast_boot == 1);
    assert(game.has_bios_hle == 1);
    assert(game.has_state_slot == 1);

    /* Unrelated PSX Display capabilities remain available. */
    assert(game.has_renderer == 1);
    assert(game.has_supersampling == 1);
    assert(game.has_screen_kind == 1);
    assert(game.has_vsync == 1);

    return 0;
}
