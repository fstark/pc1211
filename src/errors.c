#include "errors.h"
#include "program.h"
#include <stdio.h>
#include <stdlib.h>

/* Global error state */
ErrorCode g_last_error = ERR_NONE;
int g_error_line = 0;

/* Global jump buffer for error handling */
jmp_buf g_error_jump_buf;

/* Source context for error display */
static const char *g_error_source_line = NULL;
static int g_error_content_offset = 0;
static int g_error_pos = -1;
static int g_error_token_offset = -1;

/* Error messages */
const char *error_message(ErrorCode code)
{
    switch (code)
    {
    case ERR_NONE:
        return "No error";
    case 1:
        return "Grammatical/operational error";
    case 2:
        return "Line error";
    case 3:
        return "Depth error";
    case 4:
        return "Step-over error";
    case 6:
        return "Format error";
    case ERR_USER_INTERRUPTED:
        return "User interrupted";
    default:
        return "Unknown error";
    }
}

/* Set source context for error display */
void error_set_source(const char *full_line, int content_offset)
{
    g_error_source_line = full_line;
    g_error_content_offset = content_offset;
}

/* Clear source context */
void error_clear_source(void)
{
    g_error_source_line = NULL;
    g_error_content_offset = 0;
    g_error_pos = -1;
}

/* Set error position within content */
void error_set_pos(int pos)
{
    g_error_pos = pos;
}

/* Set token byte offset for runtime errors */
void error_set_token_offset(int offset)
{
    g_error_token_offset = offset;
}

/* Print source line with caret pointing to error position */
static void error_print_source(int pos_in_content)
{
    if (!g_error_source_line)
        return;

    fprintf(stderr, "  %s\n", g_error_source_line);
    /* Caret at: 2 (indent) + content_offset + pos_in_content */
    int caret_pos = 2 + g_error_content_offset + pos_in_content;
    fprintf(stderr, "  %*s^\n", caret_pos - 2, "");
}

/* Report error and halt */
void error_report(ErrorCode code, int line_number)
{
    g_last_error = code;
    g_error_line = line_number;

    fprintf(stderr, "Error %d", (int)code);
    if (line_number > 0)
    {
        fprintf(stderr, " at line %d", line_number);
    }
    fprintf(stderr, ": %s\n", error_message(code));

    if (g_error_source_line && g_error_pos >= 0)
    {
        error_print_source(g_error_pos);
    }

    exit(1);
}

/* Fatal error - never returns, uses longjmp */
void error_fatal(ErrorCode code, int line_number)
{
    g_last_error = code;
    g_error_line = line_number;

    /* Jump back to error context */
    longjmp(g_error_jump_buf, 1);
}

/* Set error without halting */
void error_set(ErrorCode code, int line_number)
{
    g_last_error = code;
    g_error_line = line_number;
}

/* Get current error code */
ErrorCode error_get_code(void)
{
    return g_last_error;
}

/* Clear error state */
void error_clear(void)
{
    g_last_error = ERR_NONE;
    g_error_line = 0;
}

/* Count colons in source text, return char position of the nth colon-separated statement */
static int find_statement_pos_in_source(const char *source, int stmt_index)
{
    int current_stmt = 0;
    int i = 0;
    bool in_string = false;

    /* Skip leading line number and whitespace */
    while (source[i] && (source[i] >= '0' && source[i] <= '9'))
        i++;
    while (source[i] == ' ' || source[i] == '\t')
        i++;

    if (stmt_index == 0)
        return i;

    while (source[i])
    {
        if (source[i] == '"')
            in_string = !in_string;
        if (!in_string && source[i] == ':')
        {
            current_stmt++;
            if (current_stmt == stmt_index)
            {
                /* Skip the colon and any space after it */
                i++;
                while (source[i] == ' ')
                    i++;
                return i;
            }
        }
        i++;
    }
    return i; /* Past end, shouldn't happen */
}

/* Print current error */
void error_print(void)
{
    if (g_last_error != ERR_NONE)
    {
        fprintf(stderr, "Error %d", (int)g_last_error);
        if (g_error_line > 0)
        {
            fprintf(stderr, " at line %d", g_error_line);
        }
        fprintf(stderr, ": %s\n", error_message(g_last_error));

        /* Show source line if available */
        if (g_error_line > 0)
        {
            const char *src = program_get_source((uint16_t)g_error_line);
            if (src)
            {
                fprintf(stderr, "  %s\n", src);

                /* Show caret if we have token offset info */
                if (g_error_token_offset >= 0)
                {
                    /* Count T_COLON tokens before the error offset */
                    uint8_t *line_ptr = program_find_line((uint16_t)g_error_line);
                    if (line_ptr)
                    {
                        const uint8_t *tok = get_tokens(line_ptr);
                        int stmt_count = 0;
                        int bytes_seen = 0;

                        while (bytes_seen < g_error_token_offset && *tok != T_EOL)
                        {
                            if (*tok == T_COLON)
                                stmt_count++;

                            /* Advance past current token */
                            switch (*tok)
                            {
                            case T_NUM:
                                tok += 9;
                                bytes_seen += 9;
                                break;
                            case T_STR:
                            {
                                int slen = 2 + tok[1];
                                tok += slen;
                                bytes_seen += slen;
                                break;
                            }
                            case T_VAR:
                            case T_SVAR:
                                tok += 2;
                                bytes_seen += 2;
                                break;
                            default:
                                tok++;
                                bytes_seen++;
                                break;
                            }
                        }

                        int caret_pos = find_statement_pos_in_source(src, stmt_count);
                        fprintf(stderr, "  %*s^\n", caret_pos, "");
                    }
                }
            }
        }
    }
}
