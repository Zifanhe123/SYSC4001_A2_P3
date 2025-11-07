#!/usr/bin/env bash
set -euo pipefail
g++ -std=c++17 -O2 -Wall -Wextra \
  Interrupts_101258593_101258593.cpp -o sim
./sim
echo "Wrote output_files/execution.txt and output_files/system_status.txt"
