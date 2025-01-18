#include "emulator.h"


int emulator_init(emulator_t *emulator, char *rom_file) {
    int err;

    emulator->rom_file = rom_file;
    err = chip8_init(&emulator->chip8, rom_file);
    if (err != CHIP8_OK) {
        return EMULATOR_CHIP8_INIT_ERR;
    }

    err = io_init(&emulator->io, 640, 320);
    if (err != IO_OK) {
        return EMULATOR_IO_INIT_ERR;
    }

    emulator->quit = 0;

    return EMULATOR_SUCCESS;
}

int emulator_cycle(emulator_t *emulator) {
    int err;

    while (!emulator->quit) {
        err = emulator->chip8.cycle_handler(&emulator->chip8);
        err = emulator->io.cycle_handler(&emulator->io);

        if (err == IO_QUIT) {
            emulator->quit = 1;
        }
    }

    return EMULATOR_SUCCESS;
}

void emulator_cleanup(emulator_t *emulator) {
    chip8_cleanup(&emulator->chip8);
    io_cleanup(&emulator->io);
}