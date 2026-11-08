#!/usr/bin/env bash
set -euo pipefail

g++ main.cpp ../map.cpp -o main \
    -I.. \
    -std=c++20 \
    # -O3 \
    -Wall -Wextra -Wpedantic -Wshadow \
    $(sdl2-config --cflags --libs)

./main
