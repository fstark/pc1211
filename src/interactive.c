#include "interactive.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "vm.h"
#include "program.h"
#include "errors.h"
#include "tokenizer.h"
#include "opcodes.h"
#include "listing.h"
#include "line_edit.h"

void cmd_run(void)
{
    vm_run();

    /* Check if there were any errors during execution */
    if (error_get_code() != ERR_NONE)
    {
        printf("ERROR:\n");
        error_print();
        error_clear();
    }
}

void cmd_new(void)
{
    program_clear();
    printf("Program cleared.\n");
}

void cmd_load(const char *filename)
{
    if (!filename || strlen(filename) == 0)
    {
        printf("Usage: LOAD <filename>\n");
        return;
    }

    program_clear(); /* Clear current program first */

    if (tokenize_file(filename))
    {
        printf("Program loaded from %s\n", filename);
    }
    else
    {
        printf("Error loading file: %s\n", filename);
    }
}

void cmd_save(const char *filename)
{
    if (!filename || strlen(filename) == 0)
    {
        printf("Usage: SAVE <filename>\n");
        return;
    }

    FILE *file = fopen(filename, "w");
    if (!file)
    {
        printf("Error: Cannot create file %s\n", filename);
        return;
    }

    cmd_list(file);
    fclose(file);
    printf("Program saved to %s\n", filename);
}

void interactive_add_line(const char *line)
{
    // Parse line number from the beginning of the line
    char *endptr;
    long line_num = strtol(line, &endptr, 10);
    if (line_num <= 0 || line_num > LINE_NUM_MAX || endptr == line)
    {
        printf("Invalid line number in: %s\n", line);
        return;
    }

    // Skip whitespace after line number
    while (*endptr == ' ' || *endptr == '\t')
    {
        endptr++;
    }

    // Set source context for error reporting
    int content_offset = (int)(endptr - line);
    error_set_source(line, content_offset);

    // Tokenize the rest of the line (after line number)
    uint8_t tokens[256];
    int token_len = 0;
    if (tokenize_line(endptr, (uint16_t)line_num, tokens, &token_len))
    {
        error_clear_source();
        if (token_len > 0)
        {
            // Add line to program with the extracted line number
            if (!program_add_line((uint16_t)line_num, tokens, token_len))
            {
                error_print();
                error_clear();
            }
        }
        else
        {
            printf("No tokens found.\n");
        }
    }
    else
    {
        printf("Error tokenizing line.\n");
        error_print();
        error_clear();
    }
}

void interactive_exec(const char *line)
{
    if (line[0] == '\0')
        return;

    if (line[0] >= '0' && line[0] <= '9')
    {
        interactive_add_line(line);
        return;
    }

    // Extract first word (command or keyword) and optional argument
    char command[32] = {0};
    char argument[256] = {0};
    int i = 0;

    // Extract command
    while (line[i] && !isspace((unsigned char)line[i]) && i < (int)(sizeof(command) - 1))
    {
        command[i] = line[i];
        i++;
    }
    command[i] = '\0';

    // Skip whitespace
    while (line[i] && isspace((unsigned char)line[i]))
    {
        i++;
    }

    // Extract argument
    int arg_start = i;
    while (line[i] && !isspace((unsigned char)line[i]) && (i - arg_start) < (int)(sizeof(argument) - 1))
    {
        argument[i - arg_start] = line[i];
        i++;
    }
    argument[i - arg_start] = '\0';

    if (strcmp(command, "NEW") == 0)
    {
        cmd_new();
        return;
    }

    if (strcmp(command, "LOAD") == 0)
    {
        cmd_load(argument);
        return;
    }

    // Check for SAVE command
    if (strcmp(command, "SAVE") == 0)
    {
        cmd_save(argument);
        return;
    }

    // Check for LIST command
    if (strcmp(command, "LIST") == 0)
    {
        cmd_list(stdout);
        return;
    }

    if (strcmp(command, "DUMP") == 0)
    {
        cmd_dump(stdout);
        return;
    }

    if (strcmp(command, "RUN") == 0)
    {
        cmd_run();
        return;
    }

    if (strcmp(command, "EXIT") == 0)
    {
        line_edit_cleanup();
        exit(0);
    }

    printf("Unknown command: %s\n", command);
}

void interactive_cleanup(void)
{
    line_edit_cleanup();
}

void interactive_repl(void)
{
    char line[256];

    // Initialize line editor
    line_edit_init();

    printf("PC-1211 BASIC Interpreter v0.5\n");
    printf("Type 'EXIT' to quit.\n");

    while (true)
    {
        printf(">");
        if (line_edit_read_line(line, sizeof(line)) == -1)
        {
            break; // EOF
        }

        // Convert line to uppercase
        for (int i = 0; line[i]; i++)
        {
            if (line[i] >= 'a' && line[i] <= 'z')
            {
                line[i] = line[i] - 'a' + 'A';
            }
        }

        interactive_exec(line);
    }
}
