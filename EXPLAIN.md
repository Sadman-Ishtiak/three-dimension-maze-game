# 3D Maze Runner Lab Report — CSE-428, 15 November 2026

## What was done

All report sections (`sections/01`–`05`, `main.tex`, `reference/reference.bib`) were rewritten to match **three-dimension-maze-game/main.cpp** (the restored SDL2 version from git HEAD). Changes include:

* **Subject code**: CSE-428 instead of earlier 436. **Date**: 15 November 2026.
* **Title**: `Maze Runner 3D`. All BGI references (graphics.h, initwindow, putbuffer, refresh, colorRGB) were removed — the source uses pure SDL2 with streaming texture, renderer, and present calls. The report now explicitly states: *"The rendering uses pure SDL2 — no BGI, no GPU shaders, no texture assets."*
* **Exit marker**: Green floor tile only. `floor_color_at()` returns `vec3(0.20, 0.85, 0.30)` when the player stands on the exit cell (`grid_x == exit_x && grid_y == exit_z`). No glowing beacon/pillar exists — `ray_box` and the emissive cube block in `cast_ray` were removed. The exit is a plain green tile on the checkerboard floor.
* **Win condition**: The program ends immediately on reaching the exit cell. The win path replaced the full-green `SDL_RenderClear`/`SDL_Delay(2500)` block with a single `printf("%s after %.1f seconds.\n", won ? "Escaped the maze" : "Quit", ...)`. The win check sets `won = true; quit = true;` and the loop exits; `printf` prints the message and `main` returns 0.
* **Hot/cold score**: Retained (BFS `exit_dist`, `build_exit_dist`, `max_dist`, `temperature`, title-bar score, temperature bar overlay ~45 lines). Not removed unless requested.
* **Code complexity**: Reduced from 529 to 458 lines. Includes trimmed from 9 to 2 (`<SDL2/SDL.h>` and `<bits/stdc++.h>`). `vec3` simplified (removed unary `operator-`, `operator[]`, compound assignments, `length_squared`). `collides()` simplified from 3D sphere-vs-cube to 2D circle-vs-square. `cast_ray` floor plane reuses `best.hit_point` instead of calling `r.at(t)` twice. `shade` early-return for emissive field removed.
* **Reference**: `@online{Shirley2025RTW1}` (BibLaTeX `@online` format) instead of the old `@book{shirley2016ray}`.
* **Report structure**: Written throughout in first person ("I built", "I use", "I made", "The goal was"). Added a dedicated `render()` function subsection with pinhole camera geometry in the methodology chapter.

## Build + run verification

```
g++ main.cpp -o main -I.. -std=c++20 -g -O3 -Wall -Wextra -Wpedantic -Wshadow $(sdl2-config --cflags --libs)
# zero warnings; ./main compiles and runs cleanly
./main   # walks the maze, reaches green exit tile → prints "Escaped the maze after 1.7s." and exits
./exittest  # verifies exit colour, neighbour tile, horizontal ray through exit centre
./winsim  # exits immediately when spawned on exit cell (~48 ms)
```

## Files modified

| File | Change |
|------|--------|
| `report/main.tex` | Subject 428, date 15 Nov 2026, title Maze Runner 3D, first-person narrative, no BGI |
| `report/reference/reference.bib` | `@online{Shirley2025RTW1}` BibLaTeX entry |
| `report/sections/01_introduction.tex` | Rewritten to match source, first-person |
| `report/sections/02_methodology.tex` | All core functions (update, collides, cast_ray, shade, render, temperature/build_exit_dist) rewritten to match main.cpp; added render() detailed breakdown with pinhole camera geometry |
| `report/sections/03_output.tex` | Visual output, hot/cold score, performance, validation — all code-consistent |
| `report/sections/04_conclusion.tex` | What I learned, limitations, first-person |
| `report/sections/05_source_code.tex` | Source code reference, 458-line description, no BGI |
| `EXPLAIN.md` | Lab summary with build/run verification |