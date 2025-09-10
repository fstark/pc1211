#ifndef VM_H
#define VM_H

#include "opcodes.h"
#include <stdbool.h>

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
    uint8_t *return_pc; /* Where to return to */
    int return_line;    /* Line number for error reporting */
} CallFrame;

typedef struct
{
    CallFrame frames[CALL_STACK_SIZE];
    int top;
} CallStack;

/* VM state */
typedef struct
{
    uint8_t *pc;          /* Program counter (token pointer) */
    int current_line;     /* Current line number for error reporting */
    bool running;         /* VM running state */
    ExprStack expr_stack; /* Expression evaluation stack */
    CallStack call_stack; /* GOSUB/RETURN call stack */
} VM;

/* VM initialization and control */
void vm_init(void);
void vm_run(void);
void vm_step(void);
void vm_halt(void);

/* Expression evaluation */
double vm_eval_expression(uint8_t **pc_ptr, uint8_t *end);
void vm_push_value(double value);
double vm_pop_value(void);

/* Comparison operations for IF */
bool vm_eval_condition(uint8_t **pc_ptr, uint8_t *end);

/* Call stack management */
void vm_push_call(uint8_t *return_pc, int return_line);
bool vm_pop_call(uint8_t **return_pc, int *return_line);

/* Statement execution */
void vm_execute_statement(void);

/* Global VM state */
extern VM g_vm;

#endif /* VM_H */
