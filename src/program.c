#include "program.h"
#include "errors.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* Global program state */
Program g_program;

/* Initialize program memory */
void program_init(void)
{
    memset(&g_program, 0, sizeof(g_program));
    program_clear();
}

/* Clear program memory */
void program_clear(void)
{
    g_program.prog_len = 2;
    g_program.prog[0] = 0;
    g_program.prog[1] = 0;
    g_program.prog[2] = 0xff;
    g_program.prog[3] = 0xff;
    var_init_all();
}

/* Initialize all variables to NUM=0, STR="" */
void var_init_all(void)
{
    for (int i = 1; i <= VARS_MAX; i++)
    {
        g_program.vars[i].type = VAR_NUM;
        g_program.vars[i].value.num = 0.0;
    }
}

/* Get variable by 1-based index */
VarCell *var_get(int index)
{
    if (index < 1 || index > VARS_MAX)
    {
        error_report(ERR_INDEX_OUT_OF_RANGE, 0);
        return NULL; /* Never reached */
    }
    return &g_program.vars[index - 1];
}

/* Set variable to numeric value */
void var_set_num(int index, double value)
{
    VarCell *cell = var_get(index);
    cell->type = VAR_NUM;
    cell->value.num = value;
}

/* Set variable to string value (truncate to STR_MAX, uppercase) */
void var_set_str(int index, const char *value)
{
    VarCell *cell = var_get(index);
    cell->type = VAR_STR;

    int len = strlen(value);
    if (len > STR_MAX)
    {
        len = STR_MAX;
    }

    /* Copy and convert to uppercase */
    for (int i = 0; i < len; i++)
    {
        char c = value[i];
        if (c >= 'a' && c <= 'z')
        {
            c = c - 'a' + 'A';
        }
        cell->value.str[i] = c;
    }
    cell->value.str[len] = '\0';
}

/* Add or replace a line in program memory */
bool program_add_line(uint16_t line_num, const uint8_t *tokens, int token_len)
{
    if (line_num < 1 || line_num > LINE_NUM_MAX)
    {
        error_report(ERR_BAD_LINE_NUMBER, 0);
        return false;
    }

    /* Calculate total record size */
    int record_len = 4 + token_len + 1; /* len + line + tokens + T_EOL */

    /* Check if program would be too large */
    if (g_program.prog_len + record_len > PROG_MAX_BYTES)
    {
        error_report(ERR_PROGRAM_TOO_LARGE, line_num);
        return false;
    }

    /* Remove existing line if present */
    program_delete_line(line_num);

    /* Find insertion point (maintain line number order) */
    uint8_t *insert_pos = program_find_first_line_after(line_num);
    int bytes_to_shift = g_program.prog + g_program.prog_len - insert_pos;
    if (bytes_to_shift > 0)
    {
        memmove(insert_pos + record_len, insert_pos, bytes_to_shift);
    }

    /* Write new record */
    *(uint16_t *)insert_pos = record_len;
    *(uint16_t *)(insert_pos + 2) = line_num;
    memcpy(insert_pos + 4, tokens, token_len);
    insert_pos[4 + token_len] = T_EOL;
    g_program.prog_len += record_len;

    // printf("PROGRAM ADD LINE %d (len=%d), new prog_len=%d\n", line_num, record_len, g_program.prog_len + record_len); // Debug print --- IGNORE ---
    return true;
}

void program_delete_line_ptr(uint8_t *line_ptr)
{
    assert(line_ptr);
    size_t bytes_to_delete = get_len(line_ptr);
    assert(bytes_to_delete > 0); /* Can't delete terminator */
    size_t bytes_after = g_program.prog_len - (line_ptr - g_program.prog) - bytes_to_delete;

    if (bytes_after > 0)
    {
        memmove(line_ptr, line_ptr + bytes_to_delete, bytes_after);
    }

    g_program.prog_len -= bytes_to_delete;
}

/* Delete a line from program memory */
bool program_delete_line(uint16_t line_num)
{
    uint8_t *line_ptr = program_find_line(line_num);

    if (line_ptr)
    {
        program_delete_line_ptr(line_ptr);
        return true;
    }

    return false; /* Line not found */
}

/* Label management functions */
static bool program_match_label(uint8_t *line_ptr, const char *label)
{
    program_validate_line_ptr(line_ptr);

    uint8_t *tokens = get_tokens(line_ptr);

    if (*tokens != T_STR)
        return false;
    uint8_t len = tokens[1];

    return strncmp((const char *)(tokens + 2), label, len) == 0;
}

/* Find line number for a label */
uint8_t *program_find_line_label(const char *label)
{
    for (uint8_t *line_ptr = program_first_line(); !program_is_last_line(line_ptr); line_ptr += get_len(line_ptr))
    {
        if (program_match_label(line_ptr, label))
            return line_ptr;
    }
    return NULL;
}

/* Find a line by line number */
uint8_t *program_find_line(uint16_t target_line)
{
    uint8_t *pos = g_program.prog;
    while (get_len(pos) != 0)
    {
        if (get_line(pos) == target_line)
            return pos;
        pos += get_len(pos);
    }
    return NULL;
}

/* Find the last line before a given line number */
uint8_t *program_find_first_line_after(uint16_t target_line)
{
    uint8_t *pos = g_program.prog;

    while (get_len(pos) != 0)
    {
        if (get_line(pos) > target_line)
            return pos;
        pos += get_len(pos);
    }

    return pos; /* Return end marker if no later line found */
}

/* Get first line */
uint8_t *program_first_line(void)
{
    uint8_t *pos = g_program.prog;
    /* Always return the first position - for empty programs, this will have len=0
     * and callers can use program_validate_line_ptr() or get_len() to check if it's valid */
    return pos;
}

/* Validate that a line pointer is within the program array */
void program_validate_line_ptr(uint8_t *line_ptr)
{
    assert(line_ptr);

    /* Must be within program bounds, but allow terminator position */
    assert(g_program.prog <= line_ptr && line_ptr <= g_program.prog + g_program.prog_len);

    /* Must be at a valid line boundary (len field exists and is reasonable) */
    if (!(line_ptr + sizeof(uint16_t) <= g_program.prog + g_program.prog_len))
        printf("%p + %d (=%p) > %p + %d (=%p)\n",
               (void *)line_ptr,
               (int)sizeof(uint16_t),
               (void *)(line_ptr + sizeof(uint16_t)),
               (void *)g_program.prog,
               g_program.prog_len,
               (void *)(g_program.prog + g_program.prog_len));

    uint16_t len = get_len(line_ptr);
    if (len == 0)
        return; /* End marker is valid */

    /* Line must fit within program bounds */
    assert(line_ptr + len <= g_program.prog + g_program.prog_len);

    /* Minimum line size: len(2) + line_num(2) + T_EOL(1) = 5 bytes */
    assert(len >= 5);
}

/* Check if we're at the last line (for iteration) */
bool program_is_last_line(uint8_t *line_ptr)
{
    program_validate_line_ptr(line_ptr);
    return get_len(line_ptr) == 0;
}

/* Get next line - NEVER call at end of program!
 * This function NEVER returns NULL - it either succeeds or asserts.
 * Use program_is_last_line() to check for end before calling. */
uint8_t *program_next_line(uint8_t *current_line)
{
    program_validate_line_ptr(current_line);
    assert(!program_is_last_line(current_line)); /* Calling at end is a bug! */

    uint8_t *next_pos = current_line + get_len(current_line);
    program_validate_line_ptr(next_pos); /* Corruption check */

    return next_pos;
}

/* Validate that a token pointer is within the program buffer */
bool program_validate_token_ptr(uint8_t *token_ptr)
{
    if (!token_ptr)
        return false;

    /* Must be within program bounds and have at least 1 byte for the token opcode */
    if (token_ptr < g_program.prog || g_program.prog + g_program.prog_len < token_ptr + 1)
        return false;

    return true;
}

/* Skip one token and return pointer to next - NEVER returns NULL */
uint8_t *token_skip(uint8_t *token)
{
    assert(program_validate_token_ptr(token));

    switch (*token)
    {
    case T_NUM:
        token += 1 + 8; /* opcode + 8 bytes double */
        break;

    case T_STR:
    {
        uint8_t len = token[1];
        token += 2 + len; /* opcode + length + string data */
        break;
    }

    case T_VAR:
    case T_SVAR:
    case T_THEN:        /* THEN followed by u16 line number */
        token += 1 + 1; /* opcode + 1 byte data */
        break;

    case T_VIDX:
    case T_SVIDX:
        /* Variable with index: T_VIDX/T_SVIDX <expression> T_ENDX */
        {
            uint8_t *pos = token + 1; /* Skip the opcode */
            /* Skip through the expression until we find T_ENDX */
            while (*pos != T_ENDX && *pos != T_EOL && *pos != 0)
            {
                assert(program_validate_token_ptr(pos));
                pos = token_skip(pos);
            }
            if (*pos == T_ENDX)
                pos++; /* Skip the T_ENDX terminator */
            token = pos;
            break;
        }

    case T_GOTO:
    case T_GOSUB:
        /* GOTO/GOSUB now followed by expression, skip to next statement */
        {
            uint8_t *pos = token + 1;
            /* Skip the expression tokens until we hit a statement terminator */
            while (*pos != T_EOL && *pos != T_COLON && *pos != 0)
            {
                assert(program_validate_token_ptr(pos));
                pos = token_skip(pos);
            }
            token = pos;
            break;
        }

    default:
        token += 1; /* Just the opcode */
        break;
    }

    assert(program_validate_token_ptr(token)); /* Validate result */
    return token;
}

/* Debug dump of token stream */
void token_dump(const uint8_t *tokens, int len)
{
    const uint8_t *pos = tokens;
    const uint8_t *end = tokens + len;

    while (pos < end && *pos != T_EOL)
    {
        printf("  %02X", *pos);

        switch (*pos)
        {
        case T_NUM:
        {
            double val = *(double *)(pos + 1);
            printf(" (NUM: %g)", val);
            pos += 9;
            break;
        }

        case T_STR:
        {
            uint8_t str_len = pos[1];
            printf(" (STR[%d]: \"", str_len);
            for (int i = 0; i < str_len; i++)
            {
                putchar(pos[2 + i]);
            }
            printf("\")");
            pos += 2 + str_len;
            break;
        }

        case T_VAR:
            printf(" (VAR: %c)", 'A' + pos[1] - 1);
            pos += 2;
            break;

        case T_GOTO:
        case T_GOSUB:
            printf(" (+ expression)");
            pos += 1;
            break;

        default:
            pos += 1;
            break;
        }

        printf("\n");
    }

    if (pos < end && *pos == T_EOL)
    {
        printf("  %02X (EOL)\n", T_EOL);
    }
}

/* VM helper functions */

/* Get token start for line */
uint8_t *program_find_line_tokens(uint16_t line_num)
{
    uint8_t *line_ptr = program_find_line(line_num);
    return line_ptr ? get_tokens(line_ptr) : NULL;
}

/* Find end of current line (find T_EOL) */
uint8_t *program_find_line_end(uint8_t *tokens)
{
    assert(program_validate_token_ptr(tokens));

    uint8_t *pos = tokens;
    uint8_t *prog_end = g_program.prog + g_program.prog_len;

    while (pos < prog_end && *pos != T_EOL)
    {
        pos = token_skip(pos);
    }

    /* Assert that we found T_EOL - every line should end with it */
    assert(pos < prog_end && *pos == T_EOL);

    return pos;
}

/* Find end of current line from any position within the line */
uint8_t *program_find_line_end_from_pos(uint8_t *pos)
{
    assert(program_validate_token_ptr(pos));

    uint8_t *prog_end = g_program.prog + g_program.prog_len;

    while (pos < prog_end && *pos != T_EOL)
    {
        pos = token_skip(pos);
    }

    /* We should have found T_EOL within the program bounds */
    return pos; /* pos now points to T_EOL or prog_end */
}

/* Get first line tokens */
uint8_t *program_first_line_tokens(void)
{
    uint8_t *line_ptr = program_first_line();
    return (get_len(line_ptr) != 0) ? get_tokens(line_ptr) : NULL;
}

/* Get next line tokens */
uint8_t *program_next_line_tokens(uint8_t *current)
{
    assert(current); /* Passing NULL is a programming error */

    /* Find the LineRecord that contains this token pointer */
    uint8_t *pos = g_program.prog;
    uint8_t *prog_end = g_program.prog + g_program.prog_len;

    while (pos < prog_end)
    {
        uint16_t len = *(uint16_t *)pos;
        uint8_t *tokens = pos + 4;

        if (tokens <= current && current < tokens + len - 4)
        {
            /* Found current line, get next */
            pos += len;
            if (pos >= prog_end)
                return NULL;

            return pos + 4; /* Skip header of next line */
        }

        pos += len;
    }

    return NULL;
}
