#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "chip8.h"
#include "chip8_handlers.h"

static int chip8_cycle(chip8_t *chip8);

decode_handler handlers[] = {
    [0x0] = chip8_decode_handler_msb_0,
    [0x1] = chip8_decode_handler_msb_1,
    [0x2] = chip8_decode_handler_msb_2,
    [0x3] = chip8_decode_handler_msb_3,
    [0x4] = chip8_decode_handler_msb_4,
    [0x5] = chip8_decode_handler_msb_5,
    [0x6] = chip8_decode_handler_msb_6,
    [0x7] = chip8_decode_handler_msb_7,
    [0x8] = chip8_decode_handler_msb_8,
    [0x9] = chip8_decode_handler_msb_9,
    [0xA] = chip8_decode_handler_msb_A,
    [0xB] = chip8_decode_handler_msb_B,
    [0xC] = chip8_decode_handler_msb_C,
    [0xD] = chip8_decode_handler_msb_D,
    [0xE] = chip8_decode_handler_msb_E,
    [0xF] = chip8_decode_handler_msb_F,
};

static int chip8_load_rom(chip8_t *chip8, const char *rom_file)
{
    FILE *fd = NULL;
    struct stat st;
    int err = CHIP8_OK;

    if (stat(rom_file, &st) != 0)
    {
        return CHIP8_ROM_ERR;
    }

    if (st.st_size > CHIP8_MAX_ROM_SIZE)
    {
        return CHIP8_ROM_TOO_BIG_ERR;
    }

    fd = fopen(rom_file, "r");
    if (fd == NULL)
    {
        return CHIP8_ROM_ERR;
    }

    if (fread(&chip8->memory[CHIP8_ROM_START], 1, st.st_size, fd) != st.st_size)
    {
        err = CHIP8_ROM_READ_ERR;
    }

    fclose(fd);
    return CHIP8_OK;
}

int chip8_init(chip8_t *chip8, const char *rom_file) {
    chip8->program_counter = CHIP8_ROM_START;
    chip8->cycle_handler = chip8_cycle;
    return chip8_load_rom(chip8, rom_file);
}

static int chip8_fetch_decode_execute(chip8_t *chip8)
{
    int err;
    ushort_t command;
    uchar_t opcode;
    int jump = 0;

    /* get the first half of the instruction and left shift it by 8 */
    command = CHIP8_MEM(chip8, chip8->program_counter) << 8;
    /* append the second half of the command */
    command &= CHIP8_MEM(chip8, chip8->program_counter + 1);

    printf("command: %x\n", command);

    /* opcode is the MSB (0xF000) */
    opcode = command & CHIP8_NIBBLE_MASK(4);

    if (opcode < 0x00 || opcode > 0xF0 || !handlers[opcode]) {
        return CHIP8_OPCODE_ERR;
    }

    err = handlers[opcode](chip8, command, opcode);

    if (!jump) {
        chip8->program_counter += 2;
    }

    return err;
}

static int chip8_cycle(chip8_t *chip8) {
    return chip8_fetch_decode_execute(chip8);
}

void chip8_cleanup(chip8_t *chip8) {
}
