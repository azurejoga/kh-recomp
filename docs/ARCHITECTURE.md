# ARCHITECTURE — how this port works inside

This document describes, for those who want to understand or contribute, how
KHRecomp turns the GBA ROM into native code and how the runtime executes it.
It is the recommended read after `docs/README.md` and before touching any
source.

## High-level view

```
ROM (ARMv4T bits)  ──ghidra/analyzer──▶  symbols (addr, mode, name)
                   ──gba_recompile──────▶  generated/*.cpp  (C++ corpus)
                         │
                         ▼
                CMake compiles the corpus + runtime
                         │
                         ▼
               KHRecomp.exe  (native x86-64, static)
                         │
                         ▼
             dispatch → runs native functions + self-heal for RAM code
```

### 1. Function extraction (analyzer)
The starting point is the ROM loaded in Ghidra as *Raw Binary* at
`0x08000000` (`ARM:LE:32:v4T`). A headless script
(`gbarecomp/tools/export_functions.py`) enumerates the functions and exports a
TSV `addr<TAB>mode<TAB>name` that becomes `symbols/`.

### 2. Corpus generation (gba_recompile)
The generator reads the TSV + the ROM and produces, in `generated/`:
- `recompiled_*.cpp` — 16 shards with ~26,137 functions translated from
  ARM/Thumb to C++ (RSP style, expressions like `ASM(...)`,
  `LCD_IF |= ...`).
- `dispatch_table.cpp` — the PC→native-address table.
- `symbol_map.cpp` — the symbol map used by debug/backtrace and by the
  runtime to name PCs.

The fine control (entry, boundaries, `extra_func`, jump tables) is
`game_static.toml` (see `gbarecomp/docs/TOML_SCHEMA.md`).

### 3. Compilation
The corpus is real C++ code: it is compiled together with `src/main.cpp` and
the `gbarecomp/` core. The result is an `.exe` where the game's functions
**are native x86-64 functions**. There is no VM interpreting on the hot path;
calling a game function is a real `call` into the binary.

### 4. Runtime (execution and self-heal)
The runtime resolves at runtime what the analyzer could not anticipate:

- **Native dispatch:** most of the code is AOT (already compiled into the
  exe). Dispatch is a PC→function table; no instruction-by-instruction
  interpretation.
- **Self-heal (dynamic RAM code):** KH CoM **generates/writes code into RAM at
  runtime** (EWRAM/IWRAM) for certain handlers, effect AIs and tables. That
  code does not exist at build time. When the PC reaches an unknown region,
  the runtime (1) interprets that function **one run**, (2) **recompiles it to
  native on the fly**, (3) writes it to disk cache, and (4) from then on runs
  it natively. We mark this in the coverage JSON as `healed_native`.

  Honesty rules and beyond:
  - `self_heal_coverage` reports `STATIC` only if **zero** PCs were
    interpreted or served from cache in this session; otherwise `NOT_STATIC`.
  - `dispatch_misses`, `interpreted_insns`, `native_calls`, `healed_native`
    give the exact picture of what happened.

## Files and responsibilities (repo root)

| Path | Role |
|---|---|
| `CMakeLists.txt` | KHRecomp build (corpus + runtime) |
| `src/main.cpp` | Entry, arg parsing, ROM/BIOS load, main loop |
| `game.toml` | Game config (ROM, save chip, entry) |
| `game.build.toml` | Self-contained config copied next to the exe (clone/package) |
| `game_static.toml` | Static corpus: entry, `extra_func` (3275), boundaries |
| `config/eur.toml` | Europe region overrides (sha1/crc32, save flash1m) |
| `generated/` | AOT corpus (versioned; derived artifact of your ROM) |
| `docs/` | This documentation set |
| `gbarecomp/` | Game-agnostic core (mstan fork) + its docs |
| `prism/` | Accessibility bridge (the prebuilt Prism, optional) |

## Notable design decisions

- **`generated/` is versioned** — it is derived from your ROM, but it ships in
  the repo so a clone builds without a recompiler toolchain; regenerate it
  from your own ROM if you want a corpus from your own cartridge. (See
  `NOTICE.md`.)
- **Self-heal is discovery, not emulation.** Interpretation is never the
  permanent engine; it is a one-run bridge to discover and recompile.
- **Mandatory hashes:** the runtime aborts if the ROM or BIOS does not match
  what `game_static.toml` / `config/eur.toml` expect.
- **Honesty is structural:** `gbarecomp`'s `PRINCIPLES.md` is the doctrine; no
  session self-declares `STATIC` with even a single interpreted PC.

## For contributors
- Respect `gbarecomp/CLAUDE.md` (never edit `generated/`; fixes in the
  runtime/config).
- Core internal docs: `gbarecomp/docs/ARCHITECTURE.md`,
  `gbarecomp/docs/TOML_SCHEMA.md`, `gbarecomp/docs/ROADMAP.md`.
- Accessibility: `docs/ACCESSIBILITY.md` (Phase 1 shipped, Phase 2 open).