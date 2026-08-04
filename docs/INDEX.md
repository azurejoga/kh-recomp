# Documentation Index — KHRecomp

Navigation map for all project documentation. Read in the order below the
first time; afterwards, jump to the sections you need.

## Starting point
- **`README.md`** (root) — one-paragraph overview + quick access to the guides.
- **`docs/README.md`** — full overview: what it is, how it works, and the
  honest, proven answer to "is it 100% native?".
- **`docs/NOTICE.md`** — **READ BEFORE DISTRIBUTING**: ROM/BIOS are not
  provided, credits and disclaimer.

## Build and run
- **`docs/BUILDING.md`** — how to build on Windows (MSYS2/MinGW-w64), requirements,
  dependencies and the corpus regeneration step.
- **`docs/TESTING.md`** — how to run the game and how to reproduce the
  validation matrix (headless soaks, flags, env vars).

## Status and honesty
- **`docs/KNOWN_ISSUES.md`** — living catalog of bugs and limitations + the
  honest roadmap to the real goal.
- **`docs/scripts/`** — revalidation scripts (combat prewarm, RAM-heal soak)
  used in the test matrix.

## Accessibility (Prism)
- **`docs/ACCESSIBILITY.md`** — the state of the accessibility bridge: what
  speaks today, what we plan and what is intentionally *not* done.

## Development (core)
The recompilation core lives in **`gbarecomp/`** and has its own
documentation: `PRINCIPLES.md` (the honesty doctrine), `ISSUES.md`,
`RELEASE_NOTES.md`, `ENHANCEMENTS.md`, `DEBUG.md`, `COSIM_ORACLE.md`, `TCP.md`
and `docs/` (architecture, build-perf, TOML schema, roadmap). Respect the
rules in `gbarecomp/CLAUDE.md` (don't edit `generated/`, fixes go in the
runtime/config).

---

*Suggested first-read order: root README → docs/README → docs/BUILDING →
docs/TESTING → docs/KNOWN_ISSUES → docs/ACCESSIBILITY → docs/NOTICE.*