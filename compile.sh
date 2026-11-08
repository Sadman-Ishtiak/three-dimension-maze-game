#!/usr/bin/env bash
set -euo pipefail

g++ main.cpp -o main \
    -std=c++20 \
    -O3 \
    -Wall -Wextra -Wpedantic -Wshadow \
    $(sdl2-config --cflags --libs)

./main
