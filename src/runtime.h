#ifndef RUNTIME_H
#define RUNTIME_H

#include "opcodes.h"
#include "program.h"

/* LIST command - display program listing */
void cmd_list(void);
void cmd_list_line(uint16_t line_num);

/* Token disassembly for debugging */
void disassemble_program(void);
void disassemble_line(uint8_t *line_ptr);
void disassemble_tokens(const uint8_t *tokens, int len);

/* Token name lookup */
const char *token_name(Tok token);

#endif /* RUNTIME_H */
