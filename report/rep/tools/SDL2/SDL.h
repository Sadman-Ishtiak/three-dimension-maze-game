#pragma once
#include <cstdint>
typedef uint32_t Uint32; typedef uint8_t Uint8;
enum { SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN,
       SDL_SCANCODE_D, SDL_SCANCODE_A, SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RSHIFT, SDL_NUM };
#define KMOD_CAPS 0x2000
inline int SDL_GetModState(){return 0;}
