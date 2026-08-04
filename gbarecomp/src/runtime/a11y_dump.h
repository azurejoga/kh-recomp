// a11y_dump.h - diagnostics for the Fase-2 text reader. When
// GBARECOMP_A11Y_DUMP is set to a path, each presented frame that also
// crosses a dump-frame threshold journals the GBA video registers, every
// text-mode background tilemap, and a small round-trip so we can learn which
// background carries KH CoM's dialog/menu glyphs and what encoding it uses.
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace gbarecomp {

struct A11yDumpState {
    FILE* out = nullptr;
    bool on = false;
    int countdown = 0;
};

inline A11yDumpState& a11y_dump_state() {
    static A11yDumpState s;
    return s;
}

// Open the dump log once. Called by the runtime. Frame numbers we snapshot
// (relative to a11y enabling) can be widened after inspecting the first one.
inline void a11y_dump_start() {
    A11yDumpState& s = a11y_dump_state();
    const char* path = std::getenv("GBARECOMP_A11Y_DUMP");
    if (!path || !*path) return;
    s.out = std::fopen(path, "wb");
    s.on = s.out != nullptr;
    if (s.on) std::fprintf(s.out, "a11y_dump begin\n");
}

// Called every presented frame. `io` = 0x04000000 page, `vram` = 96KB.
inline void a11y_dump_frame(const uint8_t* io, const uint8_t* vram) {
    A11yDumpState& s = a11y_dump_state();
    if (!s.on) return;

    // Snapshot every ~30 frames (1/sec) so menu transitions are observable.
    if (++s.countdown < 30) return;
    s.countdown = 0;

    auto rd16 = [&](uint32_t off) -> uint16_t {
        return static_cast<uint16_t>(io[off]) |
               (static_cast<uint16_t>(io[off + 1]) << 8);
    };
    const uint16_t disp = rd16(0x000);
    const uint16_t bg0 = rd16(0x008);
    const uint16_t bg1 = rd16(0x00A);
    const uint16_t bg2 = rd16(0x00C);
    const uint16_t bg3 = rd16(0x00E);
    std::fprintf(s.out,
                 "\n--- frame#64 DISPCNT=0x%04X bg0=0x%04X bg1=0x%04X "
                 "bg2=0x%04X bg3=0x%04X\n",
                 disp, bg0, bg1, bg2, bg3);

    // For each enabled text-mode BG (mode 0/1/2), print its first screenblock
    // as a raw hex map so we can spot glyph regions for the dialog box.
    for (int bg = 0; bg < 4; bg++) {
        const uint16_t cnt = (bg == 0) ? bg0 : (bg == 1) ? bg1 : (bg == 2) ? bg2 : bg3;
        if ((cnt & 0x0080u) == 0) continue;  // BG enable bit
        const int mode = disp & 0x07;
        if (mode >= 3) break;  // bitmap modes
        const int char_base = (cnt >> 2) & 0x03;      // tileset block
        const int map_base  = ((cnt >> 8) & 0x1F) * 0x800;  // tilemap block
        const int size      = (cnt >> 14) & 0x03;     // 0=256x256 ...
        std::fprintf(s.out, "BG%d map_base=0x%04X char=0x%04X size=%d\n",
                     bg, map_base, char_base * 0x4000, size);
        // Screen 0 (top-left 256x256): 32x32 entries.
        for (int ty = 0; ty < 32; ty++) {
            for (int tx = 0; tx < 32; tx++) {
                int idx = ty * 32 + tx;
                uint16_t e = static_cast<uint16_t>(vram[map_base + idx * 2]) |
                             (static_cast<uint16_t>(vram[map_base + idx * 2 + 1])
                              << 8);
                std::fprintf(s.out, "%04X ", e & 0x3FF);
            }
            std::fprintf(s.out, "\n");
        }
    }
    std::fflush(s.out);
}

}  // namespace gbare