# BUILDING — Building KHRecomp on Windows

This guide describes the **exact** toolchain with which the project is built
and tested, the minimum requirements and the steps from zero to `.exe`.

---

## Toolchain (what we actually use)

- **MSYS2** with **MinGW-w64 (mingw64)** toolchain — GCC 14.x (`g++.exe`).
- **CMake** ≥ 3.20 (`MinGW Makefiles` generator).
- **SDL2** (static, from MSYS2's mingw64 — `libSDL2.a`).
- **Prism** (accessibility bridge) — **optional**, only if the prebuilt
  artifacts exist in `<repo>/prism/`. Without them, the build compiles without
  the voice bridge (the Shift+A hotkey does nothing until you provide Prism).
- **Ninja** (optional; `MinGW Makefiles` is the validated generator).

The project links **statically**: the final `.exe` embeds SDL2, libstdc++,
libgcc and libwinpthread — **zero third-party DLLs in the deliverable** (only
Windows system DLLs). This is controlled by `GBARECOMP_STATIC_MINGW` (default
ON in Release; see `gbarecomp/CMakeLists.txt`).

---

## Prerequisites

1. **MSYS2** — install from https://www.msys2.org (64-bit). In the MSYS2 shell
   (`MSYS2 MINGW64`), install:

   ```bash
   pacman -Syu --noconfirm
   pacman -S --noconfirm mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake \
              mingw-w64-x86_64-ninja mingw-w64-x86_64-SDL2 \
              mingw-w64-x86_64-pkgconf
   ```

   Alternative to the CMake package: use the Windows CMake and point
   `CMAKE_CXX_COMPILER` at `C:/msys64/mingw64/bin/g++.exe` (see below).

2. **Game ROM** — *Kingdom Hearts: Chain of Memories* (European version).
   **Not versioned** (copyright). Put your dump in `roms/khcom_eur.gba`.
   Without the ROM, the build produces the executable but without the game
   corpus (see `docs/NOTICE.md`).

3. **GBA BIOS** — 16 KiB dump. **Not versioned**. Put your dump in
   `gbarecomp/bios/gba_bios.bin`. The project default is the **recompiled real
   BIOS** (not HLE); HLE is an optional tier documented in
   `gbarecomp/ENHANCEMENTS.md`.

4. **`prism.dll`** — already versioned and **copied automatically** next to the
   executable during the build (post-build step in `CMakeLists.txt`). Nothing
   to do manually.

> **Note on the `generated/` corpus:** it is versioned in the repo and the
> build uses it directly. If you remove it, regenerate with
> `gba_recompile --rom roms/khcom_eur.gba --config game_static.toml --out generated`.

---

## Steps

From the **repository root** (`KHRecomp/`), in the MSYS2 MINGW64 shell:

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --target KHRecomp -j$(nproc)
```

If the Windows CMake is used (outside MSYS2), tell it the compiler:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release `
      -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/g++.exe
cmake --build build --target KHRecomp
```

The final artifact is `build/KHRecomp.exe` (~30 MB, static).

### Run (play the game)

The build **assembles the playable package by itself** next to the exe: it
copies the `prism.dll`, the self-contained `game.toml` (`game.build.toml`) and
the save `khcom_eur.sav`. All that's left is for you to provide the **ROM** and
the **BIOS** (not versioned for copyright reasons), in the names expected by
`game.toml`:

```powershell
# from the root, after build (copy the ROM and BIOS next to the exe):
copy roms\khcom_eur.gba build\khcom_eur.gba
copy gbarecomp\bios\gba_bios.bin build\gba_bios.bin
cd build
.\KHRecomp.exe            # opens the game (uses local game.toml)
```

Or run with explicit paths from anywhere:

```powershell
.\KHRecomp.exe --rom ".\roms\khcom_eur.gba" --bios ".\gbarecomp\bios\gba_bios.bin"
```

---

## Recompiling the corpus (generating `generated/`)

The `generated/` directory contains the recompiled C++ code of your ROM. The
full recompilation flow (using the Ghidra analyzer and the `gbarecomp`
generator) is documented in `gbarecomp/README.md` and `gbarecomp/tools/`:

1. Import the ROM into Ghidra as *Raw Binary* at `0x08000000`,
   ARM:LE:32:v4T.
2. Run the headless script `tools/export_functions.py` to produce the symbol
   TSV (`addr<TAB>mode<TAB>name`).
3. Run the generator (`gba_recompile`) with `--symbols <your.tsv>` to produce
   the shards `recompiled_*.cpp` + `dispatch_table.cpp` + `symbol_map.cpp`.
4. Validate the hashes: the expected ROM is registered in `game_static.toml`
   and the runtime aborts on hash mismatch before executing any code.

**Important:** the `generated/` in this checkout is **versioned** and is the
corpus used by the build; if you regenerate it, do so from **your own ROM** to
keep the hashes consistent (see `docs/NOTICE.md`).

---

## Relevant CMake options

| Option | Default | Effect |
|---|---|---|
| `-DCMAKE_BUILD_TYPE=Release` | — | Optimization + static link |
| `-DGBARECOMP_STATIC_MINGW=ON` | ON (Release) | Embeds SDL2 + C++ runtime in the exe |
| `-DGBARECOMP_SDL2_OK` | auto | Detects SDL2 in mingw64 |
| `-DGBARECOMP_MINGW_PREFIX_UNIX` | `/c/msys64/mingw64` | mingw64 root (SDL2/include) |
| `-DGBARECOMP_SELFHEAL_MC_DIR` | off | Self-heal shared with MinishCapRecomp (opt-in) |

The Prism bridge is **automatic**: CMake detects `<repo>/prism/include/prism.h`
and links `libprism.a` (Windows) or `prism.lib` (MSVC). Without the directory,
the build proceeds without it (the runtime compiles with a no-op).

---

## Common troubleshooting

- **`fatal error: SDL.h: No such file`** — install `mingw-w64-x86_64-SDL2` in
  MSYS2 and confirm `GBARECOMP_MINGW_PREFIX_UNIX`.
- **`cannot find -lstdc++` / wrong toolchain** — make sure the `g++` at the
  front of PATH is the mingw64 one (`C:/msys64/mingw64/bin`), not the old
  (32-bit) MinGW.
- **Divergent corpus (hash mismatch at boot)** — the runtime validates the ROM
  against the hash expected in `game_static.toml`. Use the exact cartridge
  version that generated your `generated/`.
- **Shift+A hotkey says nothing** — Prism is not present. Put the Prism
  release artifacts in `<repo>/prism/` (`include/prism.h`,
  `lib/libprism.a`) and recompile; or use NVDA/SAPI configured in Prism.
- **Slow build** — the corpus has 16 shards and ~26k functions; `-j$(nproc)`
  and an NVMe disk help. The first `generated/` compilation takes minutes.

---

## Tests

See `docs/TESTING.md` for the headless validation matrix (soaks, self-heal,
audio, PPU) and how to reproduce each test with the diagnostic env vars.