#include "io.h"

static int io_cycle(io_t *io);

int io_init(io_t *io, int width, int height)
{
    if (!io || width <= 0 || height <= 0) {
        return IO_SDL_INIT_ERROR;
    }


    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return IO_SDL_INIT_ERROR;
    }

    io->window = SDL_CreateWindow("Chip-8 Emulator", 0, 0, width, height,
                                  SDL_WINDOW_SHOWN);

    if (io->window == NULL) {
        return IO_WINDOW_CREATE_ERROR;
    }

    io->cycle_handler = io_cycle;

    return IO_OK;
}

static int io_cycle(io_t *io)
{
    SDL_Event event;

    (void)io;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return IO_QUIT;
        }

        SDL_Delay(1000);
    }

    return IO_OK;
}

void io_cleanup(io_t *io)
{
    SDL_DestroyWindow(io->window);
    SDL_Quit();
}
