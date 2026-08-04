# Accessibility — the Prism bridge (honest state)

This port has a long-term goal that is **not complete**: making the game
playable by screen readers (NVDA, etc.) and by blind users, along the lines of
the Pokémon Access project. This document describes exactly what exists today,
what does not exist, and what we know needs to happen.

## What the bridge is

`gbarecomp/src/runtime/a11y_prism.{h,cpp}` wraps the C API of
[Prism](https://github.com/ethindp/prism) (MPL-2.0) — a platform-agnostic
TTS/braille library. The bridge is **opt-in at build time**: it is only
compiled when the prebuilt Prism artifacts are in `<repo>/prism/` (CMake
checks for `prism/include/prism.h` and defines `GBARECOMP_HAVE_PRISM=1`).
Without it, the game builds and runs normally, just without speech.

## What works today (Phase 1 — shipped and validated)

- **Focus-free hotkey activation:** holding **Shift + A** toggles accessibility
  mode. The hook uses `GetAsyncKeyState`, so it works even when the game
  window is unfocused — designed exactly that way so the user doesn't have to
  click the window first.
- **Spoken confirmation:** on enable, the game speaks *"Accessibility mode
  on"* and on disable *"Accessibility mode off"* (via NVDA on Windows,
  validated in test sessions with captured audio).
- **Correct Prism integration:** `prism_init(nullptr)` + registry acquisition +
  `prism_backend_initialize` — a single initialization (the double-init
  pattern returns `PRISM_ERROR_ALREADY_INITIALIZED`, which was diagnosed and
  fixed in development).
- **Build:** the keyboard hook is `_WIN32`-only; on other platforms the bridge
  compiles without the system hotkey.

## What does NOT work (Phase 2 — the main goal, open)

The part that actually matters — **reading the game's text** (dialogs, menus,
card names) — **has not been implemented yet**. The model we plan follows
Pokémon Access:

1. Decode the **VRAM text tilemap** (the background layer where KH draws the
   characters) via `bus.vram_ptr()` + the KH CoM tile→character conversion
   table.
2. **Detect text changes between frames** (comparing tilemap content) to
   trigger speech of dialogs/menus.
3. Send the extracted string to Prism.

**Status: not started.** There is no text extraction, no tilemap-change
trigger, no dialog speech at this time. The discovery tool exists
(`a11y_dump.h` / `GBARECOMP_A11Y_DUMP`), but it does not yet produce a
consistent tilemap dump — that's where the investigation stopped.

## Why it is like this

Phase 1 (Prism proof of concept + hotkey) was shipped and validated as the
first safe step. Phase 2 is a real research project: it requires mapping the
KH CoM tile table, understanding the text rendering (which uses its own
tilemap, not the simple standard of other GBAs), and getting the trigger right
without reading the same line repeatedly. Doing that **honestly** — not as a
demo that reads a fixed string — is the remaining work.

## How to contribute / test

- Build with Prism: just have `prism/include/prism.h` + `prism/lib` +
  `prism/bin/prism.dll` in `<repo>/prism/` before running CMake (see
  `docs/BUILDING.md`).
- Test Phase 1: run the game with NVDA active and press **Shift + A**.
- Future work is tracked in `docs/KNOWN_ISSUES.md` (see roadmap).