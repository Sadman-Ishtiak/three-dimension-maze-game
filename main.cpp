#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include<bits/stdc++.h>
#define PI 3.14159265358979323846
using namespace std;


// Global variables and constant definition area
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;


int32_t main() {
    // 1. Checking everything before running the code
    cout << "Hello World" << endl;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // 2. Create the window
    SDL_Window* window = SDL_CreateWindow(
        "Maze Runner 3D",          // Window title
        SDL_WINDOWPOS_CENTERED,      // Initial x position
        SDL_WINDOWPOS_CENTERED,      // Initial y position
        SCREEN_WIDTH,                // Width, in pixels
        SCREEN_HEIGHT,               // Height, in pixels
        SDL_WINDOW_SHOWN             // Flags (makes it visible)
    );

    // Check if the window was successfully created
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // 3. Main event loop setup
    SDL_Event event;
    bool quit = false;
    double start_time = SDL_GetTicks();

    // Keep the window open until the user closes it
    while (!quit) {
        // Poll for pending events (clicks, keypresses, window close)
        while (SDL_PollEvent(&event) != 0) {
            // User requests quit (e.g., clicking the 'X' button)
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }
    }

    double end_time = SDL_GetTicks();
    cout << "Total time spent = " << (end_time - start_time)/1000 << " seconds\n";

    // 4. Clean up and free resources
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
