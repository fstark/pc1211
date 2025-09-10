#include "errors.h"
#include <stdio.h>
#include <stdlib.h>

/* Global error state */
ErrorCode g_last_error = ERR_NONE;
int g_error_line = 0;

/* Error messages */
const char *error_message(ErrorCode code)
{
    switch (code)
    {
    case ERR_NONE:
        return "No error";

    /* Type 1 errors (runtime) */
    case ERR_DIVISION_BY_ZERO:
        return "Division by zero";
    case ERR_MATH_DOMAIN:
        return "Math domain error";
    case ERR_MATH_OVERFLOW:
        return "Math overflow";
    case ERR_INDEX_OUT_OF_RANGE:
        return "Index out of range";
    case ERR_TYPE_MISMATCH:
        return "Type mismatch";
    case ERR_FOR_STEP_ZERO:
        return "FOR step cannot be zero";
    case ERR_RETURN_WITHOUT_GOSUB:
        return "RETURN without GOSUB";
    case ERR_NEXT_WITHOUT_FOR:
        return "NEXT without FOR";

    /* Type 2 errors (structural) */
    case ERR_BAD_LINE_NUMBER:
        return "Bad line number";
    case ERR_SYNTAX_ERROR:
        return "Syntax error";
    case ERR_LINE_TOO_LONG:
        return "Line too long";
    case ERR_PROGRAM_TOO_LARGE:
        return "Program too large";
    case ERR_STACK_OVERFLOW:
        return "Stack overflow";

    default:
        return "Unknown error";
    }
}

/* Report error and halt */
void error_report(ErrorCode code, int line_number)
{
    g_last_error = code;
    g_error_line = line_number;

    fprintf(stderr, "Error %d", (int)code);
    if (line_number > 0)
    {
        fprintf(stderr, " at line %d", line_number);
    }
    fprintf(stderr, ": %s\n", error_message(code));

    exit(1);
}
