#!/bin/bash
set -eu

if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
    echo "Usage: $0 <n> <m> [liste_ids_traitres]"
    echo "Exemple: $0 7 2 0,3"
    exit 1
fi

N=$1
M=$2
TRAITORS=${3-}
cd "$(dirname "$0")/.."

mkdir -p logs
echo "Compilation du projet..."
make

rm -f logs/trace_globale.log
echo "Lancement de OM(m) avec n=$N et m=$M..."
if [ "$#" -eq 3 ]; then
    ./main "$N" "$M" "$TRAITORS"
else
    ./main "$N" "$M"
fi

if [ -n "$TRAITORS" ]; then
    TRAITOR_TAG=$(printf '%s' "$TRAITORS" | tr ',' '-')
else
    TRAITOR_TAG=auto
fi
ARCHIVE="logs/trace_n${N}_m${M}_traitres-${TRAITOR_TAG}.log"
cp logs/trace_globale.log "$ARCHIVE"
python3 scripts/verifier_resultats.py "$ARCHIVE" "$N" "$M"
echo "Journal archive : $ARCHIVE"