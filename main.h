#ifndef MAIN_H
#define MAIN_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdlib.h>

#define MIN(a, b) ((a < b) ? a : b)
#define MAX(a, b) ((a > b) ? a : b)
#define CLAMP(x, a, b) (MIN(MAX(a, x), b))

#define APPLICATION_NAME "Flappy Bird"
#define WIDTH 800
#define HEIGHT 600
#define HORIZONTAL_SPEED 100 
#define VERTICAL_SPEED 250
#define PIPE_GAP 200
#define PIPE_WIDTH 100
#define BACKGROUND_TEXTURE_WIDTH 512
#define BACKGROUND_PARALAX_RATE 3 // number of screens
#define PLAYER_SPRITE_WIDTH 128
#define PLAYER_WIDTH 100
#define PLAYER_HEIGHT (68 * 100) / PLAYER_SPRITE_WIDTH
#define PIPE_INTERVAL (PIPE_WIDTH + WIDTH) / 2
#define NUM_PIPES 2
#define FIRST_PIPE_X 400.0

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
} Player;

typedef struct {
    float x;
    float height;
} Pipe;

typedef struct {
    SDL_FRect player;
    SDL_FRect pipes[4];
    SDL_FRect background;
    SDL_FRect clouds;
} MeshState;

typedef struct {
    SDL_Texture *test, *bird, *background, *pipe, *clouds;
} TextureState;

typedef struct {
    Player player;
    Pipe pipes[2];
    double fps, second, start_frame_t, delta_t;
    MeshState meshes;
    TextureState textures;
} GameState;

bool load_texture(const char* path, SDL_Texture **tex); 
bool Player_get_mesh(Player *player, SDL_FRect *mesh);
bool Pipe_get_mesh(Pipe* self, Player *player, SDL_FRect *mesh_top, SDL_FRect *mesh_bottom);
bool collision(Pipe* pipe, Player* player);
void game_init(GameState *state);
void draw(GameState *state);
bool update(double dt, GameState* state);

#endif
