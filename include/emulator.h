#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#include <stdbool.h>

#include "chip8.h"
#include "io.h"

typedef struct
{
    chip8_t chip8;
    io_t    io;
    char   *rom_file;
    bool    shutdown;
} emulator_t;

typedef enum
{
    EMULATOR_SUCCESS = 0,
    EMULATOR_ERR,
    EMULATOR_CHIP8_INIT_ERR,
    EMULATOR_IO_INIT_ERR,
} emulator_error_t;

int emulator_init(emulator_t *emulator, char *rom_file);

int emulator_cycle(emulator_t *emulator);

void emulator_cleanup(emulator_t *emulator);

void emulator_signal_shutdown(emulator_t *emulator);

#endif /* __EMULATOR_H__ */