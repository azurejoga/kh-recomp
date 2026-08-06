# KHRecomp — Kingdom Hearts: Chain of Memories (GBA) → native PC

Static recompilation of *Kingdom Hearts: Chain of Memories* (Game Boy Advance,
European version) into a native Windows program. This is **not** an emulator:
the ROM code (ARM7TDMI / ARMv4T) is translated to C++ and compiled into real
x86-64 machine code. Game functions are native functions in the executable —
there is no interpreter on the hot path.

> Built and tested on **Windows with MSYS2 / MinGW-w64** (`C:\msys64\mingw64`).

---

## Table of contents

- [Overview](#overview)
- [Is it 100% native?](#is-it-100-native)
- [What works / what does not](#what-works--what-does-not)
- [Repository layout](#repository-layout)
- [What is versioned (and what is not)](#what-is-versioned-and-what-is-not)
- [Requirements](#requirements)
- [Building (MSYS2 / MinGW-w64)](#building-msys2--mingw-w64)
- [Running the game](#running-the-game)
- [Testing and validation](#testing-and-validation)
- [Development](#development)
- [Accessibility (Prism)](#accessibility-prism)
- [Known issues and limitations](#known-issues-and-limitations)
- [Notice — ROM, BIOS, trademarks](#notice--rom-bios-trademarks)
- [License and credits](#license-and-credits)

---

## Overview

The pipeline looks like this:

```
ROM (ARMv4T bits)  ──analyzer──▶  symbols (addr, mode, name)
                   ──gba_recompile─▶  generated/*.cpp  (C++ corpus)
                          │
                          ▼
                 CMake compiles the corpus + runtime (MSYS2/MinGW-w64)
                          │
                          ▼
                KHRecomp.exe  (native x86-64, static)
                          │
                          ▼
              dispatch → runs native functions + self-heal for RAM code
```

- **`gbarecomp/`** — the game-agnostic core: GBA static C++ recompiler
  (ARMv4T decoder, code generator, bus, PPU, audio, DMA, timers, BIOS). Fork of
  [`mstan/gbarecomp`](https://github.com/mstan/gbarecomp), derived from
  [`JRickey/gba-recomp`](https://github.com/JRickey/gba-recomp).
- **`generated/`** — the "corpus": the KH CoM ROM already recompiled into C++
  (~26,160 functions, ~126 MB, 16 shards). Versioned in the repo.
- **`src/runtime/`** — the runtime: dispatch, SDL2 window, WASAPI audio, save
  and save-states, TCP debug server, on-the-fly native overlays, and the
  screen-reader bridge (accessibility).

## Is it 100% native?

**Yes — 100% of the ROM code is recompiled and runs natively. Proven by tests,
not by assertion.** The only exception to a literal `STATIC` label is the
dynamic RAM code the game generates at runtime — physically impossible to
precompile in *any* GBA recompiler (FireRedLeafGreenRecomp and EmeraldRecomp
handle it the same way).

**Proof by test (`GBARECOMP_STRICT_STATIC=1`):** the runtime aborts instead of
self-healing on the first unmapped PC. The only observed abort is
`pc=0x02038D48` — **EWRAM** (dynamic game code). **No ROM address (`0x08...`)
ever aborts**: the entire ROM is covered by AOT precompilation.

**Real numbers (20,000-frame warm soak):**

| Metric | Value |
|---|---|
| native calls (`native_calls`) | 788,411 |
| interpreted instructions | 4,842 (0.6%) |
| dispatch misses | 13 — **all RAM, none ROM** |
| on-the-fly recompiled functions (`healed_native`) | 3,377 |
| heal failures | 0 |
| exit code | 0 |

**Why the label stays `NOT_STATIC`:** the runtime refuses to call a session
`STATIC` if *any* PC was interpreted or served from cache (honesty is
structural — see `gbarecomp/PRINCIPLES.md`). The 0.6% is a one-time discovery
bridge; after it, the code runs as real native x86-64. This is the maximum
state achievable by any GBA recompiler.

## What works / what does not

**Works (validated headless and windowed):**

- Full boot: recompiled BIOS → intro → ROM, all the way to gameplay.
- Rendering: D3D11 window, vsync, nearest scaler (tested 1366×768@60 Hz).
- Audio: WASAPI 65,536 Hz — **mono S16** (known limitation, see below).
- Save: Flash 1M (128 KB) chip with the KH CoM driver (MX29L010).
- Save-states: F1..F12 (plain key = load, Shift = save).
- Input: A=X, B=Z, L=C, R=V, Start=Return, Select=RShift, D-pad=arrows.
  Remappable via `keybinds.ini`. Hotkeys: `Alt+Return` fullscreen,
  `Shift+P` pause, `Tab` turbo, `F` perf overlay, `Shift+A` accessibility.
- Soaks: 6,000–30,000 frames without crash, `failed=0`, `exit=0`.
- Accessibility hotkey **Shift+A**: speaks "Accessibility mode on/off" via NVDA.

**Does not work / honest limitations:**

- **Mono audio** (no stereo mixing in the native path) — accepted limitation.
- **BIOS chime timing** slightly faster than hardware (imperceptible).
- **Accessibility Phase 2** (reading menu/dialogue text) is **not
  implemented** — only the hotkey + spoken confirmation exist.
- Closing the window may print `terminate called without an active exception`
  (benign, on close only).

## Repository layout

| Path | Role |
|---|---|
| `CMakeLists.txt` | KHRecomp build (corpus + runtime) |
| `src/main.cpp` | Entry point, arg parsing, ROM/BIOS load, main loop |
| `game.toml` | Game config (ROM, save chip, entry point) |
| `game.build.toml` | Self-contained config copied next to the exe (clone/package) |
| `game_static.toml` | Static corpus: entry, `extra_func` (3,275), boundaries |
| `config/eur.toml` | Europe region overrides (sha1/crc32, flash1m save) |
| `generated/` | AOT corpus (versioned; derived artifact of your ROM) |
| `src/runtime/` | Runtime: dispatch, window, audio, save-states, self-heal, Prism |
| `gbarecomp/` | Game-agnostic core (mstan fork) + its own docs |
| `prism/` | Accessibility bridge (prebuilt Prism artifacts, optional) |

## What is versioned (and what is not)

**Versioned:** source, `generated/` corpus, save (`roms/khcom_eur.sav`),
configs (all TOMLs), the docs and the **`prism.dll`** (copied automatically
next to the exe at build time).

**Not versioned (copyright reasons):**

- The game **ROM** (`roms/khcom_eur.gba`) — you provide your own dump.
- The **GBA BIOS** (`gbarecomp/bios/gba_bios.bin`) — you provide your own dump.
- The runtime **validates hashes** before executing anything: ROM SHA-1
  `8db73586cdb11b3795907edebf43228dbcd3e6b2`, CRC32 `0x772D97FB` (EU version);
  BIOS is validated too.

If you remove `generated/`, the build regenerates it with
`gba_recompile --rom roms/khcom_eur.gba --config game_static.toml --out generated`.

## Requirements

1. **MSYS2** — install from https://www.msys2.org (64-bit). In the **MSYS2
   MINGW64** shell, install the toolchain and SDL2:

   ```bash
   pacman -Syu --noconfirm
   pacman -S --noconfirm mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake \
              mingw-w64-x86_64-ninja mingw-w64-x86_64-SDL2 \
              mingw-w64-x86_64-pkgconf
   ```

   (Alternative: use the Windows CMake and point
   `-DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/g++.exe`.)

2. **Game ROM** — *Kingdom Hearts: Chain of Memories* (European version), your
   own dump, at `roms/khcom_eur.gba`.
3. **GBA BIOS** — 16 KiB dump, your own, at `gbarecomp/bios/gba_bios.bin`.
4. **`prism.dll`** — already versioned and copied automatically. Nothing to do.

> **⚠ BIOS recompile — required before building (the #1 build gotcha).**
> The **recompiled BIOS is not versioned** (copyright: it embeds Nintendo's
> BIOS bytes as instruction-by-instruction lowerings). The build detects its
> absence and compiles anyway — with a **placeholder stub** — printing
> `BIOS recompiled output absent — placeholder dispatch only`. The resulting
> exe boots via HLE/interpreted BIOS: the intro is slow and stutters.
> **To get the real LLE BIOS (native intro, no stutter), generate it once
> before the build**, from the repo root:
>
> ```bash
> cmake --build build --target gba_recompile -j$(nproc)
> ./build/gbarecomp_build/gba_recompile.exe --bios gbarecomp/bios/gba_bios.bin
> ```
>
> This writes
> `gbarecomp/src/runtime/generated_bios/bios_recompiled.cpp` (+ `.h` and
> `bios_dispatch_table.cpp`). Re-run `cmake` after generating. If the file is
> present, the build log shows `bios_backend=LLE (recompiled BIOS)` at runtime
> instead of the placeholder warning.

The final `.exe` is **static**: it embeds SDL2, libstdc++, libgcc and
libwinpthread — zero third-party DLLs in the deliverable (only Windows system
DLLs, plus `prism.dll`).

## Building (MSYS2 / MinGW-w64)

From the **repository root**, in the MSYS2 MINGW64 shell:

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --target KHRecomp -j$(nproc)
```

The final artifact is `build/KHRecomp.exe` (~30 MB, static).

The build **assembles the playable package by itself** next to the exe: it
copies `prism.dll`, the self-contained `game.toml` (`game.build.toml`) and the
save `khcom_eur.sav`. All that remains is to drop the ROM and BIOS next to the
exe with the expected names.

**Common build problems:**

- `fatal error: SDL.h: No such file` — install `mingw-w64-x86_64-SDL2` in MSYS2.
- `cannot find -lstdc++` — make sure the `g++` at the front of PATH is the
  mingw64 one (`C:/msys64/mingw64/bin`), not an old 32-bit MinGW.
- **Hash mismatch at boot** — you need the exact European cartridge version
  that generated this `generated/`.
- **Slow build** — the corpus has 16 shards and ~26k functions; `-j$(nproc)`
  and an NVMe help.

## Running the game

```powershell
# from the repo root, after build (copy the ROM and BIOS next to the exe):
copy roms\khcom_eur.gba build\khcom_eur.gba
copy gbarecomp\bios\gba_bios.bin build\gba_bios.bin
cd build
.\KHRecomp.exe            # opens the game (uses the local game.toml)
```

Or with explicit paths from anywhere:

```powershell
.\KHRecomp.exe --rom ".\roms\khcom_eur.gba" --bios ".\gbarecomp\bios\gba_bios.bin"
```

The runtime validates the ROM hashes (SHA-1 `8db73586...`, CRC32
`0x772D97FB`) before executing any code and aborts on mismatch.

**Controls (default):** A=X, B=Z, L=C, R=V, Start=Return, Select=RShift,
D-pad=arrow keys. Remappable via `keybinds.ini`. Hotkeys: `Alt+Return`
fullscreen, `Shift+P` pause, `Tab` turbo, `F` perf overlay, `Shift+A`
accessibility. Save-states: `F1..F12` (plain = load, Shift = save).

## Testing and validation

### Headless / deterministic flags

```bash
KHRecomp.exe --rom "...\khcom_eur.gba" --bios "...\gba_bios.bin" \
             --no-window --frames 20000 [--tcp 0] [--dump-bmp out]
```

- `--frames <N>` — runs N PPU frames and exits.
- `--steps <N>` — runs N CPU steps (alternative to frames).
- `--no-window` — no window (CI / soaks).
- `--tcp <port>` — TCP debug server (`0` = off).
- `--dump-bmp <prefix>` — writes frames as BMP (pixel-by-pixel comparison).

### Diagnostic env vars

| Env var | Effect |
|---|---|
| `GBARECOMP_BIOS_HLE` | optional HLE BIOS (instead of recompiled LLE) |
| `GBARECOMP_SELFHEAL_VERBOSE` | detailed log of every heal/cache |
| `GBARECOMP_SELFHEAL_RECOMPILE` | force on-the-fly recompile instead of cache |
| `GBARECOMP_STRICT_STATIC` | aborts on any unmapped PC — the "purity" test |
| `GBARECOMP_ENABLE_MODS` | enables module overlays (off by default) |
| `GBARECOMP_NO_LAUNCHER` | skips the launcher |
| `GBARECOMP_DEFAULT_GAME_CONFIG` | points to a specific `game.toml` |
| `GBARECOMP_SCREEN` / `GBARECOMP_SOLAR` | screen profile / solar filters |
| `GBARECOMP_A11Y_DUMP=<path>` | VRAM snapshots → discover text tiles (Phase 2) |
| `GBARECOMP_SAVE_TYPE` | save chip type override (e.g. `flash1m`) |
| `GBARECOMP_FP_SAVE` | fingerprint-ring save dump to a file |

### Validation matrix (soaks)

Run with `--no-window --frames <N>` and inspect the `final_pc`, `ppu_frames`
and `self_heal_coverage` lines in the log. Expected values of the final build:

| Test | Target frames | Expectation |
|---|---|---|
| Cold boot | up to screen | `exit=0`, recompiled BIOS runs to the ROM |
| Headless soak | 6,000–30,000 | `failed=0`, `final_pc` = thread idle, no crash |
| Combat prewarm | 30,000 | 3,651 heals, `failed=0` — AIs/effects |
| RAM heal | 3,000 | `failed=0` |
| Warm cache | 20,000 | `failed=0` |
| Boot frame | 1 | `unmapped=0 io_unhandled=0` |

Real example (20,000-frame warm soak):

```
summary: final_pc=0x00000348 unmapped=0 io_unhandled=0 steps=59713
  cycles=5347265721 ppu_vcount=168 ppu_frames=20000 frames_presented=20000
self_heal_coverage=NOT_STATIC dispatch_misses=13 interpreted_insns=4842
  healed_native=3257 native_calls=1222334 inflight=0 failed=0
```

### How to read the self-heal line (honestly)

- `self_heal_coverage=NOT_STATIC` — reported when **any** PC was interpreted
  or served from cache. By design, not a defect.
- `dispatch_misses` — new functions found at runtime.
- `healed_native` — recompiled on the fly, now running natively.
- `interpreted_insns` — instructions through the discovery bridge (tiny
  fraction in a hot soak: 4,842 / ~1.22M = 0.4%).
- `failed=0` — no heal failed. Values 3–8 appear in partial cache (cold
  client) and are benign — the runtime re-heals.

For a strict purity check (must fail if anything is interpreted):
`GBARECOMP_STRICT_STATIC=1`.

## Development

### How the port works inside

1. **Function extraction** — the ROM is loaded in Ghidra as Raw Binary at
   `0x08000000` (`ARM:LE:32:v4T`). A headless script
   (`gbarecomp/tools/export_functions.py`) exports `addr<TAB>mode<TAB>name` TSV
   → `symbols/`.
2. **Corpus generation** — `gba_recompile` reads the TSV + ROM and produces
   `generated/`: 16 shards `recompiled_*.cpp` (~26,137 functions), plus
   `dispatch_table.cpp` (PC→native table) and `symbol_map.cpp` (names).
   Fine control (entry, boundaries, `extra_func`, jump tables) lives in
   `game_static.toml` (schema: `gbarecomp/docs/TOML_SCHEMA.md`).
3. **Compilation** — the corpus is real C++ compiled with `src/main.cpp` and
   the `gbarecomp/` core. Game functions become native x86-64 functions; a
   game call is a real `call` into the binary.
4. **Runtime / self-heal** — the runtime resolves what the analyzer could not
   anticipate: when the PC reaches an unknown region (dynamic RAM code the
   game writes at runtime), it (1) interprets that function one run,
   (2) recompiles it to native on the fly, (3) caches it to disk, and
   (4) runs it natively from then on. Marked in the coverage JSON as
   `healed_native`.

### Rules for contributors

- Respect `gbarecomp/CLAUDE.md`: **never edit `generated/`** — fixes go in the
  runtime or the config (`game.toml` / `game_static.toml`).
- Do not auto-write derived functions into `game.toml` — proposals
  (`recomp_seed_proposals.toml` / `recomp_master_misses_*.toml.frag`) are
  human-reviewed and merged.
- After every run, check the coverage report and the miss-list; each miss is a
  discovery gap to close.
- Honesty is load-bearing: never call a build "static / done" while PCs were
  interpreted or healed from cache without reporting it.
- Core internal docs: `gbarecomp/docs/ARCHITECTURE.md`,
  `gbarecomp/docs/TOML_SCHEMA.md`, `gbarecomp/docs/ROADMAP.md`,
  `gbarecomp/PRINCIPLES.md`.

### Recompiling the corpus (generating `generated/`)

1. Import the ROM into Ghidra as Raw Binary at `0x08000000`, `ARM:LE:32:v4T`.
2. Run the headless script `tools/export_functions.py` → symbol TSV.
3. Run the generator: `gba_recompile --rom roms/khcom_eur.gba --config
   game_static.toml --out generated`.
4. Validate: the runtime aborts on hash mismatch before executing any code.

The `generated/` in this checkout is **versioned** and is the corpus used by
the build; if you regenerate it, do so from **your own ROM** to keep hashes
consistent.

## Accessibility (Prism)

The long-term goal is making the game playable by screen readers (NVDA, etc.),
inspired by Pokémon Access. Current state:

- **Phase 1 (shipped, validated):** holding **Shift + A** toggles
  accessibility mode. The hook uses `GetAsyncKeyState`, so it works even
  when the window is unfocused. It speaks "Accessibility mode on/off" via
  NVDA (Windows). Build is opt-in: it compiles only when the prebuilt Prism
  artifacts are in `<repo>/prism/` (CMake detects `prism/include/prism.h`).
- **Phase 2 (open, not started):** automatically reading the game's text
  (dialogs, menus, card names) — decode the VRAM text tilemap via
  `bus.vram_ptr()` + the KH CoM tile→character table, detect changes between
  frames, and speak the extracted string. `GBARECOMP_A11Y_DUMP=<path>` is the
  discovery tool (writes video registers + text tilemaps per frame).

Without Prism, the game builds and runs normally, just without speech.

## Known issues and limitations

| ID | Issue | Status |
|---|---|---|
| LP-001 | Audio delivered in **mono S16** (not stereo) | Accepted limitation |
| LP-002 | BIOS chime timing slightly accelerated | Accepted, imperceptible |
| KN-001 | `terminate called without an active exception` on window close | Known, benign |
| KN-002 | Call-return stack overflow (rare, early cold soaks) | Mitigated, not reproduced |
| KN-003 | Busy-spin hang class (MC-HP-002 legacy) | Not reproduced in current build |
| LOSGEND | Coverage reported `NOT_STATIC` | **Feature** — honesty by design |
| HE-001 | Heal failures `failed=3–8` in partial cache | Benign, re-heals |
| A11-001 | Shift+A speaks accessibility on/off | **Done** |
| A11-002 | Automatic text reader (Phase 2) | **Not implemented** — main open goal |

**Honest roadmap to "100% static native":** the ROM is already 100% AOT
(proven by `STRICT_STATIC`; 3,275 `extra_func` in `game_static.toml`; zero ROM
misses). Remaining work: keep monitoring the dynamic RAM misses (13 per soak),
move stable handlers to AOT when possible, and instrument `STRICT_STATIC` in
CI to prove with each release that no ROM PC is bridged. Game-generated
dynamic RAM code can never be literal static AOT — that is inherent to the
GBA, not a limitation of this project.

## Notice — ROM, BIOS, trademarks

**Read this before distributing.** This repository versions source, the
`generated/` corpus, save, configs and `prism.dll`. **The game ROM and the GBA
BIOS are deliberately left out** for copyright reasons — you provide your own
dumps in the paths the runtime expects.

- Cartridge ROMs and console BIOSes are **copyrighted material** (Nintendo /
  Square Enix / Disney), even when recoded or re-derived. Their inclusion here
  does **not** grant any right of redistribution. You are responsible for the
  legality of publishing in your context.
- Back up **your own cartridges**. Using dumps obtained outside your personal
  copy may violate copyright in your jurisdiction.
- This project is **not affiliated with, endorsed or sponsored by** Nintendo,
  Square Enix, Disney, or any rights holder of *Kingdom Hearts: Chain of
  Memories*.
- The software is provided for **research, education and software
  interoperability** with systems the user legitimately owns.
- This repository contains **no credentials or secrets**. If something like
  that is found in history, treat it as compromised and revoke it immediately.

## License and credits

MIT (see `LICENSE`). The recompilation core derives from third-party
open-source projects:

| Project | Origin | License |
|---|---|---|
| `JRickey/gba-recomp` | https://github.com/JRickey/gba-recomp | MIT OR Apache-2.0 |
| `mstan/gbarecomp` | https://github.com/mstan/gbarecomp | MIT |
| `mgba-emu/mgba` (partial, vendored) | https://github.com/mgba-emu/mgba | MPL-2.0 |
| `ethindp/prism` (accessibility, opt-in) | https://github.com/ethindp/prism | MPL-2.0 |

File-by-file attribution: `gbarecomp/THIRD_PARTY_ATTRIBUTION.md`.

---

*Ideology: recompiling is honest. `NOT_STATIC` is always disclosed. No build is
called "complete" while a PC was interpreted without record. No ROM or BIOS
will ever be embedded or distributed.*
