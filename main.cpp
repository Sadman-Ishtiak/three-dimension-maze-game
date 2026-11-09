#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_scancode.h>
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
// World convention (declared once, per the ceiling rule): paper axes x/y =
// ground plane ("xy" = the map you'd draw on paper); z = height/gravity.
// Everything below reads with that meaning: floor plane z = 0, eye at z = EYE_HEIGHT.
const double    EYE_HEIGHT      = 0.5;   // camera height: middle of a 1-unit wall
const double    FOV_DEG         = 75.0;
const double    MOVE_SPD        = 2.0;   // units per second (ground speed)
const double    TURN_SPD        = 2.5;   // radians per second (turning speed)
const double    CAPS_MULTIPLIER = 2.5;
const double WALL_H = 1.0;   // each '#' is a unit cube: z in [0,1], filling one map cell

static bool is_wall(int cx, int cy) {
    if (cx < 0 || cy < 0 || cx >= MAZE_W || cy >= MAZE_H) return true;  // outside = solid
    return MAP[cy][cx] == '#';     // rows are the y axis (the ground's second axis)
}

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

// Floor material: checkerboard over the GROUND plane (x, y). Pattern keys off
// the two in-plane coordinates; z is altitude here and never varies on the floor.
vec3 floor_color_at(const vec3& point) {
    int grid_x = (int)floor(point.x());
    int grid_y = (int)floor(point.y());

    bool is_light_tile = ((grid_x + grid_y) % 2) == 0;

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
    vec3 forward = vec3(cos(yaw), sin(yaw), 0.0);   // forward: flat along ground plane
    vec3 right   = cross(forward, vec3(0, 0, 1));  // F x UP(z) = right-hand rule
    // NOTE: c.y() below is the GREEN channel of a color -- harmless shadowing of
    // the altitude axis; the two live in different structs.

    for (int j = 0; j < SCREEN_HEIGHT; j++) {
        double v = (1.0 - 2.0*(j + 0.5)/SCREEN_HEIGHT) * half_h;
        for (int i = 0; i < SCREEN_WIDTH; i++) {
            double u = (2.0*(i + 0.5)/SCREEN_WIDTH - 1.0) * half_w;
            ray r(eye, unit_vector(forward + u*right + v*vec3(0, 0, 1)));

            vec3 c;
            if (r.direction.z() < -1e-9) {              // aims DOWN -> floor plane z = 0
                double t = -eye.z() / r.direction.z();  // solve eye.z + dir.z*t = 0
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

// advance the player state by one frame's worth of input
// mutates only pos.x/y and yaw; z (altitude) is constant in this game
void update(vec3& pos, double& yaw, double dt, const Uint8* keys) {
    // turning: left/right votes -> continuous intent * TURN_SPD * dt
    yaw += ((keys[SDL_SCANCODE_LEFT] ? 1.0 : 0.0) -
            (keys[SDL_SCANCODE_RIGHT]  ? 1.0 : 0.0)) * TURN_SPD * dt;

    // frame basis, computed fresh here (never shared with render())
    vec3 forward = vec3(cos(yaw), sin(yaw), 0.0);   // flat in ground x/y
    vec3 right   = cross(forward, vec3(0, 0, 1));   // UP is +z (z-up convention)

    // intent: two votes (walk, strafe)
    double fwd  = (keys[SDL_SCANCODE_W] ? 1.0 : 0.0) - (keys[SDL_SCANCODE_S] ? 1.0 : 0.0);
    double side = (keys[SDL_SCANCODE_D] ? 1.0 : 0.0) - (keys[SDL_SCANCODE_A] ? 1.0 : 0.0);

    bool is_fast = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT] || (SDL_GetModState() & KMOD_CAPS);
    double current_speed = is_fast ? (MOVE_SPD * CAPS_MULTIPLIER) : MOVE_SPD;
    vec3 step = (forward * fwd + right * side) * (current_speed * dt);

    // commit, per-axis. Once walls exist, each line becomes:
    //   if (!collides(candidate)) pos.axis += delta;   // slide along walls
    pos[0] += step.x();   // via the & operator[]: x() returns a value copy
    pos[1] += step.y();   // and cannot be written through

    // win check, armed later once E is visibly in the world:
    // if ((int)floor(pos.x()) == exit_x && (int)floor(pos.y()) == exit_y) ...
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
    vec3 pos(start_x + 0.5, start_z + 0.5, EYE_HEIGHT);  // cell center horizontally, eye mid-height
    double yaw = 0.0;                                    // facing +x

    // 3. Main event loop setup
    SDL_Event event;
    bool quit = false;
    double start_time = SDL_GetTicks();
    Uint32 last = SDL_GetTicks();   // frame clock: for dt

    // Keep the window open until the user closes it
    while (!quit) {
        // Poll for pending events (clicks, keypresses, window close)
        while (SDL_PollEvent(&event) != 0) {
            // User requests quit (e.g., clicking the 'X' button)
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }

        // --- advance the world: frame clock, then state update ---
        Uint32 now_frame = SDL_GetTicks();
        double dt = (now_frame - last) / 1000.0;
        last = now_frame;
        if (dt > 0.05) dt = 0.05;   // clamp: a frozen frame can't teleport the player
        update(pos, yaw, dt, SDL_GetKeyboardState(NULL));

        // --- Render this frame ---
        render(buffer, vec3(pos.x(), pos.y(), EYE_HEIGHT), yaw);

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
