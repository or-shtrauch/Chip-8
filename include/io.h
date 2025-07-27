#ifndef __IO_H__
#define __IO_H__

#include "SDL.h" // IWYU pragma: keep

struct io;
typedef struct io io_t;
typedef int (*io_cycle_handler)(io_t *);

struct io
{
    io_cycle_handler cycle_handler;
    int              scale;
    SDL_Window      *window;
};

typedef enum
{
    IO_OK = 0,
    IO_SDL_INIT_ERROR,
    IO_WINDOW_CREATE_ERROR,
    IO_QUIT,
    IO_MAX, /* must be last one */
} io_error_code_t;

int  io_init(io_t *io, int width, int height);
void io_cleanup(io_t *io);

#endif /* __IO_H__ */