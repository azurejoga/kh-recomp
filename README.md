# KHRecomp — Kingdom Hearts: Chain of Memories (GBA) → native PC

Static recompilation of *Kingdom Hearts: Chain of Memories* (Game Boy Advance,
European version) to Windows. Not conventional emulation: **the ROM code is
translated to C++ and runs as native x86-64 code** — with no interpreter on the
hot path.

**The question everyone asks first — "is it 100% native?" — is answered by
tests, not by assertion.** Summary: **the ROM is 100% AOT precompiled** (zero
ROM interpretation, proven with `GBARECOMP_STRICT_STATIC`); the dynamic RAM
code the game generates at runtime is recompiled on the fly with `failed=0`.
Details and numbers in `docs/README.md`.

## Honesty
I used AI for the most part to statically recompile this, so bugs are to be expected and problems too.
Also: I'm not sure how good it turned out, as I'm a person with total blindness.
I hope the community likes it

## Documentation

Start with the index: **`docs/INDEX.md`**.

- `docs/README.md` — overview + the "100% native" proof (real numbers)
- `docs/BUILDING.md` — how to build on Windows (MSYS2/MinGW-w64, SDL2, optional Prism)
- `docs/TESTING.md` — how to run and how to validate (soaks, env vars, STRICT_STATIC)
- `docs/KNOWN_ISSUES.md` — known bugs and limitations + honest roadmap
- `docs/ACCESSIBILITY.md` — the Prism bridge (Phase 1 shipped, Phase 2 open)
- `docs/NOTICE.md` — **READ BEFORE DISTRIBUTING**: ROM/BIOS, credits, disclaimer

## Quick status

- **Execution:** ROM 100% native (AOT); dynamic RAM code self-healed (`failed=0`)
- **Platform:** Windows (D3D11, WASAPI); headless for testing
- **Audio:** mono 65536 Hz (known limitation, see `KNOWN_ISSUES.md`)
- **Accessibility:** Shift+A hotkey speaks via NVDA (Phase 1); text reading open
- **Repository:** source, `generated/`, save, configs and the `prism.dll`
  (with automatic copy at build time) are versioned. The **ROM and BIOS stay
  out** for copyright reasons — you provide your own dumps in the expected
  paths (see `docs/NOTICE.md` and `docs/BUILDING.md`)

## License

MIT (see `LICENSE`). The core derives from `JRickey/gba-recomp` and
`mstan/gbarecomp`; attribution and legal texts in `docs/NOTICE.md` and
`gbarecomp/THIRD_PARTY_ATTRIBUTION.md`. The project is not affiliated with
Nintendo, Square Enix or Disney.