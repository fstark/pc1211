#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "opcodes.h"
#include <stdint.h>
#include <stdbool.h>

/* Keyword table entry */
typedef struct
{
    const char *name;   /* Full name (e.g., "SIN") */
    const char *abbrev; /* Abbreviation (e.g., "SI.") or NULL */
    Tok token;          /* Token value */
} Keyword;

/* Tokenizer state */
typedef struct
{
    const char *input;               /* Input string */
    int pos;                         /* Current position */
    int line_num;                    /* Current line number being parsed */
    uint8_t tokens[TOKBUF_LINE_MAX]; /* Output token buffer */
    int token_len;                   /* Current token buffer length */
} Tokenizer;

/* Main tokenization functions */
bool tokenize_line(const char *line, uint16_t line_num, uint8_t *tokens, int *token_len);
bool tokenize_file(const char *filename);

/* Individual token parsing */
bool parse_number(Tokenizer *t);
bool parse_string(Tokenizer *t);
bool parse_keyword(Tokenizer *t);
bool parse_variable(Tokenizer *t);
bool parse_operator(Tokenizer *t);

/* Utility functions */
void skip_whitespace(Tokenizer *t);
bool is_alpha(char c);
bool is_digit(char c);
bool is_alnum(char c);
const Keyword *find_keyword(const char *word);

/* Token emission */
void emit_token(Tokenizer *t, Tok token);
void emit_token_u8(Tokenizer *t, Tok token, uint8_t data);
void emit_token_u16(Tokenizer *t, Tok token, uint16_t data);
void emit_token_double(Tokenizer *t, Tok token, double data);
void emit_token_string(Tokenizer *t, Tok token, const char *str, int len);
void emit_token_string_unrestricted(Tokenizer *t, Tok token, const char *str, int len);

#endif /* TOKENIZER_H */
