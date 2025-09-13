#ifndef VM_H
#define VM_H

#include "opcodes.h"
#include "program.h"
#include <stdbool.h>

/* VM position for capturing and restoring PC+line state */
typedef struct
{
    uint8_t *line_ptr; //  Start of the line
    uint8_t *pc;       //  PC
} VMPosition;

/* Expression evaluation stack */
#define EXPR_STACK_SIZE 32
typedef struct
{
    double values[EXPR_STACK_SIZE];
    int top;
} ExprStack;

/* GOSUB/RETURN call stack */
#define CALL_STACK_SIZE 16
typedef struct
{
    VMPosition return_pos; /* Where to return to */
} CallFrame;

typedef struct
{
    CallFrame frames[CALL_STACK_SIZE];
    int top;
} CallStack;

/* FOR/NEXT loop stack */
#define FOR_STACK_SIZE 16
typedef struct
{
    VMPosition body; /* Position after the FOR statement */
    uint8_t var_idx; /* Loop variable index (1-26 for A-Z) */
    double limit;    /* TO value */
    double step;     /* STEP value */
} ForFrame;

typedef struct
{
    ForFrame frames[FOR_STACK_SIZE];
    int top;
} ForStack;

/* Angle modes for trigonometric functions */
typedef enum
{
    ANGLE_RADIAN = 0, /* Default mode */
    ANGLE_DEGREE = 1,
    ANGLE_GRAD = 2
} AngleMode;

/* VM state */
typedef struct
{
    bool running;              /* VM running state */
    uint8_t *pc;               /* Program counter (token pointer) */
    uint8_t *current_line_ptr; /* Current line pointer */
    ExprStack expr_stack;      /* Expression evaluation stack */
    CallStack call_stack;      /* GOSUB/RETURN call stack */
    ForStack for_stack;        /* FOR/NEXT loop stack */

    /* AREAD state */
    char aread_string[8]; /* AREAD string value */
    double aread_value;   /* AREAD numeric value */
    bool aread_is_string; /* Whether AREAD value is a string */

    /* Modes */
    AngleMode angle_mode; /* Trigonometric angle mode */
} VM;

/* VM initialization and control */
void vm_init(void);
void vm_run(void);
void vm_step(void);
void vm_halt(void);

/* Error handling */
void vm_error_set(ErrorCode code);
void vm_error_if(bool condition, ErrorCode code);

/* Expression evaluation */
double vm_eval_expression_auto(uint8_t **pc_ptr); /* Token-aware, no boundaries */
void vm_push_value(double value);
double vm_pop_value(void);

/* Comparison operations for IF */
bool vm_eval_condition(uint8_t **pc_ptr, uint8_t *end);

/* Call stack management */
void vm_push_call(VMPosition return_pos);
bool vm_pop_call(VMPosition *return_pos);

/* FOR loop stack management */
void vm_push_for(VMPosition body, uint8_t var_idx, double limit, double step);
bool vm_pop_for(VMPosition *body, uint8_t *var_idx, double *limit, double *step);
bool vm_find_for_by_var(uint8_t var_idx, int *frame_index);

/* Statement execution */
void vm_execute_statement(void);

/* Angle conversion functions */
double convert_angle_to_radians(double angle);
double convert_angle_from_radians(double radians);

/* Global VM state */
extern VM g_vm;

#endif /* VM_H */
