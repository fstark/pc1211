#ifndef ERRORS_H
#define ERRORS_H

#include "opcodes.h"

/* Error reporting */
void error_report(ErrorCode code, int line_number);
const char *error_message(ErrorCode code);

/* Error state */
extern ErrorCode g_last_error;
extern int g_error_line;

#endif /* ERRORS_H */
