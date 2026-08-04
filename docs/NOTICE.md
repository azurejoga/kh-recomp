# NOTICE — ROM, BIOS, trademarks and disclaimer

This document is **legally informative** and must be read before using,
building or redistributing this project.

---

## 1. ROM, BIOS and corpus — maintainer's decision

> **Maintainer's decision (2026-08-03, revised):** this repository versions
> **source, the `generated/` corpus, save, configs and the `prism.dll`** (the
> latter with automatic copy next to the executable at build time). **The game
> ROM and the GBA BIOS are deliberately left out** for copyright reasons — you
> provide your own dumps in the paths the runtime expects.

- The C++ corpus in `generated/` is an **artifact derived from your ROM**; if
  you remove it, the build must regenerate it via `gba_recompile --rom ...
  --config game_static.toml --out generated` (see `docs/BUILDING.md`).
- **You decide what to distribute:** before publishing on any public platform
  (GitHub, etc.), understand that **cartridge ROMs and console BIOSes are
  copyrighted material of Nintendo / Square Enix / Disney**, even when
  recoded or re-derived. Their inclusion here **does not** grant any right of
  redistribution to third parties. It is your responsibility to assess the
  legality of publishing in your context before uploading.
- The runtime **validates hashes** of the ROM (SHA-1
  `8db73586cdb11b3795907edebf43228dbcd3e6b2`, CRC32 `0x772D97FB`) and of the
  BIOS before executing any code.

## 2. Legal use

- **Back up your own cartridges.** Using dumps obtained outside your personal
  copy may violate copyright in your jurisdiction. You are responsible for the
  legality of what you do with this software and with the images you provide.
- This project is **not affiliated with, endorsed or sponsored by** Nintendo,
  Square Enix, Disney, or any other rights holder related to *Kingdom Hearts:
  Chain of Memories*.
- All product names, trademarks and original works belong to their respective
  owners.

## 3. Core attribution chain

The recompilation core derives from third-party open-source projects:

| Project | Origin | License |
|---|---|---|
| `JRickey/gba-recomp` | https://github.com/JRickey/gba-recomp | MIT OR Apache-2.0 |
| `mstan/gbarecomp` | https://github.com/mstan/gbarecomp | MIT |
| `mgba-emu/mgba` (partial, vendored) | https://github.com/mgba-emu/mgba | MPL-2.0 |
| `ethindp/prism` (accessibility bridge, opt-in) | https://github.com/ethindp/prism | MPL-2.0 |

File-by-file attribution is in `gbarecomp/THIRD_PARTY_ATTRIBUTION.md`.
Where the law requires, the upstream license texts are reproduced in their
respective repositories.

## 4. Disclaimer

THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.

IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

The software is provided for **research, education and software
interoperability** with systems the user legitimately owns.

## 5. No credentials, no secrets

This repository **does not contain** API keys, passwords, tokens or other
credentials. If something like that is found in the history, it must be
considered compromised and revoked immediately.