#include "tokenizer.h"
#include "program.h"
#include "errors.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* Forward declaration for recursive expression parsing */
static bool tokenize_expression_recursive(Tokenizer *t, int *paren_count);

/* Keyword table - full names and abbreviations */
static const Keyword keywords[] = {
    /* Functions */
    {"SIN", "SI.", T_SIN},
    {"COS", NULL, T_COS},
    {"TAN", "TA.", T_TAN},
    {"ASN", "AS.", T_ASN},
    {"ACS", "AC.", T_ACS},
    {"ATN", "AT.", T_ATN},
    {"LOG", "LO.", T_LOG},
    {"LN", NULL, T_LN},
    {"EXP", "EX.", T_EXP},
    {"SQR", NULL, T_SQR},
    {"DMS", "DM.", T_DMS},
    {"DEG", NULL, T_DEG},
    {"INT", NULL, T_INT},
    {"ABS", "AB.", T_ABS},
    {"SGN", "SG.", T_SGN},

    /* Statements */
    {"LET", "LE.", T_LET},
    {"PRINT", "P.", T_PRINT},
    {"INPUT", "I.", T_INPUT},
    {"IF", NULL, T_IF},
    {"THEN", "T.", T_THEN},
    {"GOTO", "G.", T_GOTO},
    {"GOSUB", "GOS.", T_GOSUB},
    {"RETURN", "RE.", T_RETURN},
    {"FOR", "F.", T_FOR},
    {"TO", NULL, T_TO},
    {"STEP", "STE.", T_STEP},
    {"NEXT", "N.", T_NEXT},
    {"END", "E.", T_END},
    {"STOP", "S.", T_STOP},
    {"REM", NULL, T_REM},

    /* Mode commands */
    {"DEGREE", "DEG.", T_DEGREE},
    {"RADIAN", "RA.", T_RADIAN},
    {"GRAD", NULL, T_GRAD},
    {"CLEAR", "CL.", T_CLEAR},
    {"BEEP", "B.", T_BEEP},
    {"PAUSE", "PA.", T_PAUSE},
    {"AREAD", "A.", T_AREAD},
    {"USING", "U.", T_USING},

    {NULL, NULL, 0} /* Terminator */
};

/* Find keyword by name (case-insensitive) */
const Keyword *find_keyword(const char *word)
{
    for (const Keyword *kw = keywords; kw->name != NULL; kw++)
    {
        /* Check full name */
        if (strcasecmp(word, kw->name) == 0)
        {
            return kw;
        }
        /* Check abbreviation */
        if (kw->abbrev && strcasecmp(word, kw->abbrev) == 0)
        {
            return kw;
        }
    }
    return NULL;
}

/* Character classification */
bool is_alpha(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

bool is_alnum(char c)
{
    return is_alpha(c) || is_digit(c);
}

/* Skip whitespace */
void skip_whitespace(Tokenizer *t)
{
    while (t->input[t->pos] == ' ' || t->input[t->pos] == '\t')
    {
        t->pos++;
    }
}

/* Token emission functions */
void emit_token(Tokenizer *t, Tok token)
{
    if (t->token_len >= TOKBUF_LINE_MAX)
    {
        error_report(ERR_LINE_TOO_LONG, t->line_num);
        return;
    }
    t->tokens[t->token_len++] = (uint8_t)token;
}

void emit_token_u8(Tokenizer *t, Tok token, uint8_t data)
{
    if (t->token_len + 2 > TOKBUF_LINE_MAX)
    {
        error_report(ERR_LINE_TOO_LONG, t->line_num);
        return;
    }
    t->tokens[t->token_len++] = (uint8_t)token;
    t->tokens[t->token_len++] = data;
}

void emit_token_u16(Tokenizer *t, Tok token, uint16_t data)
{
    if (t->token_len + 3 > TOKBUF_LINE_MAX)
    {
        error_report(ERR_LINE_TOO_LONG, t->line_num);
        return;
    }
    t->tokens[t->token_len++] = (uint8_t)token;
    *(uint16_t *)(t->tokens + t->token_len) = data;
    t->token_len += 2;
}

void emit_token_double(Tokenizer *t, Tok token, double data)
{
    if (t->token_len + 9 > TOKBUF_LINE_MAX)
    {
        error_report(ERR_LINE_TOO_LONG, t->line_num);
        return;
    }
    t->tokens[t->token_len++] = (uint8_t)token;
    *(double *)(t->tokens + t->token_len) = data;
    t->token_len += 8;
}

void emit_token_string(Tokenizer *t, Tok token, const char *str, int len)
{
    if (len > STR_MAX)
        len = STR_MAX;
    if (t->token_len + 2 + len > TOKBUF_LINE_MAX)
    {
        error_report(ERR_LINE_TOO_LONG, t->line_num);
        return;
    }
    t->tokens[t->token_len++] = (uint8_t)token;
    t->tokens[t->token_len++] = (uint8_t)len;
    for (int i = 0; i < len; i++)
    {
        char c = str[i];
        /* Convert to uppercase */
        if (c >= 'a' && c <= 'z')
        {
            c = c - 'a' + 'A';
        }
        t->tokens[t->token_len++] = (uint8_t)c;
    }
}

/* Parse a number */
bool parse_number(Tokenizer *t)
{
    const char *start = t->input + t->pos;
    char *end;
    double value = strtod(start, &end);

    if (end == start)
    {
        return false; /* Not a number */
    }

    t->pos = end - t->input;
    emit_token_double(t, T_NUM, value);
    return true;
}

/* Parse a quoted string */
bool parse_string(Tokenizer *t)
{
    if (t->input[t->pos] != '"')
    {
        return false;
    }

    t->pos++; /* Skip opening quote */
    const char *start = t->input + t->pos;
    int len = 0;

    /* Find closing quote */
    while (t->input[t->pos] != '\0' && t->input[t->pos] != '"')
    {
        t->pos++;
        len++;
    }

    if (t->input[t->pos] != '"')
    {
        error_report(ERR_SYNTAX_ERROR, t->line_num);
        return false;
    }

    t->pos++; /* Skip closing quote */
    emit_token_string(t, T_STR, start, len);
    return true;
}

/* Parse a variable (A-Z) or A(expr) */
bool parse_variable(Tokenizer *t)
{
    char c = t->input[t->pos];
    if (c < 'A' || c > 'Z')
    {
        return false;
    }

    t->pos++;

    /* Check for A(expr) syntax */
    if (c == 'A' && t->input[t->pos] == '(')
    {
        emit_token(t, T_VIDX);
        t->pos++; /* Skip '(' */

        /* Parse expression inside parentheses */
        int paren_count = 1;
        while (paren_count > 0 && t->input[t->pos] != '\0')
        {
            if (!tokenize_expression_recursive(t, &paren_count))
            {
                error_report(ERR_SYNTAX_ERROR, t->line_num);
                return false;
            }
        }

        if (paren_count != 0)
        {
            error_report(ERR_SYNTAX_ERROR, t->line_num);
            return false;
        }

        emit_token(t, T_ENDX);
        return true;
    }
    else
    {
        /* Simple variable A-Z */
        emit_token_u8(t, T_VAR, c - 'A' + 1);
        return true;
    }
}

/* Parse expression tokens recursively (for A(expr) parsing) */
static bool tokenize_expression_recursive(Tokenizer *t, int *paren_count)
{
    skip_whitespace(t);

    if (t->input[t->pos] == '\0')
    {
        return false;
    }

    /* Handle parentheses */
    if (t->input[t->pos] == '(')
    {
        (*paren_count)++;
        emit_token(t, T_LP);
        t->pos++;
        return true;
    }

    if (t->input[t->pos] == ')')
    {
        (*paren_count)--;
        if (*paren_count >= 0)
        { /* Don't emit closing paren for A(expr) */
            if (*paren_count > 0)
            {
                emit_token(t, T_RP);
            }
            t->pos++;
        }
        return true;
    }

    /* Try to parse number */
    if (is_digit(t->input[t->pos]) || t->input[t->pos] == '.')
    {
        return parse_number(t);
    }

    /* Try to parse variable */
    if (is_alpha(t->input[t->pos]))
    {
        return parse_variable(t);
    }

    /* Try to parse operator */
    return parse_operator(t);
}

/* Parse keyword or identifier */
bool parse_keyword(Tokenizer *t)
{
    if (!is_alpha(t->input[t->pos]))
    {
        return false;
    }

    const char *start = t->input + t->pos;
    int len = 0;

    /* Read alphanumeric characters and dots */
    while (is_alnum(t->input[t->pos]) || t->input[t->pos] == '.')
    {
        t->pos++;
        len++;
    }

    /* Extract word */
    char word[32];
    if (len >= (int)sizeof(word))
    {
        error_report(ERR_SYNTAX_ERROR, t->line_num);
        return false;
    }
    strncpy(word, start, len);
    word[len] = '\0';

    /* Look up keyword */
    const Keyword *kw = find_keyword(word);
    if (kw)
    {
        /* Handle special cases that need additional data */
        if (kw->token == T_GOTO || kw->token == T_GOSUB)
        {
            emit_token(t, kw->token);
            skip_whitespace(t);
            /* Parse line number */
            if (!parse_number(t))
            {
                error_report(ERR_SYNTAX_ERROR, t->line_num);
                return false;
            }
        }
        else if (kw->token == T_THEN)
        {
            emit_token(t, kw->token);
            skip_whitespace(t);
            /* Check if followed by line number */
            if (is_digit(t->input[t->pos]))
            {
                if (!parse_number(t))
                {
                    error_report(ERR_SYNTAX_ERROR, t->line_num);
                    return false;
                }
            }
        }
        else
        {
            emit_token(t, kw->token);
        }
        return true;
    }

    /* Not a keyword - might be a variable */
    if (len == 1 && start[0] >= 'A' && start[0] <= 'Z')
    {
        t->pos = start - t->input; /* Reset position */
        return parse_variable(t);
    }

    error_report(ERR_SYNTAX_ERROR, t->line_num);
    return false;
}

/* Parse operators and punctuation */
bool parse_operator(Tokenizer *t)
{
    char c1 = t->input[t->pos];
    char c2 = t->input[t->pos + 1];

    /* Two-character operators */
    if (c1 == '<' && c2 == '=')
    {
        emit_token(t, T_LE);
        t->pos += 2;
        return true;
    }
    if (c1 == '>' && c2 == '=')
    {
        emit_token(t, T_GE);
        t->pos += 2;
        return true;
    }
    if (c1 == '<' && c2 == '>')
    {
        emit_token(t, T_NE);
        t->pos += 2;
        return true;
    }

    /* Single-character operators */
    switch (c1)
    {
    case '=':
        emit_token(t, T_EQ_ASSIGN);
        t->pos++;
        return true;
    case '+':
        emit_token(t, T_PLUS);
        t->pos++;
        return true;
    case '-':
        emit_token(t, T_MINUS);
        t->pos++;
        return true;
    case '*':
        emit_token(t, T_MUL);
        t->pos++;
        return true;
    case '/':
        emit_token(t, T_DIV);
        t->pos++;
        return true;
    case '^':
        emit_token(t, T_POW);
        t->pos++;
        return true;
    case '(':
        emit_token(t, T_LP);
        t->pos++;
        return true;
    case ')':
        emit_token(t, T_RP);
        t->pos++;
        return true;
    case ',':
        emit_token(t, T_COMMA);
        t->pos++;
        return true;
    case ';':
        emit_token(t, T_SEMI);
        t->pos++;
        return true;
    case ':':
        emit_token(t, T_COLON);
        t->pos++;
        return true;
    case '<':
        emit_token(t, T_LT);
        t->pos++;
        return true;
    case '>':
        emit_token(t, T_GT);
        t->pos++;
        return true;
    default:
        return false;
    }
}

/* Tokenize a single line */
bool tokenize_line(const char *line, uint16_t line_num, uint8_t *tokens, int *token_len)
{
    Tokenizer t = {0};
    t.input = line;
    t.pos = 0;
    t.line_num = line_num;
    t.token_len = 0;

    skip_whitespace(&t);

    /* Handle empty line or comment-only line */
    if (t.input[t.pos] == '\0' ||
        (strncasecmp(t.input + t.pos, "REM", 3) == 0 &&
         (t.input[t.pos + 3] == ' ' || t.input[t.pos + 3] == '\0')))
    {

        if (t.input[t.pos] != '\0')
        {
            /* REM statement - store the comment */
            t.pos += 3; /* Skip "REM" */
            skip_whitespace(&t);
            emit_token(&t, T_REM);

            /* Store rest of line as string */
            const char *comment = t.input + t.pos;
            int comment_len = strlen(comment);
            /* Trim trailing whitespace */
            while (comment_len > 0 &&
                   (comment[comment_len - 1] == ' ' || comment[comment_len - 1] == '\t'))
            {
                comment_len--;
            }
            if (comment_len > 0)
            {
                emit_token_string(&t, T_STR, comment, comment_len);
            }
        }

        *token_len = t.token_len;
        memcpy(tokens, t.tokens, t.token_len);
        return true;
    }

    /* Parse tokens until end of line */
    while (t.input[t.pos] != '\0')
    {
        skip_whitespace(&t);
        if (t.input[t.pos] == '\0')
            break;

        bool parsed = false;

        /* Try parsing in order of precedence */
        if (t.input[t.pos] == '"')
        {
            parsed = parse_string(&t);
        }
        else if (is_digit(t.input[t.pos]) || t.input[t.pos] == '.')
        {
            parsed = parse_number(&t);
        }
        else if (is_alpha(t.input[t.pos]))
        {
            parsed = parse_keyword(&t);
        }
        else
        {
            parsed = parse_operator(&t);
        }

        if (!parsed)
        {
            error_report(ERR_SYNTAX_ERROR, line_num);
            return false;
        }
    }

    *token_len = t.token_len;
    memcpy(tokens, t.tokens, t.token_len);
    return true;
}

/* Tokenize a file */
bool tokenize_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        fprintf(stderr, "Cannot open file: %s\n", filename);
        return false;
    }

    char line_buffer[512];
    int lines_loaded = 0;

    while (fgets(line_buffer, sizeof(line_buffer), file))
    {
        /* Remove newline */
        int len = strlen(line_buffer);
        if (len > 0 && line_buffer[len - 1] == '\n')
        {
            line_buffer[len - 1] = '\0';
        }

        /* Skip empty lines */
        if (strlen(line_buffer) == 0)
            continue;

        /* Parse line number */
        char *endptr;
        long line_num = strtol(line_buffer, &endptr, 10);
        if (line_num <= 0 || line_num > LINE_NUM_MAX || endptr == line_buffer)
        {
            fprintf(stderr, "Invalid line number in: %s\n", line_buffer);
            fclose(file);
            return false;
        }

        /* Skip whitespace after line number */
        while (*endptr == ' ' || *endptr == '\t')
        {
            endptr++;
        }

        /* Tokenize the rest of the line */
        uint8_t tokens[TOKBUF_LINE_MAX];
        int token_len;

        if (!tokenize_line(endptr, (uint16_t)line_num, tokens, &token_len))
        {
            fclose(file);
            return false;
        }

        /* Add to program */
        if (!program_add_line((uint16_t)line_num, tokens, token_len))
        {
            fclose(file);
            return false;
        }

        lines_loaded++;
    }

    fclose(file);
    printf("Loaded %d lines\n", lines_loaded);
    return true;
}
