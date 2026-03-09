#define SDL_MAIN_USE_CALLBACKS 1
#include "main.h"

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_AudioStream *astream = NULL;
static GameState GAMESTATE;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Flappy Bird", "1.0", "com.saucer.flappy-bird");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer(APPLICATION_NAME, WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, WIDTH, HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    if(
        !load_texture("./assets/test.png", &GAMESTATE.textures.test)
        || !load_texture("./assets/pipe.png", &GAMESTATE.textures.pipe)
        || !load_texture("./assets/bird.png", &GAMESTATE.textures.bird)
        || !load_texture("./assets/background.png", &GAMESTATE.textures.background)
        || !load_texture("./assets/clouds.png", &GAMESTATE.textures.clouds)
    ) {
        return SDL_APP_FAILURE;
    }

    game_init(&GAMESTATE);
    *appstate = (void*)&GAMESTATE;

    SDL_AudioSpec spec = {
        .format = SDL_AUDIO_F32,
        .channels = 1,
        .freq = 8000,
    };

    astream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, FeedAudio, &GAMESTATE.audio_state);
    if(!astream) {
        SDL_Log("Could not create audio stream: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_ResumeAudioStreamDevice(astream);

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
    GameState* game_state = (GameState *) appstate;
    game_state->start_frame_t = SDL_GetTicks();

    SDL_SetRenderDrawColorFloat(renderer, 0.6, 0.8, 0.99, SDL_ALPHA_OPAQUE_FLOAT);
    SDL_RenderClear(renderer);

    if (!update(game_state->delta_t, game_state)) {
        printf("You lost!\n");
        return SDL_APP_SUCCESS;
    };

    draw(game_state);
    SDL_SetRenderDrawColorFloat(renderer, 0.0, 0.0, 0.0, SDL_ALPHA_OPAQUE_FLOAT);
    SDL_SetRenderScale(renderer, 2.0, 2.0);
    SDL_RenderDebugTextFormat(renderer, 0, 0, "%f FPS", game_state->fps);
    SDL_SetRenderScale(renderer, 4.0, 4.0);
    SDL_RenderDebugTextFormat(renderer, (WIDTH-100) / 4.0, 20, "%d", game_state->score);
    SDL_SetRenderScale(renderer, 1.0, 1.0);
    SDL_RenderPresent(renderer);

    game_state->delta_t = ((double)SDL_GetTicks() - game_state->start_frame_t) / 1000.0;
    game_state->second -= game_state->delta_t;

    if (game_state->second < 0) {
        game_state->fps = 1.0 / game_state->delta_t;
        game_state->second = 1.0;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    GameState *state = (GameState*)appstate;
    SDL_DestroyTexture(state->textures.test);
    SDL_DestroyTexture(state->textures.pipe);
    SDL_DestroyTexture(state->textures.bird);
    SDL_DestroyTexture(state->textures.background);
}

bool load_texture(const char* path, SDL_Texture **tex) 
{
    SDL_Surface *tex_data = SDL_LoadPNG(path);
    if (!tex_data) {
        printf("could not load texture file");
        return false;
    }
    
    *tex = SDL_CreateTextureFromSurface(renderer, tex_data);
    if (!*tex) {
        printf("could not create texture");
        return false;
    }

    SDL_DestroySurface(tex_data);
    return true;
}

//--------------------- GAME LOGIC --------------------------//

void game_init(GameState *state) 
{
    state->second = 1.0;
    state->fps = 0.0;
    state->start_frame_t = 0.0;
    state->delta_t = 0.0;
    state->player = (Player){0.0, 0.0, HORIZONTAL_SPEED, -VERTICAL_SPEED};
    state->pipes[0] = (Pipe){FIRST_PIPE_X, (float)(rand() % (HEIGHT - PIPE_GAP - 5))};
    state->pipes[1] = (Pipe){FIRST_PIPE_X + PIPE_INTERVAL, (float)(rand() % (HEIGHT - PIPE_GAP - 5))};

    state->meshes.background = (SDL_FRect) {
        0.0, 0.0,
        WIDTH, HEIGHT
    };

    state->meshes.clouds = (SDL_FRect) {
        0.0, 0.0,
        WIDTH, HEIGHT / 2
    };

    state->audio_state.current_sine_sample = 0;
}

//Returns false on game over
bool update(double dt, GameState *state) {
    for(size_t i = 0; i < NUM_PIPES; i++) {
        if (collision(state->pipes + i, &state->player)) {
            return false;
        }
    }

    if (state->player.x > state->pipes[0].x && state->pipes[0].scored == false) {
        state->audio_state.should_sound = true;
        state->score++;
        state->pipes[0].scored = true;
    }
    //PIPES[0] is always the earlier pipe
    if (state->player.x - state->pipes[0].x > PIPE_INTERVAL) {
        state->pipes[0] = state->pipes[1];
        state->pipes[1] = (Pipe){
            state->pipes[0].x + PIPE_INTERVAL, 
            //5 is an offset so the pipe isn't at the very edge of the screen
            (float)(rand() % (HEIGHT - PIPE_GAP - 5)),
            false
        };
    }

    const bool *key_states = SDL_GetKeyboardState(NULL);

    if (key_states[SDL_SCANCODE_W]) {
        state->player.vy = VERTICAL_SPEED;
    }
    if (key_states[SDL_SCANCODE_S]) {
        state->player.vy = -VERTICAL_SPEED;
    }
    if (key_states[SDL_SCANCODE_A]) {
        state->player.vx = -HORIZONTAL_SPEED;
    }
    if (key_states[SDL_SCANCODE_D]) {
        state->player.vx = HORIZONTAL_SPEED;
    }

    state->player.y -= state->player.vy * dt;
    state->player.x += state->player.vx * dt;
    state->player.vy += (-VERTICAL_SPEED - state->player.vy) * dt * 3;
    state->player.vx = 0;

    return true;
}

void draw(GameState *state) 
{
    SDL_RenderTexture(
        renderer, 
        state->textures.background, 
        &(SDL_FRect) {
            (int)state->player.x % state->textures.background->w/2, 
            0.0, 
            state->textures.background->w / 3, 
            state->textures.background->h}, 
        &state->meshes.background);

    SDL_RenderTexture(
        renderer, 
        state->textures.clouds, 
        &(SDL_FRect) {
            (int)state->player.x % state->textures.background->w/2, 
            0.0, 
            state->textures.background->w / 4, 
            state->textures.background->h}, 
        &state->meshes.clouds);

    Player_get_mesh(&state->player, &state->meshes.player);
    for(size_t i=0; i < NUM_PIPES; i++) {
        Pipe_get_mesh(
            state->pipes + i, 
            &state->player, 
            state->meshes.pipes + 2*i, 
            state->meshes.pipes + 2*i + 1);
    }

    SDL_RenderTextureRotated(
            renderer, 
            state->textures.bird, 
            NULL, 
            &state->meshes.player,
            CLAMP(-state->player.vy / 8, -90, 90),
            NULL,
            SDL_FLIP_NONE);

    for(size_t i=0; i < NUM_PIPES * 2; i++) {
        SDL_RenderTextureRotated(
            renderer, 
            state->textures.pipe, 
            NULL,
            state->meshes.pipes + i,
            0,
            NULL,
            !(i&1) ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE);
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
    if(pipe->height > HEIGHT - PIPE_GAP) {
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
        || player->y + PLAYER_HEIGHT > pipe->height + PIPE_GAP;

    return x_collision && y_collision;
}

static void SDLCALL FeedAudio(void *userdata, SDL_AudioStream *astream, int additional_ammount, int total_ammount)
{
    AudioState *state = (AudioState*)userdata;

    if (!state->should_sound)  {
        return;
    }
    state->should_sound = false;

    additional_ammount /= sizeof(float);
    while(additional_ammount > 0) {
        float samples[1536];
        const int total = MAX(additional_ammount, LEN(samples));

        for(int i=0; i<total; i++) {
            const int freq = 1000;
            const float phase = state->current_sine_sample++ * freq / 8000.0f;
            samples[i] = SDL_sinf(phase * 2 * SDL_PI_F) + SDL_sinf(phase * SDL_PI_F);
        }

        state->current_sine_sample %= 8000;
        SDL_PutAudioStreamData(astream, samples, total * sizeof(float));
        additional_ammount -= total;
    }
}
