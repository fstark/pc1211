#include "vm.h"
#include "program.h"
#include "errors.h"
#include <stdio.h>

/* Global VM state */
VM g_vm;

/* Initialize VM */
void vm_init(void)
{
    g_vm.pc = NULL;
    g_vm.current_line = 0;
    g_vm.running = false;
}

/* Run program (stub for now) */
void vm_run(void)
{
    printf("VM execution not implemented yet (Phase 2)\n");
}

/* Single step (stub for now) */
void vm_step(void)
{
    printf("VM stepping not implemented yet (Phase 2)\n");
}

/* Halt VM */
void vm_halt(void)
{
    g_vm.running = false;
}
