# KNOWN_ISSUES — Bugs, limitations and the honest state of each

Living catalog of known issues. **Required reading** before using the project
or before opening an issue on GitHub: much of what looks like a bug is already
a known, documented (and in some cases accepted) limitation.

Each entry has an identifier, severity, status, and (where applicable) the log
evidence.

---

## Critical / Audio

### LP-001 — Audio delivered in mono S16 (not stereo)
- **Severity:** medium (a visual/audible limitation, not a break).
- **Description:** the output audio path delivers the signal as `mono S16` at
  65,536 Hz. There is no stereo mixing in the current native path.
- **Observed in:** all soaks (e.g. 20k–60k frame validation logs, exit=0).
- **Status:** **known and accepted limitation** — not a regression bug, it is
  the current design of the audio pipeline. Converting it to stereo is an
  evolution, not a defect fix.

### LP-02 — Slightly accelerated BIOS chime timing
- **Severity:** low (imperceptible in-game).
- **Description:** the BIOS chime audio samples diverge slightly from
  hardware (12 samples of difference at the start). The chime sounds
  accelerated.
- **Evidence:** sample analysis of `bios_intro` (tests).
- **Status:** documented in the upstream ISSUES.md; **accepted** — irrelevant
  in real use, but not closed.

---

## Stability / Runtime

### KN-001 — "terminate called without an active exception" when closing the window
- **Severity:** low (only on close; does not corrupt save).
- **Description:** when closing the window in some scenarios, the runtime
  emits `terminate called without an active exception` (unhandled C++
  exception) in the logs. The process still exits.
- **Evidence:** present in 5 logs (terminate at shutdown).
- **Status:** known; clean-shutdown investigation pending.

### KN-002 — Call-return stack overflow (rare cold soak)
- **Severity:** medium (rare).
- **Description:** in early cold soaks, occasionally
  `generated call-return stack overflow` appeared when a recursive
  BIOS/intro dialog overflowed the runtime's call-return stack.
- **Status:** mitigated with the larger call-return stack; **not reproduced**
  in the current Release build. Monitor.

### KN-003 — Busy-spin hang (MC-HP-002 legacy)
- **Severity:** low (described, not reproduced in the current build).
- **Description:** "busy-spin hang" bug class in certain RAM regions,
  historically documented (MC-HP-002). No occurrence in the current Release.
- **Evidence:** old diagnostic file (`hang_dump`).

---

## Self-heal / Coverage — honesty

### LOSGEND — Coverage reported as NOT_STATIC (by design)
- **Severity:** it is a **feature**, not a bug.
- **Description:** the runtime refuses `STATIC` while **any** PC was
  interpreted or served from cache in this session. The `NOT_STATIC` label
  does not mean the port is broken — it means the project holds itself to
  honesty.
- **Proof that the ROM is 100% native:** with `GBARECOMP_STRICT_STATIC=1` the
  runtime aborts on the first unmapped PC. The only observed abort is
  `pc=0x02038D48` (EWRAM). **No ROM address (`0x08...`) aborts** — the entire
  ROM is covered by AOT precompilation.
- **Real value (20,000-frame warm soak, run 2026-08-03):**
  `dispatch_misses=13` (all EWRAM/IWRAM, none ROM), `interpreted_insns=4842`
  out of `native_calls=788411` (0.6%), `healed_native=3377`, `failed=0`,
  `exit=0`.
- **Technical note:** the 13 misses are **dynamic RAM code** the game itself
  generates at runtime (EWRAM/IWRAM) — impossible to precompile in any
  recompiler (FireRedLeafGreenRecomp, EmeraldRecomp etc. recompile this
  on the fly the same way).

### HE-001 — Heal failures in partial cache (`failed=3–8`)
- **Severity:** low.
- **Description:** in runs with a partially populated disk cache (cold
  client), there may be `failed=3–8` in the IWRAM region. The runtime re-heals
  and continues.
- **Status:** benign — a hot cache rises to `failed=0` (verified in the final
  soaks).

---

## Accessibility — work in progress

### A11-001 — Shift+A speaks "Accessibility mode on/off" (DONE)
- **Description:** the global hotkey works and speaks through the Prism
  bridge (NVDA → OneCore → SAPI). Validated with NVDA on the Windows host.
- **Status:** completed.

### A11-002 — Automatic text reader (Phase 2) — NOT IMPLEMENTED
- **Description:** accessibility does not automatically read the game's
  menu/dialog text. The goal (inspired by Pokémon Access) is: read the **VRAM
  text tilemap** every frame, decode the glyphs with the KH CoM conversion
  table, and announce the text by voice when it changes.
- **Existing tools:** `GBARECOMP_A11Y_DUMP=<path>` writes, at each dump-frame,
  the video registers + text tilemaps + a round-trip, to discover which
  background carries the glyphs and in what encoding.
- **Linux backup status:** the backup in `D:\backup_kh\kh-recomp-linux`
  **does not contain** any tile analysis (zero font/VRAM/charset files) — only
  the recompiled corpus (982 functions). The tile question remains open and is
  the main cell of Phase 2.
- **How to contribute:** run with `GBARECOMP_A11Y_DUMP=dump.txt`, play until a
  menu with text, and share the dump to map the encoding.

---

## HONEST roadmap to "100% static native"

**Current state (2026-08-03): ROM already 100% AOT, proven by `STRICT_STATIC`**
(the only abort is in RAM; zero aborts in `0x08...`). What remains is the
game's dynamic RAM code, which is already recompiled on the fly with
`failed=0`.

1. ✅ Close the ROM `dispatch_misses` — **done**: 3,275 `extra_func` in
   `game_static.toml`; zero ROM misses observed.
2. Keep monitoring the on-the-fly recovered IWRAM/EWRAM: reducing the 13
   dynamic RAM misses remains ongoing work (identify stable handlers and move
   them to AOT when possible), but it will never be zero — runtime RAM code is
   inherent to the GBA.
3. Instrument `GBARECOMP_STRICT_STATIC` in CI to **prove** (with each release)
   that no ROM PC is bridged — the real, achievable goal.

Always-standing caveat: game-generated dynamic RAM code can never be literal
static AOT — it is always recompiled on the fly. That is inherent to the GBA,
not a limitation of this project.