#define SDL_MAIN_USE_CALLBACKS 1
#include "main.h"

#define APPLICATION_NAME "game"
#define WIDTH 800
#define HEIGHT 600
#define PLAYER_SPEED 100 
#define PIPE_GAP 200
#define PIPE_WIDTH 100
#define PLAYER_WIDTH 100
#define PIPE_INTERVAL (PIPE_WIDTH + WIDTH) / 2
#define NUM_PIPES 2
#define FIRST_PIPE_X 400.0

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

static double SECOND = 1.0;
static double FPS = 0.0;
static double START_FRAME_T = 0.0;
static double DELTA = 0.0;

//STATE TODO: maybe use the appstate
static Player PLAYER;
static Pipe PIPES[NUM_PIPES];
static SDL_FRect PIPE_MESHES[NUM_PIPES * 2];
static SDL_FRect PLAYER_MESH;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Flappy Bird", "1.0", "com.saucer.flappy-bird");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer(APPLICATION_NAME, WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, WIDTH, HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    game_init();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    START_FRAME_T = SDL_GetTicks();

    SDL_SetRenderDrawColorFloat(renderer, 0.1, 0.2, 0.8, SDL_ALPHA_OPAQUE_FLOAT);
    SDL_RenderClear(renderer);

    if (!update(DELTA))
    {
        printf("You lost!\n");
        return SDL_APP_SUCCESS;
    };
    draw();

    SDL_RenderDebugTextFormat(renderer, WIDTH - 200, 20, "FPS: %f", FPS);

    SDL_RenderPresent(renderer);

    DELTA = ((double)SDL_GetTicks() - START_FRAME_T) / 1000.0;
    SECOND -= DELTA;

    if (SECOND < 0)
    {
        FPS = 1.0 / DELTA;
        SECOND = 1.0;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}

//------------------- GAME LOGIC -----------------------

void game_init() 
{
    PLAYER = (Player){0.0, 0.0, PLAYER_SPEED, -PLAYER_SPEED};
    PIPES[0] = (Pipe){FIRST_PIPE_X, (float)(rand() % (HEIGHT - PIPE_GAP - 5))};
    PIPES[1] = (Pipe){FIRST_PIPE_X + PIPE_INTERVAL, (float)(rand() % (HEIGHT - PIPE_GAP - 5))};
}

//Returns false on game over
bool update(double dt)
{
    for(size_t i = 0; i < NUM_PIPES; i++) 
    {
        if (collision(PIPES + i, &PLAYER)) 
        {
            return false;
        }
    }
    //PIPES[0] is always the earlier pipe
    if (PLAYER.x - PIPES[0].x > PIPE_INTERVAL)
    {
        PIPES[0] = PIPES[1];
        PIPES[1] = (Pipe){
            PIPES[0].x + PIPE_INTERVAL, 
            //5 is an offset so the pipe isn't at the very edge of the screen
            (float)(rand() % (HEIGHT - PIPE_GAP - 5)) 
        };
    }

    const bool *key_states = SDL_GetKeyboardState(NULL);

    if (key_states[SDL_SCANCODE_W]) 
    {
        PLAYER.vy = PLAYER_SPEED;
    }
    if (key_states[SDL_SCANCODE_S]) 
    {
        PLAYER.vy = -PLAYER_SPEED;
    }
    if (key_states[SDL_SCANCODE_A]) 
    {
        PLAYER.vx = -PLAYER_SPEED;
    }
    if (key_states[SDL_SCANCODE_D]) 
    {
        PLAYER.vx = PLAYER_SPEED;
    }

    PLAYER.y -= PLAYER.vy * dt;
    PLAYER.x += PLAYER.vx * dt;
    PLAYER.vy += (-PLAYER_SPEED - PLAYER.vy) * dt;
    PLAYER.vx = 0;

    return true;
}

void draw() 
{
    Player_get_mesh(&PLAYER, &PLAYER_MESH);
    for(size_t i=0; i < NUM_PIPES; i++) 
    {
        Pipe_get_mesh(PIPES + i, &PLAYER, PIPE_MESHES + 2*i, PIPE_MESHES + 2*i + 1);
    }

    SDL_SetRenderDrawColorFloat(renderer, 1.0, 0.0, 0.0, SDL_ALPHA_OPAQUE_FLOAT);
    SDL_RenderFillRect(renderer, &PLAYER_MESH);
    SDL_SetRenderDrawColorFloat(renderer, 0.0, 1.0, 0.0, SDL_ALPHA_OPAQUE_FLOAT);
    for(size_t i=0; i < NUM_PIPES * 2; i++) 
    {
        SDL_RenderFillRect(renderer, PIPE_MESHES + i);
    }
}

bool Player_get_mesh(Player *player, SDL_FRect *mesh) 
{
    //Player is always centered on the screen
    mesh->x = (WIDTH - PLAYER_WIDTH) / 2;
    mesh->y = player->y;
    mesh->w = PLAYER_WIDTH;
    mesh->h = PLAYER_WIDTH;
    return true;
}

bool Pipe_get_mesh(Pipe *pipe, Player *player, SDL_FRect *mesh_top, SDL_FRect *mesh_bottom)
{
    //If the top pipe is so large there isn't enough space to construct the bttom pipe
    //with adequate gap
    if(pipe->height > HEIGHT - PIPE_GAP) 
    {
        return false;
    }

    mesh_top->x = pipe->x - player->x + (WIDTH - PLAYER_WIDTH) / 2;
    mesh_top->y = 0;
    mesh_top->w = PIPE_WIDTH;
    mesh_top->h = pipe->height;

    mesh_bottom->x = pipe->x - player->x + (WIDTH - PLAYER_WIDTH) / 2;
    mesh_bottom->y = pipe->height + PIPE_GAP;
    mesh_bottom->w = PIPE_WIDTH;
    mesh_bottom->h = HEIGHT - pipe->height - PIPE_GAP;

    return true;
}

bool collision(Pipe *pipe, Player *player)
{
    bool x_collision = player->x + PLAYER_WIDTH > pipe->x 
        && player->x < pipe->x + PIPE_WIDTH;

    bool y_collision = player->y < pipe->height 
        || player->y + PLAYER_WIDTH > pipe->height + PIPE_GAP;

    return x_collision && y_collision;
}
