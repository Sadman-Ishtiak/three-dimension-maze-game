#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include<bits/stdc++.h>
#include <cmath>
#include <cstddef>
#define PI 3.14159265358979323846
using namespace std;

const int MAZE_W = 16;
const int MAZE_H = 16;

const char* MAP[MAZE_H] = {
    "################",
    "#S....#........#",
    "#####.#.######.#",
    "#...#.#......#.#",
    "###.#.#.####.#.#",
    "#...#.#.#....#.#",
    "#.###.#.#.####.#",
    "#.#...#.#......#",
    "#.#.###.######.#",
    "#.#.....#.#....#",
    "#.#######.#.####",
    "#.........#....#",
    "#.#########.##.#",
    "#.#..........#.#",
    "#...#####.#..#E#",
    "################",
};


// Global variables and constant definition area
const int       SCREEN_WIDTH    = 640;
const int       SCREEN_HEIGHT   = 480;
const double    EYE_HEIGHT      = 0.5;   // camera height: middle of a 1-unit wall
const double    FOV_DEG         = 75.0;

class vec3 {
    public:
        double e[3];
        vec3() : e{0,0,0} {}
        vec3(double e0, double e1, double e2) : e{e0, e1, e2} {}

        double x() const { return e[0]; }
        double y() const { return e[1]; }
        double z() const { return e[2]; }

        vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }
        double operator[](int i) const { return e[i]; }
        double& operator[](int i) { return e[i]; }

        vec3& operator+=(const vec3& v) {
            e[0] += v.e[0];
            e[1] += v.e[1];
            e[2] += v.e[2];
            return *this;
        }
        vec3& operator*=(double t) {
            e[0] *= t;
            e[1] *= t;
            e[2] *= t;
            return *this;
        }
        vec3& operator/=(double t) {
            return *this *= 1/t;
        }
        double length_squared() {
            return x()*x() + y()*y() + z()*z();
        }
        double length() {
            return sqrt(length_squared());
        }
};

inline vec3 operator+(const vec3 a, const vec3 b) {
    return vec3(a.x()+b.x(), a.y()+b.y(), a.z()+b.z());
}
inline vec3 operator-(const vec3 a, const vec3 b) {
    return vec3(a.x()-b.x(), a.y()-b.y(), a.z()-b.z());
}
inline vec3 operator*(const vec3 a, const vec3 b) {
    return vec3(a.x()*b.x(), a.y()*b.y(), a.z()*b.z());
}
inline vec3 operator*(const double a, const vec3 b) {
    return vec3(a*b.x(), a*b.y(), a*b.z());
}
inline vec3 operator*(const vec3 b, const double a) {
    return vec3(a*b.x(), a*b.y(), a*b.z());
}
inline vec3 operator/(const vec3 a, double t) {
    return (1/t) * a;
}
inline double dot(const vec3 a, const vec3 b) {
    return a.x()*b.x() + a.y()*b.y() + a.z()*b.z();
}
inline vec3 cross(const vec3 a, const vec3 b) {
    return vec3(
        a.y()*b.z() - a.z()*b.y(),
        a.z()*b.x() - a.x()*b.z(),
        a.x()*b.y() - a.y()*b.x()
    );
}
inline vec3 unit_vector(vec3 a) {
    return a/a.length();
}

class ray {
    public:     // default for struct
        vec3 origin, direction;
        ray(vec3 o, vec3 d) : origin(o), direction(d) {}
        vec3 at(double t) const { return origin + direction*t; }
};

class hit_record {
    public:
        double ray_t = 1e30;
        vec3 hit_point, normal, material_color;
};

// Floor material color based on checkerboard pattern
vec3 floor_color_at(const vec3& point) {
    int grid_x = (int)floor(point.x());
    int grid_z = (int)floor(point.z());

    bool is_light_tile = ((grid_x + grid_z) % 2) == 0;

    if (is_light_tile) {
        return vec3(0.60, 0.58, 0.55);  // light gray
    } else {
        return vec3(0.30, 0.30, 0.33);  // dark gray
    }
}

// FPV CODE COPY
void render(vector<Uint32>& buf, const vec3& eye, double yaw) {
    // camera basis -- once per FRAME, not per-pixel
    double half_w = tan(FOV_DEG * PI / 360.0);
    double half_h = half_w * SCREEN_HEIGHT / SCREEN_WIDTH;   // aspect-corrected
    vec3 forward = vec3(cos(yaw), 0.0, sin(yaw));
    vec3 right   = cross(forward, vec3(0, 1, 0));            // F x UP = right-hand rule

    for (int j = 0; j < SCREEN_HEIGHT; j++) {
        double v = (1.0 - 2.0*(j + 0.5)/SCREEN_HEIGHT) * half_h;
        for (int i = 0; i < SCREEN_WIDTH; i++) {
            double u = (2.0*(i + 0.5)/SCREEN_WIDTH - 1.0) * half_w;
            ray r(eye, unit_vector(forward + u*right + v*vec3(0, 1, 0)));

            vec3 c;
            if (r.direction.y() < -1e-9) {              // aims DOWN -> floor y=0
                double t = -eye.y() / r.direction.y();  // solve eye.y + dir.y*t = 0
                if (t > 1e-6) c = floor_color_at(r.at(t));
                else c = vec3(0.22, 0.22, 0.26);
            } else {
                c = vec3(0.22, 0.22, 0.26);                  // horizontal/up rays → sky
            }

            Uint8 r8 = (Uint8)(c.x()*255.99), g8 = (Uint8)(c.y()*255.99),
                  b8 = (Uint8)(c.z()*255.99);
            buf[(size_t)j*SCREEN_WIDTH + i] = (255u<<24) | (r8<<16) | (g8<<8) | b8;
        }
    }
}




int32_t main() {
    // defining the screen buffer variables
    vector<Uint32> buffer((size_t)SCREEN_WIDTH*SCREEN_HEIGHT);
    Uint32 pitch = SCREEN_WIDTH*sizeof(Uint32);

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

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );
    if (texture == NULL) {
        cout << "Texture creation failed : " << SDL_GetError() << endl;
    }



    // Check if the window was successfully created
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // --- Player state: find maze markers, spawn mid-cell ---
    int start_x = 1, start_z = 1, exit_x = -1, exit_z = -1;
    for (int z = 0; z < MAZE_H; z++)
        for (int x = 0; x < MAZE_W; x++) {
            if (MAP[z][x] == 'S') { start_x = x; start_z = z; }
            if (MAP[z][x] == 'E') { exit_x  = x; exit_z  = z; }
        }
    cout << exit_x << " " << exit_z << endl;
    vec3 pos(start_x + 0.5, EYE_HEIGHT, start_z + 0.5);  // cell center: no gridline ambiguity
    double yaw = 0.0;                                    // facing +x

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

        // --- Render this frame ---
        render(buffer, vec3(pos.x(), EYE_HEIGHT, pos.z()), yaw);

        // --- Present: buffer -> texture -> composed frame -> visible ---
        SDL_UpdateTexture(texture, NULL, buffer.data(), pitch); // CPU pixels -> GPU texture
        SDL_RenderCopy(renderer, texture, NULL, NULL);          // queue full-frame blit
        SDL_RenderPresent(renderer);                            // swap onto the screen
    }

    double end_time = SDL_GetTicks();
    cout << "Total time spent = " << (end_time - start_time)/1000 << " seconds\n";

    // 4. Clean up and free resources
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
