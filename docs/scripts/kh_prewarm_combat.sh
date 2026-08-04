#!/bin/bash
# kh_prewarm_combat.sh — pré-aquece o cache com os shards de gameplay
# (batalhas/overlays RAM) usando o harness determinístico do fork.
#
# Uso: coloque este script no diretório do executável KHRecomp.exe e rode:
#   bash kh_prewarm_combat.sh
# (ou passe o diretório do exe como $1:  bash kh_prewarm_combat.sh /caminho/do/exe)
set -e
BIN_DIR="${1:-$(pwd)}"
cd "$BIN_DIR"

# Boot limpo (sem savestate): o harness DEMO_INPUT dirige menu→intro→gameplay→combate.
# RAM overlay heal está ON por padrão no exe; verbose ligado pra registrar os heals.
GBARECOMP_SELFHEAL_VERBOSE=1 \
GBARECOMP_DEMO_INPUT=campaign-combat \
./KHRecomp.exe --no-window --frames 30000 > prewarm_combat.log 2>&1
echo "exit=$?"
