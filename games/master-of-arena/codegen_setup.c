/* Title config for psxrecomp/host/psxrecomp_codegen_host. */

#include "codegen_setup.h"

#include "psxrecomp_codegen_host.h"

static const PsxrecompCodegenHostConfig kCodegenConfig = {
    .display_name = "Armored Core: Master of Arena",
    .project_root_env = "ACMOA_PROJECT_ROOT",
    .build_dir_env = "ACMOA_BUILD_DIR",
    .force_setup_env = "ACMOA_FORCE_SETUP",
    .psxrecomp_cli_relpath = "psxrecomp/psxrecomp_cli.py",
    .seed_cfg_relpath = "games/master-of-arena/game.toml",
    .game_toml_relpath = "games/master-of-arena/game.toml",
    .gen_marker_relpath = "games/master-of-arena/generated/SLUS_010.30_dispatch.c",
    .build_dir_name = "build-release",
    .cmake_target = "psx-master-of-arena",
    .exe_basename = "ArmoredCoreMasterOfArena",
    .prepare_note =
        "Uses your legal Master of Arena discs with the local psxrecomp SDK "
        "to generate BIOS + game C, then cmake --build.",
    .prepare_note_windows =
        "Uses your legal Master of Arena discs with the local psxrecomp SDK "
        "to generate BIOS + game C, then rebuilds the Master of Arena target.",
    .prepare_note_no_cmake =
        "Uses your legal Master of Arena discs with the local psxrecomp SDK "
        "to generate BIOS + game C. Build the psx-master-of-arena target.",
};

void psx_game_codegen_setup_apply(RecompLauncherCGameInfo* gi) {
    psxrecomp_codegen_host_apply(gi, &kCodegenConfig);
}

void psx_game_codegen_relaunch_or_exit(const char* disc_path) {
    psxrecomp_codegen_host_relaunch_or_exit(disc_path);
}

void psx_game_codegen_forward_if_built(int argc, char** argv) {
    psxrecomp_codegen_host_forward_if_built(&kCodegenConfig, argc, argv);
}
