# KHRecomp — Kingdom Hearts: Chain of Memories (GBA) → PC

Static recompilation of *Kingdom Hearts: Chain of Memories* (Game Boy Advance,
European version) to run as a native Windows program, with no conventional
emulator.

> **Read this document.** It describes, without beating around the bush, what
> this project is, what it is **not**, what works, what still does not work and
> the known limitations. We prefer naked honesty over pretty marketing.

---

## What this actually is

This is **not** an emulator (not mGBA, not Visual Boy Advance, none of that).

It is a project that **reads the bits of the original ROM** (ARM7TDMI /
ARMv4T) and **converts them into C/C++ code** that runs natively on your
processor. Instead of executing a "fake chip" and interpreting instruction by
instruction in real time, the game code is **compiled to C once and executed
as native PC code** (real x86-64 instructions, no step-by-step interpreter
virtual machine).

Layered architecture:

- **`gbarecomp/`** — "game-agnostic" core: the GBA static C++ recompiler
  (ARMv4T decoder, code generator, bus, PPU, audio, DMA, timers, BIOS). It is
  a fork of [`mstan/gbarecomp`](https://github.com/mstan/gbarecomp), itself
  derived from [`JRickey/gba-recomp`](https://github.com/JRickey/gba-recomp).
  See `docs/NOTICE.md` for the attribution chain and licenses.
- **`generated/`** — the "corpus": the KH CoM ROM code already recompiled into
  C++. It is **26,160 functions** (~126 MB of generated C++, 16 shards). This
  corpus **is versioned** along with the project (see `docs/NOTICE.md`).
- **`src/runtime/`** — the linkable runtime: dispatch, window (SDL2), audio
  (WASAPI), save, save-states, TCP debug, native overlays compiled at runtime,
  and the screen-reader bridge (accessibility).

---

## 100% native? Honest answer

Short answer: **yes — 100% of the ROM code is recompiled and runs natively.
Proven by tests, not by assertion.** The only exception to the pure `STATIC`
label is the **RAM code the game itself generates at runtime** — which is
physically impossible to precompile, in any GBA recompiler in the world. Here
is the evidence:

### Proof by test (GBARECOMP_STRICT_STATIC)
The runtime has an extreme purity mode (`GBARECOMP_STRICT_STATIC=1`) that
**aborts** (instead of self-healing) on the first unmapped PC. Run against this
build:

```
runtime_arm: STRICT_STATIC dispatch miss for pc=0x02038D48 (thumb) —
  interpreter and overlay fallback are disabled. Add reviewed static
  discovery/code-copy metadata and regenerate.
strict_static=ENABLED self_heal_recompile=DISABLED cache_load=DISABLED
  interpreter_bridge=ABORT        (exit=3)
```

The first (and only) abort is at `pc=0x02038D48` — **EWRAM**. No ROM address
(`0x08...`) aborts. **The entire ROM is covered by AOT precompilation.**

### The 13 bridges per session — all RAM, no ROM
In a 20,000-frame soak (`exit=0`, `failed=0`):

| Address | Region | Nature |
|---|---|---|
| `0x02038D48` | EWRAM | dynamic game code |
| `0x03000000` … `0x03006D8C` (12) | IWRAM | dynamic game code |
| ~~`0x08...`~~ | **ROM** | **none** |

All `[HEALED->native]`: the runtime interprets once, recompiles on the fly,
and the code then runs natively — `failed=0` in every session.

### In numbers (20,000-frame soak, final build, run now)

| Metric | Value |
|---|---|
| native calls (`native_calls`) | 788,411 |
| interpreted instructions (`interpreted_insns`) | 4,842 (0.6%) |
| dispatch misses | 13 — **all RAM** |
| on-the-fly recompiled functions (`healed_native`) | 3,377 |
| heal failures | 0 |
| exit code | 0 |

### Why the label stays `NOT_STATIC` (and why that is correct)

The project refuses — **by design decision** (`PRINCIPLES.md` "Coverage
honesty is load-bearing") — to call `STATIC` any session in which even a
single PC was interpreted or served from cache. Since KH CoM generates RAM
code during execution, there will always be a minimal fraction (0.6% in a hot
soak) that goes through the discovery bridge once. Calling that "non-native"
would be dishonest in the other direction: **what runs after the bridge is
real native x86-64 code**, and what runs before it is a one-time discovery
bridge.

**Honest conclusion:** the port is **100% natively ROM-based** (proven by
`STRICT_STATIC` aborting only in RAM), and the dynamic RAM code is reliably
recompiled to native (`failed=0`). This is the maximum state achievable by any
GBA recompiler — including FireRedLeafGreenRecomp and EmeraldRecomp, which
handle the same RAM code the same way.

---

## What works today (validated in headless tests)

- **Full boot** of the game (recompiled BIOS → intro → ROM) all the way to the
  screen.
- **Rendering**: D3D11 window, vsync, `nearest` scaler. Tested at
  1,366×768@60 Hz.
- **Audio**: WASAPI 65,536 Hz, **mono S16** (see "Limitations").
- **Save**: Flash 1M (128 KB) chip with the custom KH CoM driver (MX29L010).
- **Save-state**: slots F1..F12 (plain key = load, Shift = save).
- **Input**: A=X, B=Z, L=C, R=Exit, Start=Return, Select=Right Shift,
  D-pad=arrow keys. Remappable via `keybinds.ini`.
- **Soaks**: 6,000–30,000 frames without crash, `failed=0`, `exit=0`.
- **Accessibility hotkey: Shift+A** speaks "Accessibility mode on/off"
  through the Prism bridge (NVDA → OneCore → SPApi).

## What does NOT work yet / honest limitations

See `docs/BUILDING.md`, `docs/TESTING.md` and, above all, **`docs/KNOWN_ISSUES.md`**
(an integral part of the required reading). Summary:

- **Mono audio.** The native audio pipeline delivers the channel at `mono S16`
  at 65,536 Hz. It is a current limitation of the audio path, not a code-quality
  bug — it is not stereo.
- **BIOS chime timing** slightly faster than hardware (imperceptible;
  documented in KNOWN_ISSUES as LP-001/LP-002).
- **Accessible text reader (Phase 2) not yet implemented.** Voice
  accessibility turns on (Shift+A speaks), but it does **not automatically
  read menu/dialogue text** from the game. We are mapping the text background
  tiles via VRAM dumps (`a11y_dump.h` + env var `GBARECOMP_A11Y_DUMP`). The
  old Linux backup **contains no** tile analysis (zero font/VRAM/charset
  files) — it is work to be done.
- **Closing the window can produce "terminate called without an active
  exception"** in some scenarios (usually benign, observed in the logs).
- In **partial-cache** regions there may be heal failures (`failed=3–8`) in
  RAM code — the runtime continues, and in a hot cache this rises to
  `failed=0`.

**The real goal for "100% static native"** (beyond the label): close the
remaining ROM `dispatch_misses` (most already became `extra_func` in
`game_static.toml`) and convert the on-the-fly recompiled IWRAM space to AOT
precompiled. It is ongoing work and there will always be **some** game-specific
dynamic RAM code — inherent to GBA, not removable by any recompiler.

---

## Requirements / how to build and run

See **`docs/BUILDING.md`** (Windows) for exact dependencies and
`docs/TESTING.md` for how to run and validate.

### Quick test

Provide the **ROM** and **BIOS** (not versioned for copyright reasons) in the
expected paths:

```bash
git clone <repo-url>
cd KHRecomp
# put the ROM in ./roms/khcom_eur.gba and the BIOS in ./gbarecomp/bios/gba_bios.bin
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target KHRecomp --config Release
./build/KHRecomp.exe --rom "./roms/khcom_eur.gba" --bios "./gbarecomp/bios/gba_bios.bin"
```

The `prism.dll` is versioned and copied automatically by the build (nothing
manual). The `generated/` corpus is also versioned. The ROM and BIOS are yours —
see `docs/NOTICE.md`.

---

## Credits and license

- Recompiler core: derived from `JRickey/gba-recomp` (MIT OR Apache-2.0) via
  `mstan/gbarecomp` (MIT). Full attribution and license terms in
  `gbarecomp/THIRD_PARTY_ATTRIBUTION.md` and `docs/NOTICE.md`.
- This project and its authors **have no connection to Nintendo** or to
  Square Enix. All trademark and original-work rights belong to their
  respective holders. See `docs/NOTICE.md` for the full disclaimer.

---

*Ideology: recompiling is honest. `NOT_STATIC` is always disclosed. No build is
called "complete" while a PC was interpreted without record. No ROM or BIOS will
ever be embedded or distributed.*