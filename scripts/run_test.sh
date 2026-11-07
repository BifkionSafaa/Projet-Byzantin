#!/bin/bash

# Vérifier si les arguments sont fournis
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <nombre_total> <nombre_traitres>"
    echo "Exemple: $0 4 1  (4 généraux, dont 1 traître)"
    exit 1
fi

N=$1
M=$2

# Se placer à la racine du projet
cd "$(dirname "$0")/.."

echo "🧹 Nettoyage des anciens logs..."
rm -f logs/*.log

echo "🔨 Compilation du projet..."
make clean && make
if [ $? -ne 0 ]; then
    echo "❌ Erreur de compilation."
    exit 1
fi

echo "🚀 Lancement du système avec N=$N généraux et M=$M traîtres..."
./main "$N" "$M"

echo "✅ Terminé. Vérification des logs :"
ls -l logs/
echo "👉 Pour voir les logs : cat logs/trace.log"