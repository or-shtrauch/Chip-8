#include <string.h>
#include <stdlib.h>

#include "chip8.h"

static int chip8_decode_handler_msb_0(chip8_t *chip8, uint16_t command)
{
    CHIP8_ASSERT_PTR(chip8, CHIP8_INVALID_PTR_ERR);

    /**
     * 00E0 - CLS
     * Clear the display.
     */
    if (command == 0x00E0) {
        memset(chip8->display, 0, sizeof(chip8->display));
    }
    /**
     * 00EE - RET
     * Return from a subroutine.
     * The interpreter sets the program counter to the address at the top of the
     * stack, then subtracts 1 from the stack pointer.
     */
    else if (command == 0x00EE) {
        CHIP8_ASSERT_SP_VALID(chip8, CHIP8_INVALID_STACK_PTR_ERR);

        chip8->program_counter = chip8->stack[chip8->stack_pointer];
        chip8->stack_pointer--;
    }

    return CHIP8_OK;
}

static int chip8_decode_handler_msb_1(chip8_t *chip8, uint16_t command)
{
    CHIP8_ASSERT_PTR(chip8, CHIP8_INVALID_PTR_ERR);

    /**
     * 1nnn - JP addr
     * Jump to location nnn.

     * The interpreter sets the program counter to nnn.
    */
    chip8->program_counter = command & CHIP8_LSB_MASK(3);
    return CHIP8_OK;
}

static int chip8_decode_handler_msb_2(chip8_t *chip8, uint16_t command)
{
    CHIP8_ASSERT_PTR(chip8, CHIP8_INVALID_PTR_ERR);

    /**
     * 2nnn - CALL addr
     * Call subroutine at nnn.

     * The interpreter increments the stack pointer,
     * then puts the current PC on the top of the stack.
     * The PC is then set to nnn.
     */
    chip8->stack_pointer++;
    CHIP8_ASSERT_SP_VALID(chip8, CHIP8_INVALID_STACK_PTR_ERR);

    CHIP8_STACK_TOP(chip8) = chip8->program_counter;
    chip8->program_counter = command & CHIP8_LSB_MASK(3);

    return CHIP8_OK;
}

static int chip8_decode_handler_msb_3(chip8_t *chip8, uint16_t command)
{
    uint8_t x, kk;
    CHIP8_ASSERT_PTR(chip8, CHIP8_INVALID_PTR_ERR);

    /**
     * 3xkk - SE Vx, byte
     * Skip next instruction if Vx = kk.

     * The interpreter compares register Vx to kk,
     * and if they are equal, increments the program counter by 2.
      */

    kk = command & CHIP8_LSB_MASK(2);
    x  = CHIP8_NIBBLE(command, 3);

    CHIP8_ASSERT_VALID_REGISTER(chip8, x, CHIP8_INVALID_REGISTER_ERR);

    if (CHIP8_Vx(chip8, x) == kk) {
        chip8->program_counter += 2;
    }

    return CHIP8_OK;
}

static int chip8_decode_handler_msb_4(chip8_t *chip8, uint16_t command)
{
    uint8_t x, kk;
    CHIP8_ASSERT_PTR(chip8, CHIP8_INVALID_PTR_ERR);

    /**
     * 4xkk - SNE Vx, byte
     * Skip next instruction if Vx != kk.

     * The interpreter compares register Vx to kk,
     * and if they are not equal, increments the program counter by 2.
     */

    kk = command & CHIP8_LSB_MASK(2);
    x  = CHIP8_NIBBLE(command, 3);

    CHIP8_ASSERT_VALID_REGISTER(chip8, x, CHIP8_INVALID_REGISTER_ERR);

    if (CHIP8_Vx(chip8, x) != kk) {
        chip8->program_counter += 2;
    }

    return CHIP8_OK;
}

static int chip8_decode_handler_msb_5(chip8_t *chip8, uint16_t command)
{
    uint8_t x, y;
    CHIP8_ASSERT_PTR(chip8, CHIP8_INVALID_PTR_ERR);

    /**
     * 5xy0 - SE Vx, Vy
     * Skip next instruction if Vx = Vy.

     * The interpreter compares register Vx to register Vy,
     * and if they are equal, increments the program counter by 2.
     */

    y = CHIP8_NIBBLE(command, 2);
    x = CHIP8_NIBBLE(command, 3);

    CHIP8_ASSERT_VALID_REGISTER(chip8, x, CHIP8_INVALID_REGISTER_ERR);
    CHIP8_ASSERT_VALID_REGISTER(chip8, y, CHIP8_INVALID_REGISTER_ERR);

    if (CHIP8_Vx(chip8, x) != CHIP8_Vx(chip8, y)) {
        chip8->program_counter += 2;
    }

    return CHIP8_OK;
}

static int chip8_decode_handler_msb_6(chip8_t *chip8, uint16_t command)
{
    uint8_t x, kk;

    CHIP8_ASSERT_PTR(chip8, CHIP8_INVALID_PTR_ERR);

    /**
     * 6xkk - LD Vx, byte
     * Set Vx = kk.

     * The interpreter puts the value kk into register Vx.
     */

    kk = command & CHIP8_LSB_MASK(2);
    x  = CHIP8_NIBBLE(command, 3);

    CHIP8_ASSERT_VALID_REGISTER(chip8, x, CHIP8_INVALID_REGISTER_ERR);

    CHIP8_Vx(chip8, x) = kk;

    return CHIP8_OK;
}

static int chip8_decode_handler_msb_7(chip8_t *chip8, uint16_t command)
{
    uint8_t x, kk;

    CHIP8_ASSERT_PTR(chip8, CHIP8_INVALID_PTR_ERR);

    /**
     * 7xkk - ADD Vx, byte
     * Set Vx = Vx + kk.

     * Adds the value kk to the value of register Vx,
     * then stores the result in Vx.
     */

    kk = command & CHIP8_LSB_MASK(2);
    x  = CHIP8_NIBBLE(command, 3);

    CHIP8_ASSERT_VALID_REGISTER(chip8, x, CHIP8_INVALID_REGISTER_ERR);

    CHIP8_Vx(chip8, x) += kk;

    return CHIP8_OK;
}

static int chip8_decode_handler_msb_8(chip8_t *chip8, uint16_t command)
{
    uint8_t  x, y, lsb;
    uint32_t add;

    CHIP8_ASSERT_PTR(chip8, CHIP8_INVALID_PTR_ERR);

    x   = CHIP8_NIBBLE(command, 3);
    y   = CHIP8_NIBBLE(command, 2);
    lsb = CHIP8_NIBBLE(command, 1);

    CHIP8_ASSERT_VALID_REGISTER(chip8, x, CHIP8_INVALID_REGISTER_ERR);
    CHIP8_ASSERT_VALID_REGISTER(chip8, y, CHIP8_INVALID_REGISTER_ERR);

    switch (lsb) {
    /**
     * 8xy0 - LD Vx, Vy
     * Set Vx = Vy.

     * Stores the value of register Vy in register Vx.
     */
    case 0x0:
        CHIP8_Vx(chip8, x) = CHIP8_Vx(chip8, y);
        break;

    /**
     * 8xy1 - OR Vx, Vy
     * Set Vx = Vx OR Vy.

     * Performs a bitwise OR on the values of Vx and Vy,
     * then stores the result in Vx.
     * A bitwise OR compares the corresponding bits from two values,
     * and if either bit is 1, then the same bit in the result is also 1.
     * Otherwise, it is 0.
     */
    case 0x1:
        CHIP8_Vx(chip8, x) = CHIP8_Vx(chip8, x) | CHIP8_Vx(chip8, y);
        break;

    /**
     * 8xy2 - AND Vx, Vy
     * Set Vx = Vx AND Vy.

     * Performs a bitwise AND on the values of Vx and Vy,
     * then stores the result in Vx.
     * A bitwise AND compares the corresponding bits from two values,
     * and if both bits are 1, then the same bit in the result is also 1.
     * Otherwise, it is 0.
     */
    case 0x2:
        CHIP8_Vx(chip8, x) = CHIP8_Vx(chip8, x) & CHIP8_Vx(chip8, y);
        break;

    /**
     * 8xy3 - XOR Vx, Vy
     * Set Vx = Vx XOR Vy.

     * Performs a bitwise exclusive OR on the values of Vx and Vy,
     * then stores the result in Vx.
     * An exclusive OR compares the corresponding bits from two values,
     * and if the bits are not both the same,
     * then the corresponding bit in the result is set to 1.
     * Otherwise, it is 0.
     */
    case 0x3:
        CHIP8_Vx(chip8, x) = CHIP8_Vx(chip8, x) ^ CHIP8_Vx(chip8, y);
        break;

    /**
     * 8xy4 - ADD Vx, Vy
     * Set Vx = Vx + Vy, set VF = carry.

     * The values of Vx and Vy are added together.
     * If the result is greater than 8 bits (i.e., > 255,) VF is set to 1,
     * otherwise 0. Only the lowest 8 bits of the result are kept,
     * and stored in Vx.
     */
    case 0x4:
        add = CHIP8_Vx(chip8, x) + CHIP8_Vx(chip8, y);

        CHIP8_VF(chip8)    = add > 255;
        CHIP8_Vx(chip8, x) = add & CHIP8_LOWER_8_BITS_MASK;
        break;

    /**
     * 8xy5 - SUB Vx, Vy
     * Set Vx = Vx - Vy, set VF = NOT borrow.

     * If Vx > Vy, then VF is set to 1, otherwise 0.
     * Then Vy is subtracted from Vx,
     * and the results stored in Vx.
     */
    case 0x5:
        CHIP8_VF(chip8)    = CHIP8_Vx(chip8, x) > CHIP8_Vx(chip8, y);
        CHIP8_Vx(chip8, x) = CHIP8_Vx(chip8, x) - CHIP8_Vx(chip8, y);
        break;

    /**
     * 8xy6 - SHR Vx {, Vy}
     * Set Vx = Vx SHR 1.

     * If the least-significant bit of Vx is 1,
     * then VF is set to 1, otherwise 0.
     * Then Vx is divided by 2.
     */
    case 0x6:
        CHIP8_VF(chip8)    = CHIP8_Vx(chip8, x) & 0x01;
        CHIP8_Vx(chip8, x) = CHIP8_Vx(chip8, x) >> 1;
        break;

    /**
     * 8xy7 - SUBN Vx, Vy
     * Set Vx = Vy - Vx, set VF = NOT borrow.

     * If Vy > Vx, then VF is set to 1,
     * otherwise 0. Then Vx is subtracted from Vy,
     * and the results stored in Vx.
     */
    case 0x7:
        CHIP8_VF(chip8)    = CHIP8_Vx(chip8, y) > CHIP8_Vx(chip8, x);
        CHIP8_Vx(chip8, x) = CHIP8_Vx(chip8, y) - CHIP8_Vx(chip8, x);
        break;

    /**
     * 8xyE - SHL Vx {, Vy}
     * Set Vx = Vx SHL 1.

     * If the most-significant bit of Vx is 1,
     * then VF is set to 1, otherwise to 0.
     * Then Vx is multiplied by 2.
     */
    case 0xE:
        CHIP8_VF(chip8) = CHIP8_Vx(chip8, x) >> 7;
        CHIP8_Vx(chip8, x) <<= 1;
        break;

    default:
        break;
    }

    return CHIP8_OK;
}

static int chip8_decode_handler_msb_9(chip8_t *chip8, uint16_t command)
{
    uint8_t x, y;

    CHIP8_ASSERT_PTR(chip8, CHIP8_INVALID_PTR_ERR);

    /**
     * 9xy0 - SNE Vx, Vy
     * Skip next instruction if Vx != Vy.

    * The values of Vx and Vy are compared,
    * and if they are not equal, the program counter is increased by 2.
    */

    y = CHIP8_NIBBLE(command, 2);
    x = CHIP8_NIBBLE(command, 3);

    CHIP8_ASSERT_VALID_REGISTER(chip8, x, CHIP8_INVALID_REGISTER_ERR);
    CHIP8_ASSERT_VALID_REGISTER(chip8, y, CHIP8_INVALID_REGISTER_ERR);

    if (CHIP8_Vx(chip8, x) != CHIP8_Vx(chip8, y)) {
        chip8->program_counter += 2;
    }

    return CHIP8_OK;
}

static int chip8_decode_handler_msb_A(chip8_t *chip8, uint16_t command)
{
    CHIP8_ASSERT_PTR(chip8, CHIP8_INVALID_PTR_ERR);

    /**
     * Annn - LD I, addr
     * Set I = nnn.

     * The value of register I is set to nnn.
     */
    chip8->i_register = command & CHIP8_LSB_MASK(3);
    return CHIP8_OK;
}

static int chip8_decode_handler_msb_B(chip8_t *chip8, uint16_t command)
{
    CHIP8_ASSERT_PTR(chip8, CHIP8_INVALID_PTR_ERR);

    /**
     * Bnnn - JP V0, addr
     * Jump to location nnn + V0.

     * The program counter is set to nnn plus the value of V0.
    */
    chip8->program_counter = CHIP8_V0(chip8) + (command & CHIP8_LSB_MASK(3));
    return CHIP8_OK;
}

static int chip8_decode_handler_msb_C(chip8_t *chip8, uint16_t command)
{
    uint8_t x, kk;

    CHIP8_ASSERT_PTR(chip8, CHIP8_INVALID_PTR_ERR);

    /**
     * Cxkk - RND Vx, byte
     * Set Vx = random byte AND kk.

     * The interpreter generates a random number from 0 to 255,
     * which is then ANDed with the value kk.
     * The results are stored in Vx.
     * See instruction 8xy2 for more information on AND.
     */

    kk = command & CHIP8_LSB_MASK(2);
    x  = CHIP8_NIBBLE(command, 3);

    CHIP8_ASSERT_VALID_REGISTER(chip8, x, CHIP8_INVALID_REGISTER_ERR);

    CHIP8_Vx(chip8, x) = (rand() % 256) & kk;

    return CHIP8_OK;
}

static int chip8_decode_handler_msb_D(chip8_t *chip8, uint16_t command)
{
    uint8_t x, y, n, x_pos, y_pos, pixel;

    CHIP8_ASSERT_PTR(chip8, CHIP8_INVALID_PTR_ERR);

    /**
     * Dxyn - DRW Vx, Vy, nibble
     * Display n-byte sprite starting at memory location I at (Vx, Vy), set VF =
     collision.

     * The interpreter reads n bytes from memory, starting at the address stored
     in I.
     * These bytes are then displayed as sprites on screen at coordinates (Vx,
     Vy).
     * Sprites are XORed onto the existing screen.
     * If this causes any pixels to be erased, VF is set to 1,
     * otherwise it is set to 0.
     * If the sprite is positioned so part of it is outside the coordinates of
     the display,
     * it wraps around to the opposite side of the screen.
     * See instruction 8xy3 for more information on XOR,
     * and section 2.4, Display, for more information on the Chip-8 screen and
     sprites.
     */

    x = CHIP8_NIBBLE(command, 3);
    y = CHIP8_NIBBLE(command, 2);
    n = CHIP8_NIBBLE(command, 1);

    CHIP8_ASSERT_VALID_REGISTER(chip8, x, CHIP8_INVALID_REGISTER_ERR);
    CHIP8_ASSERT_VALID_REGISTER(chip8, y, CHIP8_INVALID_REGISTER_ERR);

    x_pos = CHIP8_Vx(chip8, x);
    y_pos = CHIP8_Vx(chip8, y);

    CHIP8_VF(chip8) = 0;

    for (int y_line = 0; y_line < n; y_line++) {
        pixel = chip8->memory[chip8->i_register + y_line];
        for (int x_line = 0; x_line < 8; x_line++) {
            if ((pixel & (0x80 >> x_line)) != 0) {
                uint8_t screen_x = (x_pos + x_line) % CHIP8_DISPLAY_WIDTH;
                uint8_t screen_y = (y_pos + y_line) % CHIP8_DISPLAY_HEIGHT;

                if (chip8->display[screen_y][screen_x] == 1) {
                    CHIP8_VF(chip8) = 1;
                }
                chip8->display[screen_y][screen_x] ^= 1;
            }
        }
    }

    chip8->draw = 1;
    chip8->program_counter += 2;

    return CHIP8_OK;
}

static int chip8_decode_handler_msb_E(chip8_t *chip8, uint16_t command)
{
    uint8_t x;

    CHIP8_ASSERT_PTR(chip8, CHIP8_INVALID_PTR_ERR);

    x = CHIP8_NIBBLE(command, 3);

    CHIP8_ASSERT_VALID_REGISTER(chip8, x, CHIP8_INVALID_REGISTER_ERR);
    CHIP8_ASSERT_VALID_KEY(chip8, chip8->registers[x], CHIP8_INVALID_KEY_ERR);

    /**
     * Ex9E - SKP Vx
     * Skip next instruction if key with the value of Vx is pressed.

     * Checks the keyboard,
     * and if the key corresponding to the value of Vx is currently in the down
     position,
     * PC is increased by 2.
     */
    if ((command & CHIP8_LSB_MASK(2)) == 0x009E) {
        if (chip8->keypad_state[CHIP8_Vx(chip8, x)] == CHIP8_KEY_PRESSED) {
            chip8->program_counter += 2;
        }
    }
    /**
     * ExA1 - SKNP Vx
     * Skip next instruction if key with the value of Vx is not pressed.

     * Checks the
     */
    else if ((command & CHIP8_LSB_MASK(2)) == 0x00A1) {
        if (chip8->keypad_state[CHIP8_Vx(chip8, x)] != CHIP8_KEY_IDLE) {
            chip8->program_counter += 2;
        }
    }

    return CHIP8_OK;
}

static int chip8_decode_handler_msb_F(chip8_t *chip8, uint16_t command)
{
    CHIP8_ASSERT_PTR(chip8, CHIP8_INVALID_PTR_ERR);
    (void)command;

    /**
     * Fx07 - LD Vx, DT
     * Set Vx = delay timer value.

     * The value of DT is placed into Vx.


     * Fx0A - LD Vx, K
     * Wait for a key press, store the value of the key in Vx.

     * All execution stops until a key is pressed, then the value of that key is
     stored in Vx.


     * Fx15 - LD DT, Vx
     * Set delay timer = Vx.

     * DT is set equal to the value of Vx.


     * Fx18 - LD ST, Vx
     * Set sound timer = Vx.

     * ST is set equal to the value of Vx.


     * Fx1E - ADD I, Vx
     * Set I = I + Vx.

     * The values of I and Vx are added, and the results are stored in I.


     * Fx29 - LD F, Vx
     * Set I = location of sprite for digit Vx.

     * The value of I is set to the location for the hexadecimal sprite
     corresponding to the value of Vx. See section 2.4, Display, for more
     information on the Chip-8 hexadecimal font.


     * Fx33 - LD B, Vx
     * Store BCD representation of Vx in memory locations I, I+1, and I+2.

     * The interpreter takes the decimal value of Vx, and places the hundreds
     digit in memory at location in I, the tens digit at location I+1, and the
     ones digit at location I+2.


     * Fx55 - LD [I], Vx
     * Store registers V0 through Vx in memory starting at location I.

     * The interpreter copies the values of registers V0 through Vx into memory,
     starting at the address in I.


     * Fx65 - LD Vx, [I]
     * Read registers V0 through Vx from memory starting at location I.

     * The interpreter reads values from memory starting at location I into
     registers V0 through Vx.
     */

    return CHIP8_OK;
}

decode_handler handlers[] = {
    [0x0] = chip8_decode_handler_msb_0, [0x1] = chip8_decode_handler_msb_1,
    [0x2] = chip8_decode_handler_msb_2, [0x3] = chip8_decode_handler_msb_3,
    [0x4] = chip8_decode_handler_msb_4, [0x5] = chip8_decode_handler_msb_5,
    [0x6] = chip8_decode_handler_msb_6, [0x7] = chip8_decode_handler_msb_7,
    [0x8] = chip8_decode_handler_msb_8, [0x9] = chip8_decode_handler_msb_9,
    [0xA] = chip8_decode_handler_msb_A, [0xB] = chip8_decode_handler_msb_B,
    [0xC] = chip8_decode_handler_msb_C, [0xD] = chip8_decode_handler_msb_D,
    [0xE] = chip8_decode_handler_msb_E, [0xF] = chip8_decode_handler_msb_F,
};
