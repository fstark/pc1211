#ifndef VM_H
#define VM_H

#include "opcodes.h"
#include <stdbool.h>

/* VM state (stub for now) */
typedef struct
{
    uint8_t *pc;      /* Program counter (token pointer) */
    int current_line; /* Current line number for error reporting */
    bool running;     /* VM running state */
} VM;

/* VM initialization and control */
void vm_init(void);
void vm_run(void);
void vm_step(void);
void vm_halt(void);

/* Global VM state */
extern VM g_vm;

#endif /* VM_H */
