#!/bin/bash
set -eu

cd "$(dirname "$0")/.."

bash scripts/run_test.sh 4 1 1
bash scripts/run_test.sh 7 2 0,3
bash scripts/run_test.sh 3 1 0

echo "Scenarios executes. Les journaux sont archives dans logs/."
