#ifndef PROGRAM_H
#define PROGRAM_H

#include "opcodes.h"
#include <stdint.h>
#include <stdbool.h>

/* Variable cell - can hold either number or string */
typedef enum
{
    VAR_NUM,
    VAR_STR
} VarType;

typedef struct
{
    VarType type;
    union
    {
        double num;
        char str[STR_MAX + 1]; /* +1 for null terminator */
    } value;
} VarCell;

/* Program memory structure */
typedef struct
{
    uint8_t prog[PROG_MAX_BYTES]; /* Token buffer */
    int prog_len;                 /* Current program size */
    VarCell vars[VARS_MAX + 1];   /* Variables 1..VARS_MAX (0 unused) */
} Program;

/* Line record format: u16 len | u16 line | tokens... | T_EOL */
typedef struct
{
    uint16_t len;      /* Total record length including header */
    uint16_t line_num; /* Line number (1-999) */
    uint8_t *tokens;   /* Start of token data */
} LineRecord;

/* Global program state */
extern Program g_program;

/* Program management */
void program_init(void);
void program_clear(void);

/* Line management */
bool program_add_line(uint16_t line_num, const uint8_t *tokens, int token_len);
bool program_delete_line(uint16_t line_num);
LineRecord *program_find_line(uint16_t line_num);
LineRecord *program_first_line(void);
LineRecord *program_next_line(LineRecord *current);

/* Variable management */
void var_init_all(void);
VarCell *var_get(int index); /* 1-based indexing */
void var_set_num(int index, double value);
void var_set_str(int index, const char *value);

/* Token stream utilities */
uint8_t *token_skip(uint8_t *token);             /* Skip one token, return next */
void token_dump(const uint8_t *tokens, int len); /* Debug dump */

#endif /* PROGRAM_H */
