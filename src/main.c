#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opcodes.h"
#include "program.h"
#include "tokenizer.h"
#include "runtime.h"
#include "vm.h"
#include "errors.h"

void print_usage(const char *program_name)
{
    printf("PC-1211 BASIC Interpreter v0.5\n");
    printf("Usage: %s <program.bas> [options]\n", program_name);
    printf("Options:\n");
    printf("  --list    Show program listing\n");
    printf("  --dump    Show token dump (debug)\n");
    printf("  --run     Execute program (not implemented yet)\n");
    printf("  --help    Show this help\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        print_usage(argv[0]);
        return 1;
    }

    /* Parse command line options */
    bool show_list = false;
    bool show_dump = false;
    bool run_program = false;
    const char *filename = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--list") == 0)
        {
            show_list = true;
        }
        else if (strcmp(argv[i], "--dump") == 0)
        {
            show_dump = true;
        }
        else if (strcmp(argv[i], "--run") == 0)
        {
            run_program = true;
        }
        else if (strcmp(argv[i], "--help") == 0)
        {
            print_usage(argv[0]);
            return 0;
        }
        else if (argv[i][0] != '-')
        {
            filename = argv[i];
        }
        else
        {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    if (!filename)
    {
        fprintf(stderr, "No input file specified\n");
        return 1;
    }

    /* Initialize system */
    program_init();
    vm_init();

    printf("PC-1211 BASIC Interpreter v0.5\n");
    printf("Loading: %s\n", filename);

    /* Load and tokenize program */
    if (!tokenize_file(filename))
    {
        fprintf(stderr, "Failed to load program\n");
        return 1;
    }

    /* Execute requested operations */
    if (show_dump)
    {
        printf("\nToken dump:\n");
        disassemble_program();
    }

    if (show_list)
    {
        printf("\nProgram listing:\n");
        cmd_list();
    }

    if (run_program)
    {
        printf("\nExecuting program:\n");
        vm_run();
    }

    /* If no specific action requested, just show that we loaded it */
    if (!show_list && !show_dump && !run_program)
    {
        printf("Program loaded successfully. Use --list to view or --dump for debug info.\n");
    }

    return 0;
}
