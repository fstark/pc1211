#include "runtime.h"
#include "program.h"
#include "errors.h"
#include <stdio.h>
#include <assert.h>

/* Token name lookup table */
const char *token_name(Tok token)
{
    switch (token)
    {
    case T_EOL:
        return "EOL";
    case T_NUM:
        return "NUM";
    case T_STR:
        return "STR";
    case T_VAR:
        return "VAR";
    case T_VIDX:
        return "VIDX";
    case T_ENDX:
        return "ENDX";

    case T_EQ_ASSIGN:
        return "=";
    case T_PLUS:
        return "+";
    case T_MINUS:
        return "-";
    case T_MUL:
        return "*";
    case T_DIV:
        return "/";
    case T_POW:
        return "^";
    case T_LP:
        return "(";
    case T_RP:
        return ")";
    case T_COMMA:
        return ",";
    case T_SEMI:
        return ";";
    case T_COLON:
        return ":";
    case T_EQ:
        return "=";
    case T_NE:
        return "<>";
    case T_LT:
        return "<";
    case T_LE:
        return "<=";
    case T_GT:
        return ">";
    case T_GE:
        return ">=";

    case T_SIN:
        return "SIN";
    case T_COS:
        return "COS";
    case T_TAN:
        return "TAN";
    case T_ASN:
        return "ASN";
    case T_ACS:
        return "ACS";
    case T_ATN:
        return "ATN";
    case T_LOG:
        return "LOG";
    case T_LN:
        return "LN";
    case T_EXP:
        return "EXP";
    case T_SQR:
        return "SQR";
    case T_DMS:
        return "DMS";
    case T_DEG:
        return "DEG";
    case T_INT:
        return "INT";
    case T_ABS:
        return "ABS";
    case T_SGN:
        return "SGN";

    case T_LET:
        return "LET";
    case T_PRINT:
        return "PRINT";
    case T_INPUT:
        return "INPUT";
    case T_IF:
        return "IF";
    case T_THEN:
        return "THEN";
    case T_GOTO:
        return "GOTO";
    case T_GOSUB:
        return "GOSUB";
    case T_RETURN:
        return "RETURN";
    case T_FOR:
        return "FOR";
    case T_TO:
        return "TO";
    case T_STEP:
        return "STEP";
    case T_NEXT:
        return "NEXT";
    case T_END:
        return "END";
    case T_STOP:
        return "STOP";
    case T_REM:
        return "REM";

    case T_DEGREE:
        return "DEGREE";
    case T_RADIAN:
        return "RADIAN";
    case T_GRAD:
        return "GRAD";
    case T_CLEAR:
        return "CLEAR";
    case T_BEEP:
        return "BEEP";
    case T_PAUSE:
        return "PAUSE";
    case T_AREAD:
        return "AREAD";
    case T_USING:
        return "USING";

    default:
        return "UNKNOWN";
    }
}

/* LIST command - display readable program listing */
void cmd_list(void)
{
    LineRecord *line = program_first_line();

    if (!line)
    {
        printf("No program loaded.\n");
        return;
    }

    while (line)
    {
        cmd_list_line(line->line_num);
        line = program_next_line(line);
    }
}

/* LIST a specific line */
void cmd_list_line(uint16_t line_num)
{
    LineRecord *line = program_find_line(line_num);
    if (!line)
    {
        printf("Line %d not found.\n", line_num);
        return;
    }

    printf("%d ", line->line_num);

    const uint8_t *pos = line->tokens;
    const uint8_t *end = line->tokens + line->len - 5; /* Subtract header and T_EOL */
    bool need_space = false;

    while (pos < end && *pos != T_EOL)
    {
        if (need_space)
        {
            printf(" ");
        }
        need_space = true;

        switch (*pos)
        {
        case T_NUM:
        {
            double val = *(double *)(pos + 1);
            printf("%g", val);
            pos += 9;
            break;
        }

        case T_STR:
        {
            uint8_t len = pos[1];
            printf("\"");
            for (int i = 0; i < len; i++)
            {
                putchar(pos[2 + i]);
            }
            printf("\"");
            pos += 2 + len;
            break;
        }

        case T_VAR:
        {
            printf("%c", 'A' + pos[1] - 1);
            pos += 2;
            break;
        }

        case T_VIDX:
        {
            printf("A(");
            pos++;
            /* Parse expression until T_ENDX */
            int paren_level = 0;
            while (pos < end && !(*pos == T_ENDX && paren_level == 0))
            {
                if (*pos == T_LP)
                    paren_level++;
                if (*pos == T_RP)
                    paren_level--;

                /* Recursively display expression tokens */
                switch (*pos)
                {
                case T_NUM:
                {
                    double val = *(double *)(pos + 1);
                    printf("%g", val);
                    pos += 9;
                    break;
                }
                case T_VAR:
                {
                    printf("%c", 'A' + pos[1] - 1);
                    pos += 2;
                    break;
                }
                case T_PLUS:
                    printf("+");
                    pos++;
                    break;
                case T_MINUS:
                    printf("-");
                    pos++;
                    break;
                case T_MUL:
                    printf("*");
                    pos++;
                    break;
                case T_DIV:
                    printf("/");
                    pos++;
                    break;
                case T_POW:
                    printf("^");
                    pos++;
                    break;
                case T_LP:
                    printf("(");
                    pos++;
                    break;
                case T_RP:
                    printf(")");
                    pos++;
                    break;
                default:
                    pos++;
                    break;
                }
            }
            if (pos < end && *pos == T_ENDX)
            {
                pos++;
            }
            printf(")");
            break;
        }

        case T_REM:
        {
            printf("REM");
            pos++;
            /* Check if followed by comment string */
            if (pos < end && *pos == T_STR)
            {
                printf(" ");
                uint8_t len = pos[1];
                for (int i = 0; i < len; i++)
                {
                    putchar(pos[2 + i]);
                }
                pos += 2 + len;
            }
            break;
        }

        case T_GOTO:
        case T_GOSUB:
        {
            printf("%s", token_name(*pos));
            pos++;
            if (pos + 1 < end)
            {
                /* Should be followed by T_NUM with line number */
                if (*pos == T_NUM)
                {
                    double line_val = *(double *)(pos + 1);
                    printf(" %g", line_val);
                    pos += 9;
                }
                else
                {
                    pos++;
                }
            }
            break;
        }

        case T_THEN:
        {
            printf("THEN");
            pos++;
            if (pos < end && *pos == T_NUM)
            {
                printf(" ");
                double line_val = *(double *)(pos + 1);
                printf("%g", line_val);
                pos += 9;
            }
            break;
        }

        case T_COMMA:
            printf(",");
            need_space = false; /* No space after comma */
            pos++;
            break;

        case T_SEMI:
            printf(";");
            need_space = false; /* No space after semicolon */
            pos++;
            break;

        case T_COLON:
            printf(" : ");
            need_space = false; /* Spaces already included */
            pos++;
            break;

        default:
        {
            printf("%s", token_name(*pos));
            pos++;
            break;
        }
        }
    }

    printf("\n");
}

/* Disassemble program (debug dump) */
void disassemble_program(void)
{
    LineRecord *line = program_first_line();

    if (!line)
    {
        printf("No program loaded.\n");
        return;
    }

    printf("Program dump:\n");
    while (line)
    {
        disassemble_line(line);
        line = program_next_line(line);
    }
}

/* Disassemble a single line (debug dump) */
void disassemble_line(LineRecord *line)
{
    printf("Line %d (len=%d):\n", line->line_num, line->len);
    int token_len = line->len - 5; /* Subtract header and T_EOL */
    disassemble_tokens(line->tokens, token_len);
    printf("\n");
}

/* Disassemble token stream (debug dump) */
void disassemble_tokens(const uint8_t *tokens, int len)
{
    const uint8_t *pos = tokens;
    const uint8_t *end = tokens + len;

    while (pos < end && *pos != T_EOL)
    {
        printf("  %02X %s", *pos, token_name(*pos));

        switch (*pos)
        {
        case T_NUM:
        {
            double val = *(double *)(pos + 1);
            printf(" (%g)", val);
            pos += 9;
            break;
        }

        case T_STR:
        {
            uint8_t str_len = pos[1];
            printf(" [%d] (\"", str_len);
            for (int i = 0; i < str_len; i++)
            {
                putchar(pos[2 + i]);
            }
            printf("\")");
            pos += 2 + str_len;
            break;
        }

        case T_VAR:
            printf(" (%c)", 'A' + pos[1] - 1);
            pos += 2;
            break;

        case T_GOTO:
        case T_GOSUB:
            pos++;
            if (pos < end && *pos == T_NUM)
            {
                double line_val = *(double *)(pos + 1);
                printf(" -> %g", line_val);
                pos += 9;
            }
            break;

        case T_THEN:
            pos++;
            if (pos < end && *pos == T_NUM)
            {
                double line_val = *(double *)(pos + 1);
                printf(" -> %g", line_val);
                pos += 9;
            }
            break;

        default:
            pos++;
            break;
        }

        printf("\n");
    }

    if (pos < end && *pos == T_EOL)
    {
        printf("  %02X %s\n", T_EOL, token_name(T_EOL));
    }
}
