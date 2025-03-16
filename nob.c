#define NOB_IMPLEMENTATION
#include "nob.h"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra");
    nob_cmd_append(&cmd, "-I./raylib-5.5_macos/include");
    nob_cmd_append(&cmd, "-o", "main", "main.c");
    nob_cmd_append(&cmd, "-rpath", "@executable_path/raylib-5.5_macos/lib");
    nob_cmd_append(&cmd, "-L./raylib-5.5_macos/lib", "-lraylib");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    return 0;
}