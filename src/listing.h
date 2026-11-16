#ifndef LISTING_H
#define LISTING_H

#include <stdio.h>
#include "opcodes.h"
#include "program.h"

/* LIST command - display program listing */
void cmd_list(FILE *file);

/* Token disassembly for debugging */
void cmd_dump(FILE *file);

/* Token name lookup */
const char *token_name(Tok token);

/* Map token byte offset to character position in reconstructed source */
int listing_token_offset_to_char_pos(uint8_t *line_ptr, int token_byte_offset);

#endif /* LISTING_H */
