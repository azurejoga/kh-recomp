// main.cpp — KHRecomp entry point.
//
// Every gbarecomp game binary takes BOTH a BIOS and a ROM at launch.
// The CLI accepts:
//
//   KHRecomp [--bios <path>] [--rom <path>] [game.toml]
//
// All three are optional on the command line; missing values are
// pulled from game.toml. Hashes are verified before any code runs.

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "runtime.h"

namespace {

void print_usage() {
    std::printf(
        "KHRecomp [--bios <path>] [--rom <path>] [game.toml]\n"
        "\n"
        "Both BIOS and ROM are required (either via flags or via the\n"
        "[bios] / [rom] sections of game.toml). The runtime refuses\n"
        "to start unless both hash-verify.\n"
        "\n"
        "Headless/deterministic flags: --steps <N>, --frames <N>,\n"
        "--no-window, --tcp <port>, --dump-bmp <prefix>.\n");
}

}  // namespace

int main(int argc, char** argv) {
    std::printf("KHRecomp (Kingdom Hearts: Chain of Memories static recomp)\n");

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0 ||
            std::strcmp(argv[i], "-h") == 0) {
            print_usage();
            return 0;
        }
    }

    // Built-in defaults so a standalone KHRecomp.exe ships without a
    // sibling game.toml. The runtime falls back to these values when
    // no TOML is found and no CLI override is supplied.
    gbarecomp::RunOptions opts;
    opts.builtin_game_name  = "Kingdom Hearts: Chain of Memories";
    opts.builtin_rom_sha1   = "8db73586cdb11b3795907edebf43228dbcd3e6b2";
    // CRC32 of the pinned EU ROM (same dump the SHA-1 above gates on).
    opts.builtin_rom_crc32  = 0x772D97FBu;
    opts.launcher_region    = "Europe";
    opts.launcher_game_config = "game.toml";

    return gbarecomp::run_game(argc, argv, opts);
}
