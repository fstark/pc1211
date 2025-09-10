#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../opcodes.h"
#include "../program.h"
#include "../tokenizer.h"
#include "../runtime.h"

/* Simple test framework */
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name)                              \
    do                                          \
    {                                           \
        printf("Running test: " #name " ... "); \
        tests_run++;                            \
        if (test_##name())                      \
        {                                       \
            printf("PASS\n");                   \
            tests_passed++;                     \
        }                                       \
        else                                    \
        {                                       \
            printf("FAIL\n");                   \
        }                                       \
    } while (0)

/* Test basic program initialization */
bool test_program_init(void)
{
    program_init();

    /* Check that variables are initialized */
    VarCell *cell = var_get(1); /* Variable A */
    if (cell->type != VAR_NUM || cell->value.num != 0.0)
    {
        return false;
    }

    /* Check that program is empty */
    LineRecord *line = program_first_line();
    if (line != NULL)
    {
        return false;
    }

    return true;
}

/* Test variable assignment */
bool test_variables(void)
{
    program_init();

    /* Test numeric assignment */
    var_set_num(1, 42.5); /* A = 42.5 */
    VarCell *cell = var_get(1);
    if (cell->type != VAR_NUM || cell->value.num != 42.5)
    {
        return false;
    }

    /* Test string assignment (with truncation and uppercase) */
    var_set_str(2, "hello world"); /* B = "HELLO W" (truncated to 7) */
    cell = var_get(2);
    if (cell->type != VAR_STR || strcmp(cell->value.str, "HELLO W") != 0)
    {
        return false;
    }

    /* Test overwrite behavior */
    var_set_str(1, "test"); /* A = "TEST" (overwrites numeric) */
    cell = var_get(1);
    if (cell->type != VAR_STR || strcmp(cell->value.str, "TEST") != 0)
    {
        return false;
    }

    return true;
}

/* Test basic tokenization */
bool test_tokenization(void)
{
    program_init();

    /* Test simple assignment */
    uint8_t tokens[TOKBUF_LINE_MAX];
    int token_len;

    if (!tokenize_line("A=42", 10, tokens, &token_len))
    {
        return false;
    }

    /* Should be: T_VAR(1) T_EQ_ASSIGN T_NUM(42.0) */
    if (token_len < 11)
        return false; /* VAR(2) + ASSIGN(1) + NUM(9) */
    if (tokens[0] != T_VAR || tokens[1] != 1)
        return false;
    if (tokens[2] != T_EQ_ASSIGN)
        return false;
    if (tokens[3] != T_NUM)
        return false;
    double val = *(double *)(tokens + 4);
    if (val != 42.0)
        return false;

    return true;
}

/* Test line management */
bool test_line_management(void)
{
    program_init();

    /* Add some lines */
    uint8_t tokens1[] = {T_VAR, 1, T_EQ_ASSIGN, T_NUM, 0, 0, 0, 0, 0, 0, 0x40, 0x45}; /* A=42 */
    uint8_t tokens2[] = {T_PRINT, T_VAR, 1};                                          /* PRINT A */

    if (!program_add_line(20, tokens1, sizeof(tokens1)))
        return false;
    if (!program_add_line(10, tokens2, sizeof(tokens2)))
        return false;

    /* Lines should be ordered */
    LineRecord *line = program_first_line();
    if (!line || line->line_num != 10)
        return false;

    line = program_next_line(line);
    if (!line || line->line_num != 20)
        return false;

    line = program_next_line(line);
    if (line != NULL)
        return false; /* Should be end */

    return true;
}

int main(void)
{
    printf("PC-1211 BASIC Test Runner\n");
    printf("========================\n\n");

    /* Run tests */
    TEST(program_init);
    TEST(variables);
    TEST(tokenization);
    TEST(line_management);

    /* Summary */
    printf("\nTest Results: %d/%d passed\n", tests_passed, tests_run);

    if (tests_passed == tests_run)
    {
        printf("All tests passed!\n");
        return 0;
    }
    else
    {
        printf("Some tests failed.\n");
        return 1;
    }
}
