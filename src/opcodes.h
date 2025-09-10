#ifndef OPCODES_H
#define OPCODES_H

#include <stdint.h>

/* PC-1211 BASIC Token Opcodes (v0.5) */
typedef enum
{
    /* Core tokens */
    T_EOL = 0x00,   /* end of line record */
    T_NUM = 0x01,   /* <8 bytes double> */
    T_STR = 0x02,   /* <u8 n> <n bytes> (0..7) */
    T_VAR = 0x03,   /* <u8 1..26> (A..Z) - numeric variable */
    T_VIDX = 0x04,  /* A(expr); expr follows, terminated by T_ENDX */
    T_SVAR = 0x05,  /* <u8 1..26> (A$..Z$) - string variable */
    T_SVIDX = 0x06, /* A$(expr); expr follows, terminated by T_ENDX */
    T_ENDX = 0xFF,  /* terminator for T_VIDX/T_SVIDX inline expression */

    /* Operators & punctuation */
    T_EQ_ASSIGN = 0x10, /* = assignment */
    T_PLUS = 0x11,      /* + */
    T_MINUS = 0x12,     /* - */
    T_MUL = 0x13,       /* * */
    T_DIV = 0x14,       /* / */
    T_POW = 0x15,       /* ^ */

    T_LP = 0x16,    /* ( */
    T_RP = 0x17,    /* ) */
    T_COMMA = 0x18, /* , */
    T_SEMI = 0x19,  /* ; */
    T_COLON = 0x1A, /* : */

    T_EQ = 0x1B, /* = for IF */
    T_NE = 0x1C, /* <> */
    T_LT = 0x1D, /* < */
    T_LE = 0x1E, /* <= */
    T_GT = 0x1F, /* > */
    T_GE = 0x20, /* >= */

    /* Functions (all numeric) */
    T_SIN = 0x30, /* SIN (SI.) */
    T_COS = 0x31, /* COS */
    T_TAN = 0x32, /* TAN (TA.) */
    T_ASN = 0x33, /* ASN (AS.) - asin */
    T_ACS = 0x34, /* ACS (AC.) - acos */
    T_ATN = 0x35, /* ATN (AT.) - atan */
    T_LOG = 0x36, /* LOG (LO.) - base 10 */
    T_LN = 0x37,  /* LN - natural log */
    T_EXP = 0x38, /* EXP (EX.) */
    T_SQR = 0x39, /* SQR - square root (√ on card) */
    T_DMS = 0x3A, /* DMS (DM.) - decimal→sexagesimal */
    T_DEG = 0x3B, /* DEG - sexagesimal→decimal */
    T_INT = 0x3C, /* INT - truncate toward 0 */
    T_ABS = 0x3D, /* ABS (AB.) - absolute value */
    T_SGN = 0x3E, /* SGN (SG.) - sign function */

    /* Statements */
    T_LET = 0x40,    /* LET (LE.) */
    T_PRINT = 0x41,  /* PRINT (P.) */
    T_INPUT = 0x42,  /* INPUT (I.) */
    T_IF = 0x43,     /* IF */
    T_THEN = 0x44,   /* THEN (T.) */
    T_GOTO = 0x45,   /* GOTO (G.) */
    T_GOSUB = 0x46,  /* GOSUB (GOS.) */
    T_RETURN = 0x47, /* RETURN (RE.) */
    T_FOR = 0x48,    /* FOR (F.) */
    T_TO = 0x49,     /* TO */
    T_STEP = 0x4A,   /* STEP (STE.) */
    T_NEXT = 0x4B,   /* NEXT (N.) */
    T_END = 0x4C,    /* END (E.) */
    T_STOP = 0x4D,   /* STOP (S.) */
    T_REM = 0x4E,    /* REM */

    /* Mode & device commands (stubs/no-ops) */
    T_DEGREE = 0x50, /* DEGREE (DEG.) */
    T_RADIAN = 0x51, /* RADIAN (RA.) */
    T_GRAD = 0x52,   /* GRAD */
    T_CLEAR = 0x53,  /* CLEAR (CL.) */
    T_BEEP = 0x54,   /* BEEP (B.) */
    T_PAUSE = 0x55,  /* PAUSE (PA.) */
    T_AREAD = 0x56,  /* AREAD (A.) */
    T_USING = 0x57   /* USING (U.) */
} Tok;

/* Static memory limits */
enum
{
    PROG_MAX_BYTES = 2048, /* Program storage (per clarifications) */
    LINES_MAX = 1024,      /* Maximum line records */
    VARS_MAX = 512,        /* A(n) range: 1..VARS_MAX (A..Z = 1..26) */
    STR_MAX = 7,           /* Maximum string length */
    GOSUB_MAX = 32,        /* GOSUB stack depth */
    FOR_MAX = 64,          /* FOR loop nesting depth */
    EXPR_VAL_MAX = 64,     /* Expression evaluation stack */
    TOKBUF_LINE_MAX = 256, /* Maximum tokens per line */
    LINE_NUM_MAX = 999,    /* Maximum line number (1-999) */
    LABELS_MAX = 100       /* Maximum number of labels */
};

/* Error codes */
typedef enum
{
    ERR_NONE = 0,
    /* Type 1 errors (runtime) */
    ERR_TYPE1_BASE = 1,
    ERR_DIVISION_BY_ZERO = 1,
    ERR_MATH_DOMAIN = 2, /* SQR(-1), LOG(0), etc. */
    ERR_MATH_OVERFLOW = 3,
    ERR_INDEX_OUT_OF_RANGE = 4, /* A(n) with n<1 or n>VARS_MAX */
    ERR_TYPE_MISMATCH = 5,      /* Mixed string/number comparison */
    ERR_FOR_STEP_ZERO = 6,
    ERR_RETURN_WITHOUT_GOSUB = 7,
    ERR_NEXT_WITHOUT_FOR = 8,

    /* Type 2 errors (structural) */
    ERR_TYPE2_BASE = 10,
    ERR_BAD_LINE_NUMBER = 10, /* GOTO to non-existent line */
    ERR_SYNTAX_ERROR = 11,
    ERR_LINE_TOO_LONG = 12,
    ERR_PROGRAM_TOO_LARGE = 13,
    ERR_STACK_OVERFLOW = 14
} ErrorCode;

#endif /* OPCODES_H */
