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

/* VM state */
typedef struct
{
    uint8_t *pc;          /* Program counter (token pointer) */
    int current_line;     /* Current line number for error reporting */
    bool running;         /* VM running state */
    ExprStack expr_stack; /* Expression evaluation stack */
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

/* Statement execution */
void vm_execute_statement(void);

/* Global VM state */
extern VM g_vm;

#endif /* VM_H */
