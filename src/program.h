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

/* Label table entry */
typedef struct
{
    char label[STR_MAX + 1]; /* Label string */
    uint16_t line_num;       /* Line number where label is defined */
} LabelEntry;

/* Program memory structure */
typedef struct
{
    uint8_t prog[PROG_MAX_BYTES]; /* Token buffer */
    int prog_len;                 /* Current program size */
    VarCell vars[VARS_MAX + 1];   /* Variables 1..VARS_MAX (0 unused) */
} Program;

/* Line record format: u16 len | u16 line | tokens... | T_EOL */
/* Lines are accessed via uint8_t* pointers to the start of the record */

/* Helper functions to access line record fields */
static inline uint16_t get_len(uint8_t *line_ptr) { return *(uint16_t *)line_ptr; }
static inline uint16_t get_line(uint8_t *line_ptr) { return *(uint16_t *)(line_ptr + 2); }
static inline uint8_t *get_tokens(uint8_t *line_ptr) { return line_ptr + 4; }

/* Global program state */
extern Program g_program;

/* Program management */
void program_init(void);
void program_clear(void);

/* Line management */
bool program_add_line(uint16_t line_num, const uint8_t *tokens, int token_len);
bool program_delete_line(uint16_t line_num);

/* Label management */
void program_add_label(const char *label, uint16_t line_num);
uint8_t *program_find_line_label(const char *label);
uint8_t *program_find_line(uint16_t line_num);
uint8_t *program_find_first_line_after(uint16_t target_line);
uint8_t *program_first_line(void);
void program_validate_line_ptr(uint8_t *line_ptr);
bool program_is_last_line(uint8_t *line_ptr);
uint8_t *program_next_line(uint8_t *current_line);

/* Variable management */
void var_init_all(void);
VarCell *var_get(int index); /* 1-based indexing */
void var_set_num(int index, double value);
void var_set_str(int index, const char *value);

/* Token stream utilities */
bool program_validate_token_ptr(uint8_t *token_ptr);
uint8_t *token_skip(uint8_t *token);             /* Skip one token, return next */
void token_dump(const uint8_t *tokens, int len); /* Debug dump */

/* VM helper functions */
uint8_t *program_find_line_tokens(uint16_t line_num);  /* Get token start for line */
uint8_t *program_find_line_end(uint8_t *tokens);       /* Find end of current line */
uint8_t *program_find_line_end_from_pos(uint8_t *pos); /* Find end from any position within line */
uint8_t *program_first_line_tokens(void);              /* Get first line tokens */
uint8_t *program_next_line_tokens(uint8_t *current);   /* Get next line tokens */

#endif /* PROGRAM_H */
