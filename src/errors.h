#ifndef ERRORS_H
#define ERRORS_H

#include "opcodes.h"
#include <setjmp.h>

/* Error context management */
#define ERROR_CONTEXT_SET(label)                           \
    do                                                     \
    {                                                      \
        if (setjmp(g_error_jump_buf) != 0)                 \
        {                                                  \
            /* Error occurred - jump to context handler */ \
            goto label;                                    \
        }                                                  \
    } while (0)

/* Error reporting */
void error_fatal(ErrorCode code, int line_number); /* Never returns */
const char *error_message(ErrorCode code);

/* Legacy functions for non-fatal errors (parsing, etc.) */
void error_report(ErrorCode code, int line_number);
void error_set(ErrorCode code, int line_number);
ErrorCode error_get_code(void);
void error_clear(void);
void error_print(void);

/* Global jump buffer for error handling */
extern jmp_buf g_error_jump_buf;

/* Error state */
extern ErrorCode g_last_error;
extern int g_error_line;

#endif /* ERRORS_H */
