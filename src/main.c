#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include "opcodes.h"
#include "program.h"
#include "tokenizer.h"
#include "listing.h"
#include "vm.h"
#include "errors.h"
#include "interactive.h"
#include "line_edit.h"

void print_usage(const char *program_name)
{
    printf("PC-1211 BASIC Interpreter v0.5\n");
    printf("Usage: %s <program.bas> [options]\n", program_name);
    printf("Options:\n");
    printf("  --list           Show program listing\n");
    printf("  --dump           Show token dump (debug)\n");
    printf("  --run            Execute program\n");
    printf("  --help           Show this help\n");
}

/* Signal handler for Ctrl-C during program execution */
static void sigint_handler(int sig)
{
    (void)sig;
    vm_set_user_interrupted();
}

int main(int argc, char *argv[])
{
    /* Register cleanup function to restore terminal on exit */
    atexit(line_edit_cleanup);

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

    /* Initialize system */
    program_init();
    vm_init();

    /* Install signal handler for Ctrl-C during program execution */
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    if (!show_list && !show_dump && !run_program)
    {
        interactive_repl();
        return 0;
    }

    if (!filename)
    {
        fprintf(stderr, "No input file specified\n");
        return 1;
    }

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
        cmd_dump(stdout);
    }

    if (show_list)
    {
        printf("\nProgram listing:\n");
        cmd_list(stdout);
    }

    if (run_program)
    {
        printf("\nExecuting program:\n");
        vm_run();

        /* Return non-zero exit code if there were errors during execution */
        if (error_get_code() != ERR_NONE)
        {
            return 1;
        }
    }

    /* If no specific action requested, just show that we loaded it */
    if (!show_list && !show_dump && !run_program)
    {
        printf("Program loaded successfully. Use --list to view or --dump for debug info.\n");
    }

    return 0;
}
