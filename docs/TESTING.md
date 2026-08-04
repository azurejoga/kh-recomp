# TESTING — How to run and how to validate

This guide shows how to run the game and how to reproduce the headless
validation matrix this build uses to prove (and honestly reveal) the state of
the port.

---

## Interactive run

```powershell
.\build\KHRecomp.exe --rom "C:\roms\khcom_eur.gba" --bios "C:\bios\gba_bios.bin"
```

- Arguments: `--bios <path>`, `--rom <path>`, and optionally a `game.toml`.
  Omitted values are pulled from the TOML or the built-in defaults.
- The runtime **validates the ROM hashes** (SHA-1 `8db73586...` and CRC32
  `0x772D97FB` of the European version) before executing any code and aborts
  on mismatch.

**Controls (default):** A=X, B=Z, L=C, R=V, Start=Return, Select=RShift,
D-pad=arrow keys. Remappable via `keybinds.ini`. Hotkeys: `Alt+Return`
fullscreen, `Shift+P` pause, `Tab` turbo, `F` perf overlay, `Shift+A`
accessibility.

**Save-states:** `F1..F12` (plain key = load, Shift = save). Game saves go to
the Flash 1M (128 KB) chip.

---

## Headless / deterministic flags

The executable has a headless mode used by the tests:

```bash
KH\KHRecomp.exe --rom "...\khcom_eur.gba" --bios "...\gba_bios.bin" \
                --no-window --frames 20000 [--tcp 0] [--dump-bmp out]
```

- `--frames <N>` — runs N PPU frames and exits.
- `--steps <N>` — runs N CPU steps (alternative to frames).
- `--no-window` — no window (CI / soaks).
- `--tcp <port>` — TCP debug server (`0` = off).
- `--dump-bmp <prefix>` — writes frames as BMP (pixel-by-pixel comparison).

---

## Diagnostic env vars (fine control)

| Env var | Effect |
|---|---|
| `GBARECOMP_BIOS_HLE` | turns on the optional HLE BIOS (instead of recompiled LLE) |
| `GBARECOMP_SELFHEAL_VERBOSE` | detailed log of every heal/cache |
| `GBARECOMP_SELFHEAL_RECOMPILE` | force on-the-fly recompile instead of cache |
| `GBARECOMP_STRICT_STATIC` | aborts (instead of self-heal) on any unmapped PC — the "purity" test |
| `GBARECOMP_ENABLE_MODS` | enables module overlays (opt-in, off by default) |
| `GBARECOMP_NO_LAUNCHER` | skips the launcher |
| `GBARECOMP_DEFAULT_GAME_CONFIG` | points to a specific `game.toml` |
| `GBARECOMP_SCREEN` / `GBARECOMP_SOLAR` | screen profile (raw/unlit/frontlit/backlit/classic) / solar filters |
| `GBARECOMP_A11Y_DUMP=<path>` | VRAM snapshots → discover text tiles (Phase 2 a11y) |
| `GBARECOMP_SAVE_TYPE` | save chip type override (e.g. `flash1m`) |
| `GBARECOMP_FP_SAVE` | fingerprint-ring save dump to a file |

---

## Validation matrix (test-bench soaks)

The tests run with `--no-window --frames <N>` and the result is inspected via
the `final_pc`, `ppu_frames` and `self_heal_coverage` lines in the log.
Expected values of the final build:

| Test | Target frames | Expectation |
|---|---|---|
| Cold boot | up to screen | `exit=0`, recompiled BIOS runs to the ROM |
| Headless soak | 6,000–30,000 | `failed=0`, `final_pc` = thread idle, no crash |
| Combat prewarm | 30,000 | 3,651 heals, `failed=0` — validates AIs/effects |
| RAM heal | 3,000 | `failed=0` |
| Warm cache | 20,000 | `failed=0`, indispensable |
| Boot frame | 1 | `unmapped=0 io_unhandled=0` |

Line of interest (real example, 20,000-frame warm soak):

```
summary: final_pc=0x00000348 unmapped=0 io_unhandled=0 steps=59713
  cycles=5347265721 ppu_vcount=168 ppu_frames=20000 frames_presented=20000
self_heal_coverage=NOT_STATIC dispatch_misses=13 interpreted_insns=4842
  healed_native=3257 native_calls=1222334 inflight=0 failed=0
```

---

## How to read (and be honest about) the self-heal line

- `self_heal_coverage=NOT_STATIC` is the **state reported when any PC was
  interpreted or served from cache** in this session. It is by design
  (`PRINCIPLES.md`), not a defect.
- `dispatch_misses` = how many new functions were found at runtime.
- `healed_native` = how many were recompiled on the fly and now run natively.
- `interpreted_insns` = total instructions that went through the discovery
  bridge (the fraction of the total is tiny in a hot soak: 4,842 out of
  ~1.22M = 0.4%).
- `failed=0` = no heal failed (integrity cache). Values 3–8 appear in partial
  cache (cold client) and are benign: the runtime re-heals.

For a **strict purity check** (must **fail** if anything is interpreted), use
`GBARECOMP_STRICT_STATIC=1`. The flag makes the runtime abort instead of
self-healing — it is the test that denies the "100% literal static" label if
any piece is not covered.

---

## What is not tested yet / limits

- **Audio** is delivered `mono S16` at 65,536 Hz (path limitation, not a bug) —
  there is no stereo/frequency test because the path is mono.
- **Accessibility Phase 2** (reading menu/dialog text) is not implemented;
  `GBARECOMP_A11Y_DUMP` is the tool to map the text-background tiles, future
  work.
- **Window closing** may emit `terminate called without an active exception`
  (benign, recurring in the logs).
- **Stack overflow crashes** were observed in early cold soaks (rare,
  `generated call-return stack overflow`), already mitigated with the larger
  call-return stack; not reproduced in the current Release build.

See `docs/KNOWN_ISSUES.md` for the complete catalog and the state of each bug.