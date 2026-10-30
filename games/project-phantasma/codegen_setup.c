/* Title config for psxrecomp/host/psxrecomp_codegen_host. */

#include "codegen_setup.h"

#include "psxrecomp_codegen_host.h"

static const PsxrecompCodegenHostConfig kCodegenConfig = {
    .display_name = "Armored Core: Project Phantasma",
    .project_root_env = "ACPP_PROJECT_ROOT",
    .build_dir_env = "ACPP_BUILD_DIR",
    .force_setup_env = "ACPP_FORCE_SETUP",
    .psxrecomp_cli_relpath = "psxrecomp/psxrecomp_cli.py",
    .seed_cfg_relpath = "games/project-phantasma/game.toml",
    .game_toml_relpath = "games/project-phantasma/game.toml",
    .gen_marker_relpath = "games/project-phantasma/generated/SLUS_006.70_dispatch.c",
    .build_dir_name = "build-release",
    .cmake_target = "psx-project-phantasma",
    .exe_basename = "ArmoredCoreProjectPhantasma",
    .prepare_note =
        "Uses your legal Project Phantasma disc with the local psxrecomp SDK "
        "to generate BIOS + game C, then cmake --build.",
    .prepare_note_windows =
        "Uses your legal Project Phantasma disc with the local psxrecomp SDK "
        "to generate BIOS + game C, then rebuilds the Project Phantasma target.",
    .prepare_note_no_cmake =
        "Uses your legal Project Phantasma disc with the local psxrecomp SDK "
        "to generate BIOS + game C. Build the psx-project-phantasma target.",
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
