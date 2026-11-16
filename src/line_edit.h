#ifndef LINE_EDIT_H
#define LINE_EDIT_H

#include <stddef.h>

// Initialize line editor (must be called before using other functions)
void line_edit_init(void);

// Read a line character by character in raw mode
// Returns the number of characters read, or -1 on EOF
int line_edit_read_line(char *buffer, size_t buffer_size);

// Cleanup line editor (restore terminal settings)
void line_edit_cleanup(void);

#endif // LINE_EDIT_H
