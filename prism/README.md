# prism/ — accessibility bridge (prebuilt Prism)

This folder contains the prebuilt artifacts of
[Prism](https://github.com/ethindp/prism) (MPL-2.0) used to compile the
KHRecomp accessibility bridge (speaking via NVDA / other screen readers).

- `include/prism.h` — the Prism C API
- `lib/libprism.a` / `lib/prism.lib` — libraries (MinGW-w64 / MSVC)
- `bin/prism.dll` — runtime DLL (optional; the bridge links statically when
  possible)

## How it is used

`gbarecomp/CMakeLists.txt` checks for `prism/include/prism.h`; if present, it
defines `GBARECOMP_HAVE_PRISM=1` and compiles
`gbarecomp/src/runtime/a11y_prism.{h,cpp}`. **Without this folder, the build
works normally, just without the voice bridge.**

## License and origin

- Project: [Prism](https://github.com/ethindp/prism) — MPL-2.0.
- Logically, this folder is a **third-party dependency** redistributed here
  for build convenience. See `README.md` (Notice) and
  `gbarecomp/THIRD_PARTY_ATTRIBUTION.md`. The full Prism source lives in the
  upstream repository.

## Project phase

The bridge is at Phase 1 (shipped and validated): the **Shift + A** hotkey
turns on confirmation speech via NVDA. Phase 2 (reading the game's text from
VRAM) is **not yet implemented** — see `README.md` (Accessibility).