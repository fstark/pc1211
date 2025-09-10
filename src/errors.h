#ifndef ERRORS_H
#define ERRORS_H

#include "opcodes.h"

/* Error reporting */
void error_report(ErrorCode code, int line_number);
const char *error_message(ErrorCode code);

/* Error state management */
void error_set(ErrorCode code, int line_number);
ErrorCode error_get_code(void);
void error_clear(void);
void error_print(void);

/* Error state */
extern ErrorCode g_last_error;
extern int g_error_line;

#endif /* ERRORS_H */
