#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "chip8_handlers.c"
#include "chip8.c"

void __wrap_srand(unsigned int seed)
{
    printf("wrap srand");
};

time_t __wrap_time(time_t *tloc)
{
    time_t t;
    printf("wrap time");
    return t;
}

static void test_chip8_decode_handlers_null_ptr(void **state)
{
    int i;
    int err;

    for (i = 0; i < sizeof(handlers) / sizeof(handlers[0]); i++) {
        err = handlers[i](NULL, 0, 0);
        assert_int_equal(err, CHIP8_INVALID_PTR_ERR);
    }
}

static void test_chip8_decode_handler_msb_0_opcode_00E0(void **state)
{
    int      i;
    int      j;
    chip8_t  chip8;
    uint16_t command = 0x00E0;
    uint8_t  opcode;

    chip8_decode_handler_msb_0(&chip8, command, opcode);

    for (i = 0; i < CHIP8_DISPLAY_HEIGHT; i++) {
        for (j = 0; j < CHIP8_DISPLAY_WIDTH; j++) {
            assert_true(chip8.display[i][j] == '\0');
        }
    }
}

static void test_chip8_decode_handler_msb_0_opcode_00EE_invalid_sp(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x00EE;
    uint8_t  opcode;

    chip8.stack_pointer = (sizeof(chip8.stack) / sizeof(uint8_t)) * 2;
    err                 = chip8_decode_handler_msb_0(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_INVALID_STACK_PTR_ERR);

    chip8.stack_pointer = -10;
    err                 = chip8_decode_handler_msb_0(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_INVALID_STACK_PTR_ERR);
}

static void test_chip8_decode_handler_msb_0_opcode_00EE_success(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x00EE;
    uint8_t  opcode;

    chip8.stack_pointer              = 10;
    chip8.stack[chip8.stack_pointer] = 11;
    err = chip8_decode_handler_msb_0(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.program_counter, 11);
    assert_int_equal(chip8.stack_pointer, 9);
}

static void test_chip8_decode_handler_msb_1_opcode_1NNN_success(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x1FFF;
    uint8_t  opcode;

    err = chip8_decode_handler_msb_1(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.program_counter, 0x0FFF);
}

static void test_chip8_decode_handler_msb_2_opcode_2NNN_invalid_sp(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x2FFF;
    uint8_t  opcode;

    chip8.stack_pointer = (sizeof(chip8.stack) / sizeof(uint8_t)) * 2;
    err                 = chip8_decode_handler_msb_2(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_INVALID_STACK_PTR_ERR);

    chip8.stack_pointer = -10;
    err                 = chip8_decode_handler_msb_2(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_INVALID_STACK_PTR_ERR);
}

static void test_chip8_decode_handler_msb_2_opcode_2NNN_success(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x2FFF;
    uint8_t  opcode;

    chip8.program_counter = 1;
    chip8.stack_pointer   = 4;
    err                   = chip8_decode_handler_msb_2(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.stack_pointer, 5);
    assert_int_equal(chip8.stack[chip8.stack_pointer], 1);
    assert_int_equal(chip8.program_counter, 0x0FFF);
}

static void test_chip8_decode_handler_msb_3_opcode_3XKK_success(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x3ABB;
    uint8_t  opcode;

    chip8.program_counter = 1;
    chip8.registers[0xA]  = 0xAA;
    err                   = chip8_decode_handler_msb_3(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.program_counter, 1);

    chip8.program_counter = 1;
    chip8.registers[0xA]  = 0xBB;
    err                   = chip8_decode_handler_msb_3(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.program_counter, 3);
}

static void test_chip8_decode_handler_msb_4_opcode_4XKK_success(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x4ABB;
    uint8_t  opcode;

    chip8.program_counter = 1;
    chip8.registers[0xA]  = 0xBB;
    err                   = chip8_decode_handler_msb_4(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.program_counter, 1);

    chip8.program_counter = 1;
    chip8.registers[0xA]  = 0xAA;
    err                   = chip8_decode_handler_msb_4(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.program_counter, 3);
}

static void test_chip8_decode_handler_msb_5_opcode_5XY0_success(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x5AB0;
    uint8_t  opcode;

    chip8.program_counter = 1;
    chip8.registers[0xA]  = 0xB;
    chip8.registers[0xB]  = 0xB;
    err                   = chip8_decode_handler_msb_5(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.program_counter, 1);

    chip8.program_counter = 1;
    chip8.registers[0xA]  = 0xA;
    chip8.registers[0xB]  = 0xB;
    err                   = chip8_decode_handler_msb_5(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.program_counter, 3);
}

static void test_chip8_decode_handler_msb_6_opcode_6XKK_success(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x6ABB;
    uint8_t  opcode;

    err = chip8_decode_handler_msb_6(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.registers[0xA], 0xBB);
}

static void test_chip8_decode_handler_msb_7_opcode_7XKK_success(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x7ABB;
    uint8_t  opcode;

    chip8.registers[0xA] = 0x00;
    err                  = chip8_decode_handler_msb_7(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.registers[0xA], 0xBB);
}

static void test_chip8_decode_handler_msb_8_opcode_8XY0_success(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x8AB0;
    uint8_t  opcode;

    chip8.registers[0xA] = 0xAA;
    chip8.registers[0xB] = 0xFF;
    err                  = chip8_decode_handler_msb_8(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.registers[0xA], 0xFF);
}

static void test_chip8_decode_handler_msb_8_opcode_8XY1_success(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x8AB1;
    uint8_t  opcode;

    chip8.registers[0xA] = 0xAA;
    chip8.registers[0xB] = 0xFF;
    err                  = chip8_decode_handler_msb_8(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.registers[0xA], 0xFF | 0xAA);
}

static void test_chip8_decode_handler_msb_8_opcode_8XY2_success(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x8AB2;
    uint8_t  opcode;

    chip8.registers[0xA] = 0xAA;
    chip8.registers[0xB] = 0xFF;
    err                  = chip8_decode_handler_msb_8(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.registers[0xA], 0xFF & 0xAA);
}

static void test_chip8_decode_handler_msb_8_opcode_8XY3_success(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x8AB3;
    uint8_t  opcode;

    chip8.registers[0xA] = 0xAA;
    chip8.registers[0xB] = 0xFF;
    err                  = chip8_decode_handler_msb_8(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.registers[0xA], 0xFF ^ 0xAA);
}

static void test_chip8_decode_handler_msb_8_opcode_8XY4_success(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x8AB4;
    uint8_t  opcode;

    chip8.registers[0xA] = 0x10;
    chip8.registers[0xB] = 0x20;

    err = chip8_decode_handler_msb_8(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.registers[0xA], 0x30);
    assert_int_equal(chip8.registers[0xF], 0);
}

static void test_chip8_decode_handler_msb_8_opcode_8XY4_carry(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x8AB4;
    uint8_t  opcode;

    chip8.registers[0xA] = 0xFF;
    chip8.registers[0xB] = 0x01;

    err = chip8_decode_handler_msb_8(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.registers[0xA], 0x00);
    assert_int_equal(chip8.registers[0xF], 1);
}

static void test_chip8_decode_handler_msb_8_opcode_8XY5_no_borrow(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x8AB5;
    uint8_t  opcode;

    chip8.registers[0xA] = 0x30;
    chip8.registers[0xB] = 0x20;

    err = chip8_decode_handler_msb_8(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.registers[0xA], 0x10);
    assert_int_equal(chip8.registers[0xF], 1);
}

static void test_chip8_decode_handler_msb_8_opcode_8XY5_borrow(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x8AB5;
    uint8_t  opcode;

    chip8.registers[0xA] = 0x10;
    chip8.registers[0xB] = 0x20;

    err = chip8_decode_handler_msb_8(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.registers[0xA], 0xF0);
    assert_int_equal(chip8.registers[0xF], 0);
}

static void
test_chip8_decode_handler_msb_8_opcode_8XY6_shift_right_even(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x8AB6;
    uint8_t  opcode;

    chip8.registers[0xA] = 0x14;

    err = chip8_decode_handler_msb_8(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.registers[0xA], 0x0A);
    assert_int_equal(chip8.registers[0xF], 0);
}

static void
test_chip8_decode_handler_msb_8_opcode_8XY6_shift_right_odd(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x8AB6;
    uint8_t  opcode;

    chip8.registers[0xA] = 0x15;

    err = chip8_decode_handler_msb_8(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.registers[0xA], 0x0A);
    assert_int_equal(chip8.registers[0xF], 1);
}

static void test_chip8_decode_handler_msb_8_opcode_8XY7_no_borrow(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x8AB7;
    uint8_t  opcode;

    chip8.registers[0xA] = 0x20;
    chip8.registers[0xB] = 0x30;

    err = chip8_decode_handler_msb_8(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.registers[0xA], 0x10);
    assert_int_equal(chip8.registers[0xF], 1);
}

static void test_chip8_decode_handler_msb_8_opcode_8XY7_borrow(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x8AB7;
    uint8_t  opcode;

    chip8.registers[0xA] = 0x30;
    chip8.registers[0xB] = 0x10;

    err = chip8_decode_handler_msb_8(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.registers[0xA], 0xE0);
    assert_int_equal(chip8.registers[0xF], 0);
}

static void test_chip8_decode_handler_msb_8_opcode_8XYE_shift(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x8ABE;
    uint8_t  opcode;

    chip8.registers[0xA] = 0x80;
    chip8.registers[0xB] = 0x00;

    err = chip8_decode_handler_msb_8(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.registers[0xA], 0x00);
    assert_int_equal(chip8.registers[0xF], 1);
}

static void test_chip8_decode_handler_msb_8_opcode_8XYE_no_carry(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x8ABE;
    uint8_t  opcode;

    chip8.registers[0xA] = 0x40;
    chip8.registers[0xB] = 0x00;

    err = chip8_decode_handler_msb_8(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.registers[0xA], 0x80);
    assert_int_equal(chip8.registers[0xF], 0);
}

static void test_chip8_decode_handler_msb_9_opcode_9XY0_success(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x9AB0;
    uint8_t  opcode;

    chip8.program_counter = 1;
    chip8.registers[0xA]  = 0x10;
    chip8.registers[0xB]  = 0x20;

    err = chip8_decode_handler_msb_9(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.program_counter, 3);

    chip8.program_counter = 1;
    chip8.registers[0xA]  = 0x10;
    chip8.registers[0xB]  = 0x10;

    err = chip8_decode_handler_msb_9(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.program_counter, 1);
}

static void test_chip8_decode_handler_msb_A_opcode_ANNN_success(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0xACCC;
    uint8_t  opcode;


    chip8.i_register = 0xFF;

    err = chip8_decode_handler_msb_A(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.i_register, 0xCCC);
}

static void test_chip8_decode_handler_msb_B_opcode_BNNN_success(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0xBCCC;
    uint8_t  opcode;


    chip8.registers[0x0]  = 0x01;
    chip8.program_counter = 0;
    err                   = chip8_decode_handler_msb_B(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.program_counter, 0x01 + 0xCCC);
}

static void test_chip8_decode_handler_msb_C_opcode_CXKK_success(void **state)
{
    int      err;
    chip8_t  chip8;
    uint16_t command = 0x6ABB;
    uint8_t  opcode;

    chip8.registers[0xA] = -1;
    err                  = chip8_decode_handler_msb_C(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_not_equal(chip8.registers[0xA], 0x00);
}

static void test_chip8_decode_handler_msb_D_basic_sprite(void **state)
{
    chip8_t  chip8;
    uint16_t command = 0xD015;
    uint8_t  opcode;
    int      err;

    memset(&chip8, 0, sizeof(chip8_t));
    chip8.i_register     = 0x200;
    chip8.registers[0x0] = 5;
    chip8.registers[0x1] = 10;

    chip8.memory[0x200] = 0x3C;
    chip8.memory[0x201] = 0xC3;
    chip8.memory[0x202] = 0xFF;
    chip8.memory[0x203] = 0xC3;
    chip8.memory[0x204] = 0x3C;

    err = chip8_decode_handler_msb_D(&chip8, command, opcode);

    assert_int_equal(err, CHIP8_OK);
    assert_int_equal(chip8.program_counter, 2);
    assert_int_equal(chip8.draw, 1);

    uint8_t expected_display[5][6] = { { 1, 0, 1, 1, 0, 1 },
                                       { 0, 1, 1, 1, 1, 0 },
                                       { 1, 1, 1, 1, 1, 1 },
                                       { 0, 1, 1, 1, 1, 0 },
                                       { 1, 0, 1, 1, 0, 1 } };

    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 6; x++) {
            assert_int_equal(chip8.display[y + 10][x + 5],
                             expected_display[y][x]);
        }
    }
}

// static void test_chip8_decode_handler_msb_D_collision(void **state) {
//     chip8_t chip8;
//     uint16_t command = 0xD012; // DRW V0, V1, 2 (draw 2-byte sprite)
//     uint8_t opcode;
//     int err;

//     // Initialize the Chip8 struct (partially, for this test)
//     memset(&chip8, 0, sizeof(chip8_t));
//     chip8.i_register = 0x200; // Set I register to starting address of sprite
//     data chip8.V[0] = 5;        // Set V0 (x-coordinate) to 5 chip8.V[1] =
//     10;       // Set V1 (y-coordinate) to 10

//     // Set sprite data in memory
//     chip8.memory[0x200] = 0xF0;
//     chip8.memory[0x201] = 0x0F;

//     // Set some pixels on the display to cause a collision
//     chip8.display[10][5] = 1;
//     chip8.display[11][7] = 1;

//     err = chip8_decode_handler_msb_D(&chip8, command, opcode);

//     assert_int_equal(err, CHIP8_OK);
//     assert_int_equal(chip8.program_counter, 2); // PC should be incremented
//     assert_int_equal(chip8.draw, 1);          // Draw flag should be set
//     assert_int_equal(chip8.V[0xF], 1);       // Collision register should be
//     set
// }


int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_chip8_decode_handlers_null_ptr),
        cmocka_unit_test(test_chip8_decode_handler_msb_0_opcode_00E0),
        cmocka_unit_test(
            test_chip8_decode_handler_msb_0_opcode_00EE_invalid_sp),
        cmocka_unit_test(test_chip8_decode_handler_msb_0_opcode_00EE_success),
        cmocka_unit_test(test_chip8_decode_handler_msb_1_opcode_1NNN_success),
        cmocka_unit_test(
            test_chip8_decode_handler_msb_2_opcode_2NNN_invalid_sp),
        cmocka_unit_test(test_chip8_decode_handler_msb_2_opcode_2NNN_success),
        cmocka_unit_test(test_chip8_decode_handler_msb_3_opcode_3XKK_success),
        cmocka_unit_test(test_chip8_decode_handler_msb_4_opcode_4XKK_success),
        cmocka_unit_test(test_chip8_decode_handler_msb_5_opcode_5XY0_success),
        cmocka_unit_test(test_chip8_decode_handler_msb_6_opcode_6XKK_success),
        cmocka_unit_test(test_chip8_decode_handler_msb_7_opcode_7XKK_success),
        cmocka_unit_test(test_chip8_decode_handler_msb_8_opcode_8XY0_success),
        cmocka_unit_test(test_chip8_decode_handler_msb_8_opcode_8XY1_success),
        cmocka_unit_test(test_chip8_decode_handler_msb_8_opcode_8XY2_success),
        cmocka_unit_test(test_chip8_decode_handler_msb_8_opcode_8XY3_success),
        cmocka_unit_test(test_chip8_decode_handler_msb_8_opcode_8XY4_success),
        cmocka_unit_test(test_chip8_decode_handler_msb_8_opcode_8XY4_carry),
        cmocka_unit_test(test_chip8_decode_handler_msb_8_opcode_8XY5_no_borrow),
        cmocka_unit_test(test_chip8_decode_handler_msb_8_opcode_8XY5_borrow),
        cmocka_unit_test(
            test_chip8_decode_handler_msb_8_opcode_8XY6_shift_right_even),
        cmocka_unit_test(
            test_chip8_decode_handler_msb_8_opcode_8XY6_shift_right_odd),
        cmocka_unit_test(test_chip8_decode_handler_msb_8_opcode_8XY7_no_borrow),
        cmocka_unit_test(test_chip8_decode_handler_msb_8_opcode_8XY7_borrow),
        cmocka_unit_test(test_chip8_decode_handler_msb_8_opcode_8XYE_shift),
        cmocka_unit_test(test_chip8_decode_handler_msb_8_opcode_8XYE_no_carry),
        cmocka_unit_test(test_chip8_decode_handler_msb_9_opcode_9XY0_success),
        cmocka_unit_test(test_chip8_decode_handler_msb_A_opcode_ANNN_success),
        cmocka_unit_test(test_chip8_decode_handler_msb_B_opcode_BNNN_success),
        cmocka_unit_test(test_chip8_decode_handler_msb_C_opcode_CXKK_success),
        cmocka_unit_test(test_chip8_decode_handler_msb_D_basic_sprite),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}