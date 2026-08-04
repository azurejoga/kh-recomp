#!/bin/bash
# kh_ramheal_soak.sh — prova de estabilidade do RAM-overlay heal no KHRecomp.
#
# Uso: coloque no diretório do executável KHRecomp.exe e rode:
#   bash kh_ramheal_soak.sh
# (ou passe o diretório do exe como $1:  bash kh_ramheal_soak.sh /caminho/do/exe)
set -e
BIN_DIR="${1:-$(pwd)}"
cd "$BIN_DIR"

export GBARECOMP_RAM_OVERLAY_HEAL=1
export GBARECOMP_SELFHEAL_VERBOSE=1

echo "===== [1/2] FULL BOOT (sem savestate) 6000 frames — prova que a intro/cutscene cura nativa ====="
mv savestate_postintro.khs savestate_postintro.khs.BK 2>/dev/null && echo "(savestate movido p/ boot completo)"
timeout 150 ./KHRecomp.exe --no-window --frames 6000 > soak_fullboot.log 2>&1
echo "fullboot exit=$?"
mv savestate_postintro.khs.BK savestate_postintro.khs 2>/dev/null && echo "(savestate restaurado)"
echo "HEALED no fullboot: $(grep -c 'HEALED RAM' soak_fullboot.log)"
echo "interpreted no fullboot: $(grep -o '"interpreted_insns": *[0-9]*' soak_fullboot.log | grep -o '[0-9]*$')"
echo

echo "===== [2/2] GAMEPLAY (savestate auto-resume) 20000 frames ====="
timeout 200 ./KHRecomp.exe --no-window --frames 20000 > soak_gameplay.log 2>&1
echo "gameplay exit=$?"
echo "HEALED RAM no gameplay: $(grep -c 'HEALED RAM' soak_gameplay.log)"
echo
echo "===== COVERAGE FINAL (gameplay) ====="
cat recomp_coverage_B8CP.json 2>/dev/null | python -c "import json,sys;d=json.load(sys.stdin);print(json.dumps({k:d[k] for k in ['coverage','distinct_misses','interpreted_insns','healed_native','native_calls','inflight','failed']},indent=1));print('misses:');[print(' ',m['pc'],m['mode'],'healed=',m['healed'],'native=',m['native_calls']) for m in d['misses']]" 2>/dev/null || echo "sem json"
echo "===== DONE ====="