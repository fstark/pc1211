#include "line_edit.h"

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "vm.h" /* For vm_set_user_interrupted */

// Special key codes for escape sequences (internal implementation detail)
#define KEY_UP_ARROW 1001
#define KEY_DOWN_ARROW 1002
#define KEY_RIGHT_ARROW 1003
#define KEY_LEFT_ARROW 1004
#define KEY_HOME 1005
#define KEY_END 1006

// History buffer configuration
#define HISTORY_SIZE 10
#define MAX_LINE_LENGTH 256

// Terminal control variables
static struct termios orig_termios;
static bool raw_mode_enabled = false;
static bool initialized = false;

// Command history circular buffer
static char history[HISTORY_SIZE][MAX_LINE_LENGTH];
static int history_count = 0;    // Total number of commands ever added
static int history_current = -1; // Current position in history (-1 = not browsing)

// Signal handler for Ctrl-C
static void sigint_handler(int sig)
{
    (void)sig; // Unused parameter
    vm_set_user_interrupted();
    // Don't call any non-async-safe functions here - just set the flag
}

// Enable raw mode for character-by-character input
static void enable_raw_mode(void)
{
    if (raw_mode_enabled)
        return;

    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
        return;

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_cflag |= (CS8);
    raw.c_oflag &= ~(OPOST);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        return;
    raw_mode_enabled = true;
}

// Disable raw mode and restore original terminal settings
static void disable_raw_mode(void)
{
    if (!raw_mode_enabled)
        return;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    raw_mode_enabled = false;
}

// Add a command to the history buffer
static void add_to_history(const char *line)
{
    // Don't add empty lines or duplicate of last command
    if (strlen(line) == 0)
        return;

    int last_index = (history_count - 1) % HISTORY_SIZE;
    if (history_count > 0 && strcmp(history[last_index], line) == 0)
        return;

    // Add to circular buffer
    int index = history_count % HISTORY_SIZE;
    strncpy(history[index], line, MAX_LINE_LENGTH - 1);
    history[index][MAX_LINE_LENGTH - 1] = '\0';
    history_count++;
}

// Get command from history at relative position (0 = most recent, 1 = one older, etc.)
static const char *get_from_history(int relative_pos)
{
    if (history_count == 0 || relative_pos < 0)
        return NULL;

    int available = (history_count < HISTORY_SIZE) ? history_count : HISTORY_SIZE;
    if (relative_pos >= available)
        return NULL;

    // Calculate actual index in circular buffer
    int actual_index;
    if (history_count < HISTORY_SIZE)
    {
        actual_index = history_count - 1 - relative_pos;
    }
    else
    {
        actual_index = (history_count - 1 - relative_pos) % HISTORY_SIZE;
    }

    return history[actual_index];
}

// Clear the current line on screen and reset cursor
static void clear_line(void)
{
    // Move cursor to beginning of line first
    printf("\r");
    // Clear to end of line (this clears everything after cursor)
    printf("\033[K");
}

// Display a line and set buffer contents
static void display_line(char *buffer, const char *line, size_t *pos, size_t *cursor_pos)
{
    strncpy(buffer, line, MAX_LINE_LENGTH - 1);
    buffer[MAX_LINE_LENGTH - 1] = '\0';
    *pos = strlen(buffer);
    *cursor_pos = *pos;

    printf(">%s", buffer); // Include the prompt since we cleared the entire line
}

void line_edit_init(void)
{
    if (initialized)
        return;

    // Register cleanup function to restore terminal on exit
    atexit(line_edit_cleanup);

    // Install Ctrl-C signal handler
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    initialized = true;
}

// Read escape sequence (for arrow keys, etc.)
static int read_escape_sequence(void)
{
    int c1 = getchar();
    if (c1 != '[')
        return -1; // Not a standard escape sequence

    int c2 = getchar();
    switch (c2)
    {
    case 'A':
        return KEY_UP_ARROW;
    case 'B':
        return KEY_DOWN_ARROW;
    case 'C':
        return KEY_RIGHT_ARROW;
    case 'D':
        return KEY_LEFT_ARROW;
    case 'H':
        return KEY_HOME;
    case 'F':
        return KEY_END;
    default:
        return -1; // Unknown sequence
    }
}

// Read a line character by character in raw mode
int line_edit_read_line(char *buffer, size_t buffer_size)
{
    size_t pos = 0;
    size_t cursor_pos = 0; // Position of cursor in buffer
    int c;
    char temp_buffer[MAX_LINE_LENGTH]; // Temporary buffer for history navigation
    bool in_history = false;

    enable_raw_mode();

    // Reset history navigation
    history_current = -1;

    while (pos < buffer_size - 1)
    {
        c = getchar();

        if (c == EOF)
        {
            disable_raw_mode();
            return -1; // EOF
        }

        if (c == 3) // Ctrl-C
        {
            printf("^C\n");
            buffer[0] = '\0';
            pos = 0;
            disable_raw_mode();
            return 0;
        }

        if (c == '\r' || c == '\n')
        {
            printf("\r\n");

            // Add to history if not empty
            if (pos > 0)
            {
                buffer[pos] = '\0';
                add_to_history(buffer);
            }
            break;
        }

        if (c == 127 || c == 8) // Backspace or DEL
        {
            if (cursor_pos > 0)
            {
                // Move characters after cursor position one position left
                for (size_t i = cursor_pos - 1; i < pos - 1; i++)
                {
                    buffer[i] = buffer[i + 1];
                }
                pos--;
                cursor_pos--;

                // Redraw line from cursor position
                printf("\b");
                for (size_t i = cursor_pos; i < pos; i++)
                {
                    printf("%c", buffer[i]);
                }
                printf(" \b"); // Clear the last character

                // Move cursor back to correct position
                for (size_t i = cursor_pos; i < pos; i++)
                {
                    printf("\b");
                }
            }
            continue;
        }

        if (c == 27) // ESC - start of escape sequence
        {
            int seq = read_escape_sequence();
            switch (seq)
            {
            case KEY_UP_ARROW:
            {
                const char *hist_line;
                if (history_current == -1)
                {
                    // First time entering history - save current line
                    if (pos > 0)
                    {
                        strncpy(temp_buffer, buffer, pos);
                        temp_buffer[pos] = '\0';
                    }
                    else
                    {
                        temp_buffer[0] = '\0';
                    }
                    history_current = 0;
                }
                else
                {
                    history_current++;
                }

                hist_line = get_from_history(history_current);
                if (hist_line != NULL)
                {
                    clear_line(); // Clear entire current line
                    display_line(buffer, hist_line, &pos, &cursor_pos);
                    in_history = true;
                }
                else
                {
                    // Hit the end of history, stay at last valid position
                    if (history_current > 0)
                        history_current--;
                }
                break;
            }
            case KEY_DOWN_ARROW:
            {
                if (history_current > 0)
                {
                    history_current--;
                    const char *hist_line = get_from_history(history_current);
                    if (hist_line != NULL)
                    {
                        clear_line(); // Clear entire current line
                        display_line(buffer, hist_line, &pos, &cursor_pos);
                    }
                }
                else if (history_current == 0)
                {
                    // Return to the temporary buffer (what user was typing)
                    history_current = -1;
                    clear_line(); // Clear entire current line
                    if (strlen(temp_buffer) > 0)
                    {
                        display_line(buffer, temp_buffer, &pos, &cursor_pos);
                    }
                    else
                    {
                        pos = 0;
                        cursor_pos = 0;
                        printf(">"); // Just show the prompt for empty line
                    }
                    in_history = false;
                }
                break;
            }
            case KEY_LEFT_ARROW:
                if (cursor_pos > 0)
                {
                    cursor_pos--;
                    printf("\b");
                }
                break;
            case KEY_RIGHT_ARROW:
                if (cursor_pos < pos)
                {
                    printf("%c", buffer[cursor_pos]);
                    cursor_pos++;
                }
                break;
            case KEY_HOME:
                while (cursor_pos > 0)
                {
                    cursor_pos--;
                    printf("\b");
                }
                break;
            case KEY_END:
                while (cursor_pos < pos)
                {
                    printf("%c", buffer[cursor_pos]);
                    cursor_pos++;
                }
                break;
            default:
                break;
            }
            continue;
        }

        if (c >= 32 && c < 127) // Printable characters
        {
            // If we were browsing history and user types, reset to current editing
            if (in_history)
            {
                history_current = -1;
                in_history = false;
            }

            // Insert character at cursor position
            if (cursor_pos < pos)
            {
                // Move characters after cursor position one position right
                for (size_t i = pos; i > cursor_pos; i--)
                {
                    buffer[i] = buffer[i - 1];
                }
            }

            buffer[cursor_pos] = c;
            pos++;

            // Redraw from cursor position to end
            for (size_t i = cursor_pos; i < pos; i++)
            {
                printf("%c", buffer[i]);
            }

            cursor_pos++;

            // Move cursor back to correct position if we inserted in middle
            for (size_t i = cursor_pos; i < pos; i++)
            {
                printf("\b");
            }
        }
    }

    buffer[pos] = '\0';
    disable_raw_mode();
    return pos;
}

void line_edit_cleanup(void)
{
    disable_raw_mode();
}
