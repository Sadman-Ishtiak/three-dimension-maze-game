#!/usr/bin/env bash
set -euo pipefail

g++ main.cpp -o main \
    -I.. \
    -std=c++20 \
    -g -O3 \
    -Wall -Wextra -Wpedantic -Wshadow \
    $(sdl2-config --cflags --libs)

./main
