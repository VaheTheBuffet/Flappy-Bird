#ifndef MAIN_H
#define MAIN_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
} Player;

bool Player_get_mesh(Player *player, SDL_FRect *mesh);

typedef struct {
    float x;
    float height;
} Pipe;

bool Pipe_get_mesh(Pipe* self, Player *player, SDL_FRect *mesh_top, SDL_FRect *mesh_bottom);

bool collision(Pipe* pipe, Player* player);

void game_init();
void draw();
bool update(double dt);

#endif
