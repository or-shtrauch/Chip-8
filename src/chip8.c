#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "chip8.h"

static int            chip8_cycle(chip8_t *chip8);
extern decode_handler handlers[];

static int chip8_load_rom(chip8_t *chip8, const char *rom_file)
{
    FILE       *fd = NULL;
    struct stat st;
    int         err = CHIP8_OK;

    memset(&st, 0, sizeof(struct stat));

    if (stat(rom_file, &st) != 0) {
        return CHIP8_ROM_ERR;
    }

    if (st.st_size > CHIP8_MAX_ROM_SIZE) {
        return CHIP8_ROM_TOO_BIG_ERR;
    }

    fd = fopen(rom_file, "r");
    if (fd == NULL) {
        return CHIP8_ROM_ERR;
    }

    if (fread(&chip8->memory[CHIP8_ROM_START], 1, st.st_size, fd) !=
        (unsigned long)st.st_size) {
        err = CHIP8_ROM_READ_ERR;
    }

    fclose(fd);
    return err;
}

int chip8_init(chip8_t *chip8, const char *rom_file)
{
    srand(time(NULL));
    chip8->program_counter = CHIP8_ROM_START;
    chip8->cycle_handler   = chip8_cycle;
    return chip8_load_rom(chip8, rom_file);
}

static int chip8_fetch_decode_execute(chip8_t *chip8)
{
    int      err     = CHIP8_OK;
    uint16_t command = 0;
    uint8_t  opcode  = 0;
    int      jump    = 0;

    CHIP8_ASSERT_PTR(chip8, CHIP8_INVALID_PTR_ERR);

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

    err = handlers[opcode](chip8, command);

    if (!jump) {
        chip8->program_counter += 2;
    }

    return err;
}

static int chip8_cycle(chip8_t *chip8)
{
    return chip8_fetch_decode_execute(chip8);
}

void chip8_cleanup(chip8_t *chip8)
{
    (void)chip8;
}
