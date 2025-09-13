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
    var_init_all();
}

/* Clear program memory */
void program_clear(void)
{
    g_program.prog_len = 0;
    var_init_all();
    program_clear_labels();
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
    uint8_t *insert_pos = g_program.prog;
    uint8_t *prog_end = g_program.prog + g_program.prog_len;

    while (insert_pos < prog_end)
    {
        uint16_t len = *(uint16_t *)insert_pos;
        uint16_t existing_line = *(uint16_t *)(insert_pos + 2);

        if (existing_line > line_num)
        {
            break;
        }
        insert_pos += len;
    }

    /* Shift existing data to make room */
    int bytes_to_shift = prog_end - insert_pos;
    if (bytes_to_shift > 0)
    {
        memmove(insert_pos + record_len, insert_pos, bytes_to_shift);
    }

    /* Write new record */
    *(uint16_t *)insert_pos = record_len;
    *(uint16_t *)(insert_pos + 2) = line_num;
    memcpy(insert_pos + 4, tokens, token_len);
    insert_pos[4 + token_len] = T_EOL;

    /* Check for label at start of line */
    if (token_len > 0 && tokens[0] == T_STR)
    {
        uint8_t str_len = tokens[1];
        if (str_len <= STR_MAX)
        {
            char label[STR_MAX + 1];
            memcpy(label, tokens + 2, str_len);
            label[str_len] = '\0';
            program_add_label(label, line_num);
        }
    }

    g_program.prog_len += record_len;
    return true;
}

/* Delete a line from program memory */
bool program_delete_line(uint16_t line_num)
{
    uint8_t *pos = g_program.prog;
    uint8_t *prog_end = g_program.prog + g_program.prog_len;

    while (pos < prog_end)
    {
        uint16_t len = *(uint16_t *)pos;
        uint16_t existing_line = *(uint16_t *)(pos + 2);

        if (existing_line == line_num)
        {
            /* Found line to delete - shift remaining data */
            int bytes_to_shift = prog_end - (pos + len);
            if (bytes_to_shift > 0)
            {
                memmove(pos, pos + len, bytes_to_shift);
            }
            g_program.prog_len -= len;
            return true;
        }

        if (existing_line > line_num)
        {
            break; /* Line not found */
        }

        pos += len;
    }

    return false; /* Line not found */
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

/* Get first line */
uint8_t *program_first_line(void)
{
    if (g_program.prog_len == 0)
        return NULL;

    uint8_t *pos = g_program.prog;
    if (get_len(pos) == 0)
        return NULL;

    assert(program_validate_line_ptr(pos));
    return pos;
} /* Validate that a line pointer is within the program array */
bool program_validate_line_ptr(uint8_t *line_ptr)
{
    if (!line_ptr)
        return false;

    /* Must be within program bounds */
    if (line_ptr < g_program.prog || line_ptr >= g_program.prog + g_program.prog_len)
        return false;

    /* Must be at a valid line boundary (len field exists and is reasonable) */
    if (line_ptr + sizeof(uint16_t) > g_program.prog + g_program.prog_len)
        return false;

    uint16_t len = get_len(line_ptr);
    if (len == 0)
        return true; /* End marker is valid */

    /* Line must fit within program bounds */
    if (line_ptr + len > g_program.prog + g_program.prog_len)
        return false;

    /* Minimum line size: len(2) + line_num(2) + T_EOL(1) = 5 bytes */
    if (len < 5)
        return false;

    return true;
}

/* Check if we're at the last line (for iteration) */
bool program_is_last_line(uint8_t *line_ptr)
{
    assert(program_validate_line_ptr(line_ptr));
    uint8_t *next_pos = line_ptr + get_len(line_ptr);
    return get_len(next_pos) == 0;
}

/* Get next line - NEVER call at end of program! 
 * This function NEVER returns NULL - it either succeeds or asserts.
 * Use program_is_last_line() to check for end before calling. */
uint8_t *program_next_line(uint8_t *current_line)
{
    assert(program_validate_line_ptr(current_line));
    assert(!program_is_last_line(current_line)); /* Calling at end is a bug! */

    uint8_t *next_pos = current_line + get_len(current_line);
    assert(program_validate_line_ptr(next_pos)); /* Corruption check */

    return next_pos;
}

/* Skip one token and return pointer to next */
uint8_t *token_skip(uint8_t *token)
{
    if (!token)
        return NULL;

    switch (*token)
    {
    case T_NUM:
        return token + 1 + 8; /* opcode + 8 bytes double */

    case T_STR:
    {
        uint8_t len = token[1];
        return token + 2 + len; /* opcode + length + string data */
    }

    case T_VAR:
    case T_SVAR:
    case T_THEN:              /* THEN followed by u16 line number */
        return token + 1 + 1; /* opcode + 1 byte data */

    case T_VIDX:
    case T_SVIDX:
        /* Variable with index: T_VIDX/T_SVIDX <expression> T_ENDX */
        {
            uint8_t *pos = token + 1; /* Skip the opcode */
            /* Skip through the expression until we find T_ENDX */
            while (pos && *pos != T_ENDX && *pos != T_EOL && *pos != 0)
            {
                pos = token_skip(pos);
            }
            if (pos && *pos == T_ENDX)
                pos++; /* Skip the T_ENDX terminator */
            return pos;
        }

    case T_GOTO:
    case T_GOSUB:
        /* GOTO/GOSUB now followed by expression, skip to next statement */
        {
            uint8_t *pos = token + 1;
            /* Skip the expression tokens until we hit a statement terminator */
            while (pos && *pos != T_EOL && *pos != T_COLON && *pos != 0)
            {
                pos = token_skip(pos);
            }
            return pos;
        }

    default:
        return token + 1; /* Just the opcode */
    }
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
    if (!tokens)
        return NULL;

    uint8_t *pos = tokens;
    uint8_t *prog_end = g_program.prog + g_program.prog_len;

    while (pos < prog_end && *pos != T_EOL)
    {
        pos = token_skip(pos);
        if (!pos)
            break;
    }

    /* Assert that we found T_EOL - every line should end with it */
    assert(pos < prog_end && *pos == T_EOL);

    return pos;
}

/* Find end of current line from any position within the line */
uint8_t *program_find_line_end_from_pos(uint8_t *pos)
{
    if (!pos)
        return NULL;

    uint8_t *prog_end = g_program.prog + g_program.prog_len;

    while (pos < prog_end && *pos != T_EOL)
    {
        pos = token_skip(pos);
        if (!pos)
            break;
    }

    /* We should have found T_EOL within the program bounds */
    return pos; /* pos now points to T_EOL or prog_end */
}

/* Get first line tokens */
uint8_t *program_first_line_tokens(void)
{
    uint8_t *line_ptr = program_first_line();
    return line_ptr ? get_tokens(line_ptr) : NULL;
}

/* Get next line tokens */
uint8_t *program_next_line_tokens(uint8_t *current)
{
    if (!current)
        return NULL;

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

/* Label management functions */

/* Clear all labels */
void program_clear_labels(void)
{
    g_program.label_count = 0;
}

/* Add a label mapping */
void program_add_label(const char *label, uint16_t line_num)
{
    if (g_program.label_count >= LABELS_MAX)
        return; /* Silently ignore if table full */

    /* Check if label already exists and update it */
    for (int i = 0; i < g_program.label_count; i++)
    {
        if (strcmp(g_program.labels[i].label, label) == 0)
        {
            g_program.labels[i].line_num = line_num;
            return;
        }
    }

    /* Add new label */
    strncpy(g_program.labels[g_program.label_count].label, label, STR_MAX);
    g_program.labels[g_program.label_count].label[STR_MAX] = '\0';
    g_program.labels[g_program.label_count].line_num = line_num;
    g_program.label_count++;
}

/* Find line number for a label */
uint16_t program_find_label(const char *label)
{
    for (int i = 0; i < g_program.label_count; i++)
    {
        if (strcmp(g_program.labels[i].label, label) == 0)
        {
            return g_program.labels[i].line_num;
        }
    }
    return 0; /* Label not found */
}
