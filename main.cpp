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
    "#.#...E.#.#....#",
    "#.#######.#.####",
    "#.........#....#",
    "#.#########.##.#",
    "#.#..........#.#",
    "#...#####......#",
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
const double    WALL_H          = 1.0;   // each '#' is a unit cube: z in [0,1], filling one map cell
const double    PLAYER_R        = 0.28;    // player collision radius (corridor is 1.0 wide)

static bool is_wall(int cx, int cy) {
    if (cx < 0 || cy < 0 || cx >= MAZE_W || cy >= MAZE_H) return true;  // outside = solid
    return MAP[cy][cx] == '#';     // rows are the y axis (the ground's second axis)
}

// ---- exit markers and walking distance ----
// 'S'/'E' are found once in main(). The row index is world y in this z-up
// file, so it is named _z here to match the rest of the file's map talk.
static int  start_x = 1, start_z = 1;    // 'S' cell
static int  exit_x  = -1, exit_z = -1;   // 'E' cell
static vector<int> exit_dist;            // BFS distance to the exit cell
static int  max_dist = 1;

// BFS from the exit over floor cells: true walking distance, so a wall
// between you and the exit counts (a straight line would lie). The vector
// doubles as the queue: it grows while h walks it, so no pop_front needed.
static void build_exit_dist() {
    exit_dist.assign(MAZE_W * MAZE_H, -1);
    vector<int> q{exit_z * MAZE_W + exit_x};
    exit_dist[q[0]] = 0;
    for (size_t h = 0; h < q.size(); h++) {
        int c = q[h], cx = c % MAZE_W, cy = c / MAZE_W;
        const int dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
        for (int k = 0; k < 4; k++) {
            int nx = cx + dx[k], ny = cy + dy[k];
            if (is_wall(nx, ny) || exit_dist[ny * MAZE_W + nx] >= 0) continue;
            exit_dist[ny * MAZE_W + nx] = exit_dist[c] + 1;
            q.push_back(ny * MAZE_W + nx);
        }
    }
    for (int d : exit_dist) if (d > max_dist) max_dist = d;
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
        bool emissive = false;      // true = unlit (the exit beacon)
};

// +1.0 = standing on the exit, -1.0 = farthest reachable cell.
static double temperature(const vec3& pos) {
    int d = exit_dist[(int)pos.y() * MAZE_W + (int)pos.x()];  // row = world y
    if (d < 0) d = max_dist;                                  // unreachable = coldest
    return 1.0 - 2.0 * d / max_dist;
}

// Ray vs axis-aligned box, slab method. Only the exit beacon needs this:
// it is smaller than a cell, so the grid DDA cannot represent it.
static bool ray_box(const ray& r, const vec3& bmin, const vec3& bmax,
                    double t0, double t1, double& t_hit, vec3& n_hit) {
    double tmin = t0, tmax = t1;
    int axis = -1;
    double sign = 0.0;
    for (int a = 0; a < 3; a++) {
        double inv = 1.0 / r.direction[a];  // +-inf when the component is 0
        double tn = (bmin[a] - r.origin[a]) * inv;
        double tf = (bmax[a] - r.origin[a]) * inv;
        if (tn > tf) swap(tn, tf);
        if (tn > tmin) { tmin = tn; axis = a; sign = r.direction[a] > 0 ? -1.0 : 1.0; }
        if (tf < tmax) tmax = tf;
        if (tmax <= tmin) return false;
    }
    if (axis < 0) return false;             // ray origin inside the box
    t_hit = tmin;
    n_hit = vec3(axis == 0 ? sign : 0, axis == 1 ? sign : 0, axis == 2 ? sign : 0);
    return true;
}

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

hit_record cast_ray(const ray& r) {
    hit_record best;                        // ray_t = 1e30: "no hit yet"

    // ---- planes (moved verbatim from render(); only the names change) ----
    if (r.direction.z() < -1e-9) {          // aims down -> floor plane z = 0
        double t = -r.origin.z() / r.direction.z();
        if (t > 1e-6) {
            best.ray_t = t;
            best.hit_point = r.at(t);
            best.normal = vec3(0, 0, 1);    // floor faces up
            best.material_color = floor_color_at(r.at(t));
        }
    } else if (r.direction.z() > 1e-9) {    // aims up -> ceiling plane z = WALL_H
        double t = (WALL_H - r.origin.z()) / r.direction.z();
        if (t > 1e-6) {
            best.ray_t = t;
            best.hit_point = r.at(t);
            best.normal = vec3(0, 0, -1);   // ceiling faces down
            best.material_color = vec3(0.22, 0.22, 0.26);
        }
    }
    double t_plane = best.ray_t;            // walls must beat this distance or lose

    // ---- 2D DDA walk: march through maze cells, nearest-face-first ----
    // Start cell: the ray origin's cell on the ground grid.
    int cx = (int)floor(r.origin.x()), cy = (int)floor(r.origin.y());
    // Step signs: which horizontal direction we move per axis.
    int sx = r.direction.x() > 0 ? 1 : -1;
    int sy = r.direction.y() > 0 ? 1 : -1;
    // Cost to cross ONE full cell per axis, in t-units. Ray is unit:
    // filling one cell of x needs |1/dir.x| t-units.
    double dtx = abs(1.0 / r.direction.x());
    double dty = abs(1.0 / r.direction.y());
    // Time of FIRST gridline crossing per axis:
    // (distance from origin to that boundary, in this axis's units) * cost.
    double tx = (sx > 0 ? (cx + 1.0 - r.origin.x()) : (r.origin.x() - cx)) * dtx;
    double ty = (sy > 0 ? (cy + 1.0 - r.origin.y()) : (r.origin.y() - cy)) * dty;

    double t_entry = 0.0;   // when we entered the CURRENT cell
    int entry_axis = -1;    // 0 = came in through an x-face, 1 = through a y-face

    for (int i = 0; i < 256 && t_entry < t_plane; i++) {   // bounded, and can't beat planes
        // Entering a wall cell through a face: that face IS the visible surface.
        // Gate: the cube only spans z in [0, WALL_H] -- check ray altitude at entry.
        if (entry_axis >= 0 && is_wall(cx, cy)) {
            double z_at = r.origin.z() + r.direction.z() * t_entry;
            if (z_at >= 0.0 && z_at <= WALL_H) {
                best.ray_t          = t_entry;
                best.hit_point      = r.at(t_entry);
                best.normal         = (entry_axis == 0) ? vec3(-sx, 0, 0) : vec3(0, -sy, 0);
                best.material_color = vec3(0.55, 0.47, 0.40);   // tan, shading later
                return best;    // first wall hit wins: nearest surface, early exit
            }
        }
        // advance: cross whichever gridline comes FIRST (smaller time),
        // pay one full cell of cost on that axis, remember the entry face.
        if (tx < ty) { t_entry = tx; tx += dtx; cx += sx; entry_axis = 0; }
        else         { t_entry = ty; ty += dty; cy += sy; entry_axis = 1; }
    }

    // Exit beacon: a small glowing cube standing in the exit cell. Tested
    // last, against best.ray_t, so a wall in front of it still wins.
    double t;
    vec3 n;
    vec3 bmin(exit_x + 0.30, exit_z + 0.30, 0.05);   // z = altitude (0 = floor)
    vec3 bmax(exit_x + 0.70, exit_z + 0.70, 0.80);
    if (ray_box(r, bmin, bmax, 1e-6, best.ray_t, t, n)) {
        best.ray_t          = t;
        best.hit_point      = r.at(t);
        best.normal         = n;
        best.material_color = vec3(0.25, 1.0, 0.35);
        best.emissive       = true;
    }
    return best;
}

// Lambertian shading with a torch carried at the eye. No shadow rays:
// the light sits where the camera is, so every visible point is lit.
vec3 shade(const hit_record& h, const vec3& eye) {
    if (h.emissive) return h.material_color;        // beacon glows, no shading
    vec3 to_light = eye - h.hit_point;              // surface -> torch
    double d2 = dot(to_light, to_light);            // squared distance
    double diff = max(dot(h.normal, unit_vector(to_light)), 0.0);  // Lambert cosine
    double atten = 1.0 / (0.9 + 0.30 * d2);         // distance falloff
    vec3 lit = h.material_color * (vec3(0.14, 0.14, 0.18) +          // ambient
                                   vec3(1.7, 1.45, 1.10) * (diff * atten)); // warm torch
    return vec3(sqrt(clamp(lit.x(), 0.0, 1.0)),
                sqrt(clamp(lit.y(), 0.0, 1.0)),
                sqrt(clamp(lit.z(), 0.0, 1.0)));    // gamma 2.0 encode
}

bool collides(const vec3& p) {
    // Broad phase: only cells the sphere's ground footprint can touch.
    int x0 = (int)floor(p.x() - PLAYER_R), x1 = (int)floor(p.x() + PLAYER_R);
    int y0 = (int)floor(p.y() - PLAYER_R), y1 = (int)floor(p.y() + PLAYER_R);

    for (int cy = y0; cy <= y1; cy++) {
        for (int cx = x0; cx <= x1; cx++) {
            if (!is_wall(cx, cy)) continue;

            // Closest point on this cube to the sphere center:
            double qx = clamp(p.x(), (double)cx, cx + 1.0);
            double qy = clamp(p.y(), (double)cy, cy + 1.0);
            double qz = clamp(p.z(), 0.0, WALL_H);

            double dx = p.x() - qx, dy = p.y() - qy, dz = p.z() - qz;
            // Compare squared distance to squared radius: no sqrt.
            if (dx*dx + dy*dy + dz*dz < PLAYER_R * PLAYER_R) return true;
        }
    }
    return false;
}

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
            hit_record h = cast_ray(r);
            vec3 c = shade(h, eye);

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

    // intent: two votes (walk, strafe). Arrow up/down work like W/S.
    double fwd  = (keys[SDL_SCANCODE_W] ? 1.0 : 0.0) - (keys[SDL_SCANCODE_S] ? 1.0 : 0.0)
                + (keys[SDL_SCANCODE_UP] ? 1.0 : 0.0) - (keys[SDL_SCANCODE_DOWN] ? 1.0 : 0.0);
    fwd = clamp(fwd, -1.0, 1.0);
    double side = (keys[SDL_SCANCODE_D] ? 1.0 : 0.0) - (keys[SDL_SCANCODE_A] ? 1.0 : 0.0);

    bool is_fast = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT] || (SDL_GetModState() & KMOD_CAPS);
    double current_speed = is_fast ? (MOVE_SPD * CAPS_MULTIPLIER) : MOVE_SPD;
    vec3 step = (forward * fwd + right * side) * (current_speed * dt);

    // Slide along walls: try each ground axis independently, commit only
    // if the candidate position is free. Blocked x still lets y move.
    vec3 candidate = pos;
    candidate[0] += step.x();
    if (!collides(candidate)) pos[0] = candidate[0];

    candidate = pos;
    candidate[1] += step.y();
    if (!collides(candidate)) pos[1] = candidate[1];
}


int32_t main() {
    // defining the screen buffer variables
    vector<Uint32> buffer((size_t)SCREEN_WIDTH*SCREEN_HEIGHT);
    Uint32 pitch = SCREEN_WIDTH*sizeof(Uint32);

    // 1. Initialize SDL
    // Windows builds define SDL_MAIN_HANDLED, so SDL must be told main() is ready.
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // 2. Create the window
    SDL_Window* window = SDL_CreateWindow(
        "Maze Runner 3D",            // Window title
        SDL_WINDOWPOS_CENTERED,      // Initial x position
        SDL_WINDOWPOS_CENTERED,      // Initial y position
        SCREEN_WIDTH,                // Width, in pixels
        SCREEN_HEIGHT,               // Height, in pixels
        SDL_WINDOW_SHOWN             // Flags (makes it visible)
    );
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, 0);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );
    if (texture == NULL) {
        printf("Texture could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // --- Player state: find maze markers, spawn mid-cell ---
    for (int z = 0; z < MAZE_H; z++)
        for (int x = 0; x < MAZE_W; x++) {
            if (MAP[z][x] == 'S') { start_x = x; start_z = z; }
            if (MAP[z][x] == 'E') { exit_x  = x; exit_z  = z; }
        }
    build_exit_dist();
    vec3 pos(start_x + 0.5, start_z + 0.5, EYE_HEIGHT);  // cell center horizontally, eye mid-height
    double yaw = 0.0;                                    // facing +x

    printf("Find the glowing green exit cube!\n"
           "Controls: WASD or Up/Down move, A/D strafe, Left/Right turn.\n"
           "Hold Shift or CapsLock to sprint %.1fx. Esc quits.\n"
           "Hot/cold score (+1 at exit, -1 farthest) is in the title bar.\n",
           CAPS_MULTIPLIER);

    // 3. Main event loop setup
    SDL_Event event;
    bool quit = false, won = false;
    double start_time = SDL_GetTicks();
    Uint32 last = SDL_GetTicks();   // frame clock: for dt

    // Keep the window open until the user closes it
    while (!quit) {
        // Poll for pending events (clicks, keypresses, window close)
        while (SDL_PollEvent(&event) != 0) {
            // User requests quit (e.g., clicking the 'X' button)
            if (event.type == SDL_QUIT ||
                (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                quit = true;
            }
        }

        // --- advance the world: frame clock, then state update ---
        Uint32 now_frame = SDL_GetTicks();
        double dt = (now_frame - last) / 1000.0;
        last = now_frame;
        if (dt > 0.05) dt = 0.05;   // clamp: a frozen frame can't teleport the player
        update(pos, yaw, dt, SDL_GetKeyboardState(NULL));

        // win when your cell IS the exit cell
        if ((int)floor(pos.x()) == exit_x && (int)floor(pos.y()) == exit_z) {
            won = true;
            quit = true;
        }

        // hot/cold score: +1 = on the exit, -1 = farthest reachable cell
        double temp = temperature(pos);
        const char* feel = temp > 0.5 ? "HOT" : temp > 0.0 ? "WARM"
                         : temp > -0.5 ? "COOL" : "COLD";
        char title[64];
        snprintf(title, sizeof title, "Maze Runner 3D  |  Score %+.2f  %s", temp, feel);
        SDL_SetWindowTitle(window, title);

        // --- Render this frame ---
        render(buffer, vec3(pos.x(), pos.y(), EYE_HEIGHT), yaw);

        // --- Present: buffer -> texture -> composed frame -> visible ---
        SDL_UpdateTexture(texture, NULL, buffer.data(), pitch); // CPU pixels -> GPU texture
        SDL_RenderCopy(renderer, texture, NULL, NULL);          // queue full-frame blit

        // temperature bar: blue (cold) on the left -> red (hot) on the right.
        // Drawn on the renderer AFTER the 3D texture, so it sits on top.
        double f = (temp + 1.0) / 2.0;
        SDL_SetRenderDrawColor(renderer, 20, 24, 30, 255);
        SDL_Rect bg{16, 12, SCREEN_WIDTH - 32, 10};
        SDL_RenderFillRect(renderer, &bg);
        SDL_SetRenderDrawColor(renderer, (Uint8)(60 + 195 * f), (Uint8)(120 - 60 * f),
                               (Uint8)(255 - 215 * f), 255);
        SDL_Rect bar{16, 12, (int)((SCREEN_WIDTH - 32) * f), 10};
        SDL_RenderFillRect(renderer, &bar);

        SDL_RenderPresent(renderer);                            // swap onto the screen

        Uint32 spent = SDL_GetTicks() - now_frame;              // no-vsync guard
        if (spent < 15) SDL_Delay(15 - spent);                  // cap ~60 fps
    }

    double end_time = SDL_GetTicks();
    cout << "Total time spent = " << (end_time - start_time)/1000 << " seconds\n";

    // win screen: paint the window green for a moment, then exit
    if (won) {
        printf("You escaped the maze in %.1f seconds!\n", (end_time - start_time)/1000.0);
        SDL_SetRenderDrawColor(renderer, 40, 200, 90, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(2500);
    }

    // 4. Clean up and free resources
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
