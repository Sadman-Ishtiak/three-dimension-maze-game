#!/usr/bin/env bash
# Re-creates the figure images and the numbers quoted in Chapter 4, using the REAL
# rendering code (lines 1-316 of ../code/main.cpp) with a tiny stub for SDL types,
# so SDL2 itself is not needed. Needs g++ and ImageMagick (convert).
set -euo pipefail
cd "$(dirname "$0")"
head -n 317 ../code/main.cpp > core.cpp
g++ -std=c++20 -O3 -I. harness.cpp -o harness
./harness
convert v1.ppm ../images/view_spawn.png
convert v4.ppm ../images/view_exit.png
convert v5.ppm ../images/view_checker.png
rm -f v*.ppm core.cpp harness
