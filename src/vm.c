#include "vm.h"
#include "program.h"
#include "errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>

/* Global VM state */
VM g_vm;
char g_aread_string[8] = "";    /* AREAD string value */
double g_aread_value = 0.0;     /* AREAD numeric value */
bool g_aread_is_string = false; /* Whether AREAD value is a string */

/* Initialize VM */
void vm_init(void)
{
    g_vm.pc = NULL;
    g_vm.current_line = 0;
    g_vm.running = false;
    g_vm.angle_mode = ANGLE_RADIAN; /* Default to radians */
    g_vm.expr_stack.top = 0;
    g_vm.call_stack.top = 0;
    g_vm.for_stack.top = 0;
    g_vm.current_line_ptr = NULL;
}

/* Core position management functions */

/* Capture current position for stack storage */
static VMPosition vm_capture_position(void)
{
    VMPosition pos;
    pos.pc = g_vm.pc;
    pos.line = g_vm.current_line;
    return pos;
}

/* Restore position from captured state - this is the ONLY function that changes PC+line */
static void vm_restore_position(VMPosition pos)
{
    g_vm.pc = pos.pc;
    g_vm.current_line = pos.line;

    /* Update current_line_ptr to match the line */
    if (pos.line > 0)
    {
        g_vm.current_line_ptr = program_find_line(pos.line);
    }
    else
    {
        g_vm.current_line_ptr = NULL;
    }
}

/* Start program at first line */
static void vm_start_program(void)
{
    g_vm.current_line_ptr = program_first_line();
    if (g_vm.current_line_ptr)
    {
        VMPosition pos;
        pos.pc = get_tokens(g_vm.current_line_ptr);
        pos.line = get_line(g_vm.current_line_ptr);
        vm_restore_position(pos);
        g_vm.running = true;
    }
    else
    {
        g_vm.running = false;
    }
}

/* Advance to next line or end program */
static void vm_next_line(void)
{
    if (!g_vm.current_line_ptr)
    {
        g_vm.running = false;
        return;
    }

    if (program_is_last_line(g_vm.current_line_ptr))
    {
        g_vm.running = false;
        return;
    }

    g_vm.current_line_ptr = program_next_line(g_vm.current_line_ptr);
    VMPosition pos;
    pos.pc = get_tokens(g_vm.current_line_ptr);
    pos.line = get_line(g_vm.current_line_ptr);
    vm_restore_position(pos);
}

/* Go to specific line number - scans to find line */
static void vm_goto_line(uint16_t target_line)
{
    uint8_t *line_ptr = program_find_line(target_line);
    if (!line_ptr)
    {
        error_set(ERR_BAD_LINE_NUMBER, g_vm.current_line);
        return;
    }

    VMPosition pos;
    pos.pc = get_tokens(line_ptr);
    pos.line = get_line(line_ptr);
    vm_restore_position(pos);
}

/* Go to label - looks up label then goes to line */
static void vm_goto_label(const char *label)
{
    uint16_t target_line = program_find_label(label);
    if (target_line == 0)
    {
        error_set(ERR_BAD_LINE_NUMBER, g_vm.current_line);
        return;
    }
    vm_goto_line(target_line);
}

/* Push value onto expression stack */
void vm_push_value(double value)
{
    if (g_vm.expr_stack.top >= EXPR_STACK_SIZE)
    {
        error_set(ERR_STACK_OVERFLOW, g_vm.current_line);
        return;
    }
    g_vm.expr_stack.values[g_vm.expr_stack.top++] = value;
}

/* Pop value from expression stack */
double vm_pop_value(void)
{
    assert(g_vm.expr_stack.top > 0); /* Stack underflow is a programming error */
    return g_vm.expr_stack.values[--g_vm.expr_stack.top];
}

/* Push call frame onto call stack */
void vm_push_call(VMPosition return_pos)
{
    if (g_vm.call_stack.top >= CALL_STACK_SIZE)
    {
        error_set(ERR_STACK_OVERFLOW, g_vm.current_line);
        return;
    }
    g_vm.call_stack.frames[g_vm.call_stack.top].return_pos = return_pos;
    g_vm.call_stack.top++;
}

/* Pop call frame from call stack */
bool vm_pop_call(VMPosition *return_pos)
{
    if (g_vm.call_stack.top <= 0)
    {
        error_set(ERR_RETURN_WITHOUT_GOSUB, g_vm.current_line);
        return false;
    }
    g_vm.call_stack.top--;
    *return_pos = g_vm.call_stack.frames[g_vm.call_stack.top].return_pos;
    return true;
}

/* Push FOR frame onto FOR stack */
void vm_push_for(VMPosition pc_after_for, uint8_t var_idx, double limit, double step)
{
    if (g_vm.for_stack.top >= FOR_STACK_SIZE)
    {
        error_set(ERR_STACK_OVERFLOW, g_vm.current_line);
        return;
    }
    g_vm.for_stack.frames[g_vm.for_stack.top].pc_after_for = pc_after_for;
    g_vm.for_stack.frames[g_vm.for_stack.top].var_idx = var_idx;
    g_vm.for_stack.frames[g_vm.for_stack.top].limit = limit;
    g_vm.for_stack.frames[g_vm.for_stack.top].step = step;
    g_vm.for_stack.top++;
}

/* Pop FOR frame from FOR stack */
bool vm_pop_for(VMPosition *pc_after_for, uint8_t *var_idx, double *limit, double *step)
{
    if (g_vm.for_stack.top <= 0)
    {
        error_set(ERR_NEXT_WITHOUT_FOR, g_vm.current_line);
        return false;
    }
    g_vm.for_stack.top--;
    *pc_after_for = g_vm.for_stack.frames[g_vm.for_stack.top].pc_after_for;
    *var_idx = g_vm.for_stack.frames[g_vm.for_stack.top].var_idx;
    *limit = g_vm.for_stack.frames[g_vm.for_stack.top].limit;
    *step = g_vm.for_stack.frames[g_vm.for_stack.top].step;
    return true;
}

/* Find FOR frame by variable index (for named NEXT) */
bool vm_find_for_by_var(uint8_t var_idx, int *frame_index)
{
    /* Search from top of stack downward */
    for (int i = g_vm.for_stack.top - 1; i >= 0; i--)
    {
        if (g_vm.for_stack.frames[i].var_idx == var_idx)
        {
            *frame_index = i;
            return true;
        }
    }
    return false;
}

/* Angle conversion functions for trigonometric operations */
double convert_angle_to_radians(double angle)
{
    switch (g_vm.angle_mode)
    {
    case ANGLE_DEGREE:
        return angle * (M_PI / 180.0);
    case ANGLE_GRAD:
        return angle * (M_PI / 200.0);
    case ANGLE_RADIAN:
    default:
        return angle;
    }
}

double convert_angle_from_radians(double radians)
{
    switch (g_vm.angle_mode)
    {
    case ANGLE_DEGREE:
        return radians * (180.0 / M_PI);
    case ANGLE_GRAD:
        return radians * (200.0 / M_PI);
    case ANGLE_RADIAN:
    default:
        return radians;
    }
}

/* Token-aware expression evaluation */
static double eval_expression_auto(uint8_t **pc_ptr);
static double eval_term_auto(uint8_t **pc_ptr);
static double eval_power_auto(uint8_t **pc_ptr);
static double eval_factor_auto(uint8_t **pc_ptr);

/* Token-aware expression evaluation (no boundaries needed) */
double vm_eval_expression_auto(uint8_t **pc_ptr)
{
    return eval_expression_auto(pc_ptr);
}

/* Token-aware expression evaluation (no explicit boundaries needed) */

/* Evaluate expression: term ((+|-) term)* */
static double eval_expression_auto(uint8_t **pc_ptr)
{
    double result = eval_term_auto(pc_ptr);

    while (true)
    {
        uint8_t op = **pc_ptr;
        if (op == T_PLUS)
        {
            (*pc_ptr)++;
            result += eval_term_auto(pc_ptr);
        }
        else if (op == T_MINUS)
        {
            (*pc_ptr)++;
            result -= eval_term_auto(pc_ptr);
        }
        else
        {
            break; /* Stop at any other token */
        }
    }

    return result;
}

/* Evaluate term: power ((*|/) power)* */
static double eval_term_auto(uint8_t **pc_ptr)
{
    double result = eval_power_auto(pc_ptr);

    while (true)
    {
        uint8_t op = **pc_ptr;
        if (op == T_MUL)
        {
            (*pc_ptr)++;
            result *= eval_power_auto(pc_ptr);
        }
        else if (op == T_DIV)
        {
            (*pc_ptr)++;
            double divisor = eval_power_auto(pc_ptr);
            if (divisor == 0.0)
            {
                error_set(ERR_DIVISION_BY_ZERO, g_vm.current_line);
                return 0.0;
            }
            result /= divisor;
        }
        else
        {
            break; /* Stop at any other token */
        }
    }

    return result;
}

/* Evaluate power: factor (^ factor)* (right associative) */
static double eval_power_auto(uint8_t **pc_ptr)
{
    double result = eval_factor_auto(pc_ptr);

    if (**pc_ptr == T_POW)
    {
        (*pc_ptr)++;
        double exponent = eval_power_auto(pc_ptr); /* Right associative */
        result = pow(result, exponent);

        /* Check for math domain/overflow errors */
        if (!isfinite(result))
        {
            error_set(ERR_MATH_OVERFLOW, g_vm.current_line);
            return 0.0;
        }
    }

    return result;
}

/* Evaluate factor: number | variable | variable(expr) | (expr) | -factor */
static double eval_factor_auto(uint8_t **pc_ptr)
{
    uint8_t token = **pc_ptr;

    switch (token)
    {
    case T_NUM:
    {
        (*pc_ptr)++;
        double value = *(double *)*pc_ptr;
        *pc_ptr += sizeof(double);
        return value;
    }

    case T_VAR:
    {
        (*pc_ptr)++;
        uint8_t var_idx = **pc_ptr;
        (*pc_ptr)++;
        if (var_idx < 1 || var_idx > 26)
        { /* A-Z variables only */
            error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
            return 0.0;
        }
        VarCell *cell = &g_program.vars[var_idx - 1];
        if (cell->type != VAR_NUM)
        {
            error_set(ERR_TYPE_MISMATCH, g_vm.current_line);
            return 0.0;
        }
        return cell->value.num;
    }

    case T_VIDX:
    {
        (*pc_ptr)++; /* Skip T_VIDX */

        /* Evaluate index expression directly */
        double index_val = eval_expression_auto(pc_ptr);

        /* Skip T_ENDX terminator */
        if (**pc_ptr == T_ENDX)
        {
            (*pc_ptr)++;
        }

        /* Convert to integer index (truncate toward zero) */
        int index = (int)index_val;
        if (index < 1 || index > VARS_MAX)
        {
            error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
            return 0.0;
        }

        VarCell *cell = &g_program.vars[index - 1];
        if (cell->type != VAR_NUM)
        {
            error_set(ERR_TYPE_MISMATCH, g_vm.current_line);
            return 0.0;
        }
        return cell->value.num;
    }

    case T_LP:
    {
        (*pc_ptr)++;
        double result = eval_expression_auto(pc_ptr);
        if (**pc_ptr == T_RP)
        {
            (*pc_ptr)++;
        }
        else
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
        }
        return result;
    }

    case T_MINUS:
    {
        (*pc_ptr)++;
        return -eval_factor_auto(pc_ptr);
    }

    /* Math functions */
    case T_SIN:
    {
        (*pc_ptr)++;
        if (**pc_ptr != T_LP)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return 0.0;
        }
        (*pc_ptr)++;
        double arg = eval_expression_auto(pc_ptr);
        if (**pc_ptr == T_RP)
            (*pc_ptr)++;
        double radians = convert_angle_to_radians(arg);
        return sin(radians);
    }

    case T_COS:
    {
        (*pc_ptr)++;
        if (**pc_ptr != T_LP)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return 0.0;
        }
        (*pc_ptr)++;
        double arg = eval_expression_auto(pc_ptr);
        if (**pc_ptr == T_RP)
            (*pc_ptr)++;
        double radians = convert_angle_to_radians(arg);
        return cos(radians);
    }

    case T_TAN:
    {
        (*pc_ptr)++;
        if (**pc_ptr != T_LP)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return 0.0;
        }
        (*pc_ptr)++;
        double arg = eval_expression_auto(pc_ptr);
        if (**pc_ptr == T_RP)
            (*pc_ptr)++;
        double radians = convert_angle_to_radians(arg);
        return tan(radians);
    }

    case T_ASN:
    {
        (*pc_ptr)++;
        if (**pc_ptr != T_LP)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return 0.0;
        }
        (*pc_ptr)++;
        double arg = eval_expression_auto(pc_ptr);
        if (**pc_ptr == T_RP)
            (*pc_ptr)++;
        if (arg < -1.0 || arg > 1.0)
        {
            error_set(ERR_MATH_DOMAIN, g_vm.current_line);
            return 0.0;
        }
        double radians = asin(arg);
        return convert_angle_from_radians(radians);
    }

    case T_ACS:
    {
        (*pc_ptr)++;
        if (**pc_ptr != T_LP)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return 0.0;
        }
        (*pc_ptr)++;
        double arg = eval_expression_auto(pc_ptr);
        if (**pc_ptr == T_RP)
            (*pc_ptr)++;
        if (arg < -1.0 || arg > 1.0)
        {
            error_set(ERR_MATH_DOMAIN, g_vm.current_line);
            return 0.0;
        }
        double radians = acos(arg);
        return convert_angle_from_radians(radians);
    }

    case T_ATN:
    {
        (*pc_ptr)++;
        if (**pc_ptr != T_LP)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return 0.0;
        }
        (*pc_ptr)++;
        double arg = eval_expression_auto(pc_ptr);
        if (**pc_ptr == T_RP)
            (*pc_ptr)++;
        double radians = atan(arg);
        return convert_angle_from_radians(radians);
    }

    case T_LOG:
    {
        (*pc_ptr)++;
        if (**pc_ptr != T_LP)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return 0.0;
        }
        (*pc_ptr)++;
        double arg = eval_expression_auto(pc_ptr);
        if (**pc_ptr == T_RP)
            (*pc_ptr)++;
        if (arg <= 0.0)
        {
            error_set(ERR_MATH_DOMAIN, g_vm.current_line);
            return 0.0;
        }
        return log10(arg);
    }

    case T_LN:
    {
        (*pc_ptr)++;
        if (**pc_ptr != T_LP)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return 0.0;
        }
        (*pc_ptr)++;
        double arg = eval_expression_auto(pc_ptr);
        if (**pc_ptr == T_RP)
            (*pc_ptr)++;
        if (arg <= 0.0)
        {
            error_set(ERR_MATH_DOMAIN, g_vm.current_line);
            return 0.0;
        }
        return log(arg);
    }

    case T_EXP:
    {
        (*pc_ptr)++;
        if (**pc_ptr != T_LP)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return 0.0;
        }
        (*pc_ptr)++;
        double arg = eval_expression_auto(pc_ptr);
        if (**pc_ptr == T_RP)
            (*pc_ptr)++;
        double result = exp(arg);
        if (!isfinite(result))
        {
            error_set(ERR_MATH_OVERFLOW, g_vm.current_line);
            return 0.0;
        }
        return result;
    }

    case T_SQR:
    {
        (*pc_ptr)++;
        if (**pc_ptr != T_LP)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return 0.0;
        }
        (*pc_ptr)++;
        double arg = eval_expression_auto(pc_ptr);
        if (**pc_ptr == T_RP)
            (*pc_ptr)++;
        if (arg < 0.0)
        {
            error_set(ERR_MATH_DOMAIN, g_vm.current_line);
            return 0.0;
        }
        return sqrt(arg);
    }

    case T_ABS:
    {
        (*pc_ptr)++;
        if (**pc_ptr != T_LP)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return 0.0;
        }
        (*pc_ptr)++;
        double arg = eval_expression_auto(pc_ptr);
        if (**pc_ptr == T_RP)
            (*pc_ptr)++;
        return fabs(arg);
    }

    case T_INT:
    {
        (*pc_ptr)++;
        if (**pc_ptr != T_LP)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return 0.0;
        }
        (*pc_ptr)++;
        double arg = eval_expression_auto(pc_ptr);
        if (**pc_ptr == T_RP)
            (*pc_ptr)++;
        return floor(arg);
    }

    case T_SGN:
    {
        (*pc_ptr)++;
        if (**pc_ptr != T_LP)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return 0.0;
        }
        (*pc_ptr)++;
        double arg = eval_expression_auto(pc_ptr);
        if (**pc_ptr == T_RP)
            (*pc_ptr)++;
        if (arg < 0.0)
            return -1.0;
        else if (arg > 0.0)
            return 1.0;
        else
            return 0.0;
    }

    case T_DMS:
    {
        (*pc_ptr)++;
        if (**pc_ptr != T_LP)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return 0.0;
        }
        (*pc_ptr)++;
        double arg = eval_expression_auto(pc_ptr);
        if (**pc_ptr == T_RP)
            (*pc_ptr)++;

        /* Convert decimal degrees to DD.MMSS format */
        double degrees = floor(fabs(arg));
        double decimal_part = fabs(arg) - degrees;
        double total_minutes = decimal_part * 60.0;
        double minutes = floor(total_minutes);
        double decimal_seconds = (total_minutes - minutes) * 60.0;

        /* Format as DD.MMSS with fractional seconds */
        double result = degrees + (minutes / 100.0) + (decimal_seconds / 10000.0);

        /* Preserve sign */
        return (arg < 0.0) ? -result : result;
    }

    case T_DEG:
    {
        (*pc_ptr)++;
        if (**pc_ptr != T_LP)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return 0.0;
        }
        (*pc_ptr)++;
        double arg = eval_expression_auto(pc_ptr);
        if (**pc_ptr == T_RP)
            (*pc_ptr)++;

        /* Convert DD.MMSS format to decimal degrees */
        double abs_arg = fabs(arg);
        double degrees = floor(abs_arg);
        double fractional = abs_arg - degrees;

        /* Extract minutes (digits 1-2 after decimal) */
        double minutes_part = fractional * 100.0;
        double minutes = floor(minutes_part);

        /* Extract seconds (digits 3-4 and beyond after decimal) */
        double seconds_part = (minutes_part - minutes) * 100.0;

        /* Convert to decimal degrees */
        double result = degrees + (minutes / 60.0) + (seconds_part / 3600.0);

        /* Preserve sign */
        return (arg < 0.0) ? -result : result;
    }

    default:
        error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
        return 0.0;
    }
}

/* Simple string evaluation for IF conditions - returns string in buffer, max 7 chars + null */
static bool eval_string_expression(uint8_t **pc_ptr, char *result_buffer)
{
    uint8_t token = **pc_ptr;

    switch (token)
    {
    case T_STR: /* String literal */
    {
        (*pc_ptr)++; /* Skip T_STR */
        uint8_t str_len = **pc_ptr;
        (*pc_ptr)++;

        /* Copy up to 7 characters */
        int copy_len = (str_len > 7) ? 7 : str_len;
        memcpy(result_buffer, *pc_ptr, copy_len);
        result_buffer[copy_len] = '\0';

        *pc_ptr += str_len; /* Skip all string data */
        return true;
    }

    case T_SVAR: /* String variable A$-Z$ */
    {
        (*pc_ptr)++; /* Skip T_SVAR */
        uint8_t var_idx = **pc_ptr;
        (*pc_ptr)++;

        if (var_idx < 1 || var_idx > 26)
        {
            error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
            return false;
        }

        VarCell *cell = &g_program.vars[var_idx - 1];
        if (cell->type != VAR_STR)
        {
            /* Uninitialized string variable = empty string */
            result_buffer[0] = '\0';
        }
        else
        {
            strncpy(result_buffer, cell->value.str, 7);
            result_buffer[7] = '\0';
        }
        return true;
    }

    case T_SVIDX: /* String variable with index A$(expr) */
    {
        (*pc_ptr)++; /* Skip T_SVIDX */

        /* Evaluate index expression */
        double index_val = vm_eval_expression_auto(pc_ptr);
        if (error_get_code() != ERR_NONE)
            return false;

        /* Check for T_ENDX terminator */
        if (**pc_ptr != T_ENDX)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return false;
        }
        (*pc_ptr)++; /* Skip T_ENDX */

        int index = (int)index_val;
        if (index < 1 || index > VARS_MAX)
        {
            error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
            return false;
        }

        VarCell *cell = &g_program.vars[index - 1];
        if (cell->type != VAR_STR)
        {
            result_buffer[0] = '\0'; /* Empty string */
        }
        else
        {
            strncpy(result_buffer, cell->value.str, 7);
            result_buffer[7] = '\0';
        }
        return true;
    }

    default:
        error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
        return false;
    }
}

/* Evaluate condition for IF statement */
bool vm_eval_condition(uint8_t **pc_ptr, uint8_t *end)
{
    /* Check if left side is a string expression */
    uint8_t left_token = **pc_ptr;
    bool is_string_comparison = (left_token == T_STR || left_token == T_SVAR || left_token == T_SVIDX);

    if (is_string_comparison)
    {
        /* String comparison */
        char left_str[8];  /* 7 chars + null */
        char right_str[8]; /* 7 chars + null */

        /* Evaluate left string */
        if (!eval_string_expression(pc_ptr, left_str))
            return false;

        /* Get comparison operator - only = and <> supported for strings */
        if (*pc_ptr >= end)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return false;
        }

        uint8_t op = **pc_ptr;
        if (op != T_EQ && op != T_EQ_ASSIGN && op != T_NE)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return false;
        }
        (*pc_ptr)++;

        /* Evaluate right string */
        if (!eval_string_expression(pc_ptr, right_str))
            return false;

        /* Perform string comparison */
        int cmp_result = strcmp(left_str, right_str);
        switch (op)
        {
        case T_EQ:
        case T_EQ_ASSIGN:
            return cmp_result == 0;
        case T_NE:
            return cmp_result != 0;
        default:
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return false;
        }
    }
    else
    {
        /* Numeric comparison */
        double left_val = vm_eval_expression_auto(pc_ptr);

        if (error_get_code() != ERR_NONE)
            return false;

        /* Get comparison operator */
        if (*pc_ptr >= end)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return false;
        }

        uint8_t op = **pc_ptr;
        (*pc_ptr)++;

        /* Parse right side of comparison */
        double right_val = vm_eval_expression_auto(pc_ptr);

        if (error_get_code() != ERR_NONE)
            return false;

        /* Perform comparison */
        switch (op)
        {
        case T_EQ:
        case T_EQ_ASSIGN: /* Allow = as comparison in IF statements */
            return left_val == right_val;
        case T_NE:
            return left_val != right_val;
        case T_LT:
            return left_val < right_val;
        case T_LE:
            return left_val <= right_val;
        case T_GT:
            return left_val > right_val;
        case T_GE:
            return left_val >= right_val;
        default:
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return false;
        }
    }
}

/* Statement execution function pointer type */
typedef void (*execute_fn_t)(void);

/* Forward declarations for execution functions */
static void execute_label(void);
static void execute_var_assign(void);
static void execute_svar_assign(void);
static void execute_vidx_assign(void);
static void execute_svidx_assign(void);
static void execute_let(void);
static void execute_print(void);
static void execute_goto(void);
static void execute_if(void);
static void execute_gosub(void);
static void execute_return(void);
static void execute_for(void);
static void execute_next(void);
static void execute_end(void);
static void execute_stop(void);
static void execute_input(void);
static void execute_aread(void);
static void execute_degree(void);
static void execute_radian(void);
static void execute_grad(void);
static void execute_clear(void);
static void execute_beep(void);
static void execute_pause(void);
static void execute_using(void);
static void execute_default(void);
static void execute_rem(void);
static void execute_colon(void);
static void execute_eol(void);

/* Function pointer table for statement execution - indexed by token value */
static execute_fn_t execute_table[256] = {
    [T_EOL] = execute_eol,
    [T_STR] = execute_label,          /* Labels are stored as T_STR */
    [T_VAR] = execute_var_assign,     /* Direct assignment: A = expr */
    [T_SVAR] = execute_svar_assign,   /* String assignment: A$ = string_expr */
    [T_VIDX] = execute_vidx_assign,   /* Indexed assignment: A(expr) = expr */
    [T_SVIDX] = execute_svidx_assign, /* Indexed string assignment: A$(expr) = string_expr */

    [T_COLON] = execute_colon,

    [T_LET] = execute_let,
    [T_PRINT] = execute_print,
    [T_INPUT] = execute_input,
    [T_IF] = execute_if,
    [T_GOTO] = execute_goto,
    [T_GOSUB] = execute_gosub,
    [T_RETURN] = execute_return,
    [T_FOR] = execute_for,
    [T_NEXT] = execute_next,
    [T_END] = execute_end,
    [T_STOP] = execute_stop,
    [T_REM] = execute_rem,

    /* Mode & device commands */
    [T_DEGREE] = execute_degree,
    [T_RADIAN] = execute_radian,
    [T_GRAD] = execute_grad,
    [T_CLEAR] = execute_clear,
    [T_BEEP] = execute_beep,
    [T_PAUSE] = execute_pause,
    [T_AREAD] = execute_aread,
    [T_USING] = execute_using,

    /* Default handler for all other tokens */
    /* Note: All uninitialized entries are NULL, which we'll handle as execute_default */
};

/* Execute a single statement */
void vm_execute_statement(void)
{
    if (!g_vm.pc || !g_vm.running)
        return;

    uint8_t token = *g_vm.pc;
    g_vm.pc++;

    /* Look up function in table */
    execute_fn_t execute_fn = execute_table[token];

    if (execute_fn)
    {
        uint8_t *pc_after_advance = g_vm.pc; /* PC after the advance */
        execute_fn();

        /* If the function changed PC (GOTO, GOSUB, RETURN, NEXT jump, etc.)
         * or stopped running (END, STOP), don't do normal PC advancement */
        if (!g_vm.running || g_vm.pc != pc_after_advance)
        {
            return;
        }
    }
    else
    {
        /* Handle unknown tokens */
        execute_default();
    }
}

/* Statement execution helper function implementations */

static void execute_label(void)
{
    /* Label - skip it */
    uint8_t str_len = *g_vm.pc++;
    g_vm.pc += str_len; /* Skip the string data */
    /* Label is now skipped, continue with next token */
}

static void execute_var_assign(void)
{
    /* Direct assignment: A = expr */
    uint8_t var_idx = *g_vm.pc++;

    /* Expect = */
    if (*g_vm.pc != T_EQ_ASSIGN)
    {
        error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
        return;
    }
    g_vm.pc++;

    /* Evaluate right-hand side */
    double value = vm_eval_expression_auto(&g_vm.pc);

    if (error_get_code() != ERR_NONE)
        return;

    /* Store in variable */
    if (var_idx < 1 || var_idx > 26)
    { /* A-Z variables only */
        error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
        return;
    }

    VarCell *cell = &g_program.vars[var_idx - 1];
    cell->type = VAR_NUM;
    cell->value.num = value;
}

static void execute_svar_assign(void)
{
    /* String assignment: A$ = string_expr */
    uint8_t var_idx = *g_vm.pc++;

    /* Expect = */
    if (*g_vm.pc != T_EQ_ASSIGN)
    {
        error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
        return;
    }
    g_vm.pc++;

    /* For now, only handle string literals */
    if (*g_vm.pc != T_STR)
    {
        error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
        return;
    }
    g_vm.pc++; /* Skip T_STR */

    uint8_t orig_str_len = *g_vm.pc++;
    uint8_t copy_len = (orig_str_len > STR_MAX) ? STR_MAX : orig_str_len;

    /* Store in string variable */
    if (var_idx < 1 || var_idx > 26)
    {
        error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
        return;
    }

    VarCell *cell = &g_program.vars[var_idx - 1];
    cell->type = VAR_STR;

    /* Copy string data, convert to uppercase (up to STR_MAX chars) */
    int i;
    for (i = 0; i < copy_len; i++)
    {
        char c = *g_vm.pc++;
        if (c >= 'a' && c <= 'z')
        {
            c = c - 'a' + 'A'; /* Convert to uppercase */
        }
        cell->value.str[i] = c;
    }
    cell->value.str[i] = '\0'; /* Null terminate */

    /* Skip remaining bytes if string was longer than STR_MAX */
    for (int j = copy_len; j < orig_str_len; j++)
    {
        g_vm.pc++;
    }
}

static void execute_vidx_assign(void)
{
    /* Indexed assignment: A(expr) = expr */
    /* T_VIDX always refers to variable A with indexing */
    /* No placeholder byte - directly parse the index expression */

    /* Evaluate index */
    double index_val = vm_eval_expression_auto(&g_vm.pc);

    /* Skip T_ENDX */
    if (*g_vm.pc == T_ENDX)
        g_vm.pc++;

    /* Expect = */
    if (*g_vm.pc != T_EQ_ASSIGN)
    {
        error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
        return;
    }
    g_vm.pc++;

    /* Evaluate right-hand side */
    double value = vm_eval_expression_auto(&g_vm.pc);

    if (error_get_code() != ERR_NONE)
        return;

    /* Store in indexed variable */
    int index = (int)index_val;
    if (index < 1 || index > VARS_MAX)
    {
        error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
        return;
    }

    VarCell *cell = &g_program.vars[index - 1];
    cell->type = VAR_NUM;
    cell->value.num = value;
}

static void execute_svidx_assign(void)
{
    /* Indexed string assignment: A$(expr) = string_expr */
    /* T_SVIDX works like T_VIDX - parse expression between T_SVIDX and T_ENDX */
    double index_val = vm_eval_expression_auto(&g_vm.pc);

    /* Skip T_ENDX terminator */
    if (*g_vm.pc == T_ENDX)
        g_vm.pc++;

    /* Expect = */
    if (*g_vm.pc != T_EQ_ASSIGN)
    {
        error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
        return;
    }
    g_vm.pc++;

    /* For now, only handle string literals */
    if (*g_vm.pc != T_STR)
    {
        error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
        return;
    }
    g_vm.pc++; /* Skip T_STR */

    uint8_t orig_str_len = *g_vm.pc++;
    uint8_t copy_len = (orig_str_len > STR_MAX) ? STR_MAX : orig_str_len;

    /* Convert index to integer */
    int index = (int)index_val;
    if (index < 1 || index > VARS_MAX)
    {
        error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
        return;
    }

    VarCell *cell = &g_program.vars[index - 1];
    cell->type = VAR_STR;

    /* Copy string data, convert to uppercase (up to STR_MAX chars) */
    int i;
    for (i = 0; i < copy_len; i++)
    {
        char c = *g_vm.pc++;
        if (c >= 'a' && c <= 'z')
        {
            c = c - 'a' + 'A'; /* Convert to uppercase */
        }
        cell->value.str[i] = c;
    }
    cell->value.str[i] = '\0'; /* Null terminate */

    /* Skip remaining bytes if string was longer than STR_MAX */
    for (int j = copy_len; j < orig_str_len; j++)
    {
        g_vm.pc++;
    }
}

static void execute_let(void)
{
    /* Handle assignment with explicit LET: LET VAR = expr or LET VAR(expr) = expr */
    uint8_t next_token = *g_vm.pc;

    if (next_token == T_VAR)
    {
        g_vm.pc++;
        uint8_t var_idx = *g_vm.pc++;

        /* Expect = */
        if (*g_vm.pc != T_EQ_ASSIGN)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return;
        }
        g_vm.pc++;

        /* Evaluate right-hand side */
        double value = vm_eval_expression_auto(&g_vm.pc);

        if (error_get_code() != ERR_NONE)
            return;

        /* Store in variable */
        if (var_idx < 1 || var_idx > 26)
        { /* A-Z variables only */
            error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
            return;
        }

        VarCell *cell = &g_program.vars[var_idx - 1];
        cell->type = VAR_NUM;
        cell->value.num = value;
    }
    else if (next_token == T_VIDX)
    {
        g_vm.pc++; /* Skip T_VIDX */
        g_vm.pc++; /* Skip placeholder byte */

        /* Evaluate index */
        double index_val = vm_eval_expression_auto(&g_vm.pc);

        /* Skip T_ENDX */
        if (*g_vm.pc == T_ENDX)
            g_vm.pc++;

        /* Expect = */
        if (*g_vm.pc != T_EQ_ASSIGN)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return;
        }
        g_vm.pc++;

        /* Evaluate right-hand side */
        double value = vm_eval_expression_auto(&g_vm.pc);

        if (error_get_code() != ERR_NONE)
            return;

        /* Store in indexed variable */
        int index = (int)index_val;
        if (index < 1 || index > VARS_MAX)
        {
            error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
            return;
        }

        VarCell *cell = &g_program.vars[index - 1];
        cell->type = VAR_NUM;
        cell->value.num = value;
    }
}

/* Shared print logic used by both PRINT and PAUSE */
static void print_expressions(void)
{
    while (*g_vm.pc != T_COLON && *g_vm.pc != T_EOL)
    {
        if (*g_vm.pc == T_COMMA || *g_vm.pc == T_SEMI)
        {
            printf(" ");
            g_vm.pc++;
        }
        else if (*g_vm.pc == T_STR)
        {
            /* Handle string literals */
            g_vm.pc++; /* Skip T_STR */
            uint8_t str_len = *g_vm.pc++;
            for (int i = 0; i < str_len; i++)
            {
                putchar(*g_vm.pc++);
            }
        }
        else if (*g_vm.pc == T_SVAR)
        {
            /* Handle string variables */
            g_vm.pc++; /* Skip T_SVAR */
            uint8_t var_idx = *g_vm.pc++;
            if (var_idx < 1 || var_idx > 26)
            {
                error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
                return;
            }
            VarCell *cell = &g_program.vars[var_idx - 1];
            if (cell->type == VAR_STR)
            {
                printf("%s", cell->value.str);
            }
            else
            {
                /* Print empty string for uninitialized string variables */
                /* (PC-1211 behavior for uninitialized string vars) */
            }
        }
        else if (*g_vm.pc == T_SVIDX)
        {
            /* Handle indexed string variables */
            g_vm.pc++; /* Skip T_SVIDX */

            /* Evaluate index expression */
            double index_val = vm_eval_expression_auto(&g_vm.pc);
            if (error_get_code() != ERR_NONE)
                return;

            /* Skip T_ENDX terminator */
            if (*g_vm.pc == T_ENDX)
            {
                g_vm.pc++;
            }

            /* Convert to integer index (truncate toward zero) */
            int index = (int)index_val;
            if (index < 1 || index > VARS_MAX)
            {
                error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
                return;
            }

            VarCell *cell = &g_program.vars[index - 1];
            if (cell->type == VAR_STR)
            {
                printf("%s", cell->value.str);
            }
            else
            {
                /* Print empty string for uninitialized string variables */
            }
        }
        else
        {
            /* Handle numeric expressions */
            double value = vm_eval_expression_auto(&g_vm.pc);
            if (error_get_code() != ERR_NONE)
                return;
            printf("%g", value);
        }
    }
    printf("\n");
}

static void execute_print(void)
{
    /* Simple PRINT implementation */
    print_expressions();

    /* PRINT clears AREAD after displaying */
    g_aread_value = 0.0;
    g_aread_string[0] = '\0';
    g_aread_is_string = false;
}

static void execute_goto(void)
{
    /* Check if next token is a string label (literal or variable) */
    if (*g_vm.pc == T_STR)
    {
        /* String literal label */
        g_vm.pc++; /* Skip T_STR */
        uint8_t str_len = *g_vm.pc++;
        if (str_len > STR_MAX)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return;
        }
        char label[STR_MAX + 1];
        memcpy(label, g_vm.pc, str_len);
        label[str_len] = '\0';
        g_vm.pc += str_len;

        vm_goto_label(label);
    }
    else if (*g_vm.pc == T_SVAR)
    {
        /* String variable label */
        g_vm.pc++; /* Skip T_SVAR */
        uint8_t var_idx = *g_vm.pc++;

        /* Get string variable value */
        VarCell *cell = var_get(var_idx);
        if (!cell)
            return; /* Error already set by var_get */

        if (cell->type != VAR_STR)
        {
            error_set(ERR_TYPE_MISMATCH, g_vm.current_line);
            return;
        }

        vm_goto_label(cell->value.str);
    }
    else
    {
        /* Parse expression for line number */
        double line_num = eval_expression_auto(&g_vm.pc);
        if (error_get_code() != ERR_NONE)
            return;

        vm_goto_line((uint16_t)line_num);
    }
    /* Don't advance PC normally - handled by return */
}

static void execute_end(void)
{
    g_vm.running = false;
}

static void execute_stop(void)
{
    g_vm.running = false;
}

static void execute_gosub(void)
{
    /* Capture current position for return */
    VMPosition return_pos = vm_capture_position();

    /* Check if next token is a string label (literal or variable) */
    if (*g_vm.pc == T_STR)
    {
        /* String literal label */
        g_vm.pc++; /* Skip T_STR */
        uint8_t str_len = *g_vm.pc++;
        if (str_len > STR_MAX)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return;
        }
        char label[STR_MAX + 1];
        memcpy(label, g_vm.pc, str_len);
        label[str_len] = '\0';
        g_vm.pc += str_len;

        /* Update return position to point after the label */
        return_pos.pc = g_vm.pc;

        /* Push return address onto call stack */
        vm_push_call(return_pos);
        if (error_get_code() != ERR_NONE)
            return;

        vm_goto_label(label);
    }
    else if (*g_vm.pc == T_SVAR)
    {
        /* String variable label */
        g_vm.pc++; /* Skip T_SVAR */
        uint8_t var_idx = *g_vm.pc++;

        /* Update return position to point after the variable */
        return_pos.pc = g_vm.pc;

        /* Get string variable value */
        VarCell *cell = var_get(var_idx);
        if (!cell)
            return; /* Error already set by var_get */

        if (cell->type != VAR_STR)
        {
            error_set(ERR_TYPE_MISMATCH, g_vm.current_line);
            return;
        }

        /* Push return address onto call stack */
        vm_push_call(return_pos);
        if (error_get_code() != ERR_NONE)
            return;

        vm_goto_label(cell->value.str);
    }
    else
    {
        /* Parse expression for line number */
        double line_num = eval_expression_auto(&g_vm.pc);
        if (error_get_code() != ERR_NONE)
            return;

        /* Update return position to point after the expression */
        return_pos.pc = g_vm.pc;

        /* Push return address onto call stack */
        vm_push_call(return_pos);
        if (error_get_code() != ERR_NONE)
            return;

        vm_goto_line((uint16_t)line_num);
    }
    /* Don't advance PC normally - handled by jump */
}

static void execute_return(void)
{
    /* Pop return address from call stack */
    VMPosition return_pos;
    if (!vm_pop_call(&return_pos))
    {
        return; /* Error already set */
    }

    vm_restore_position(return_pos);
    /* Don't advance PC normally - this is handled by position restore */
}

static void execute_for(void)
{
    /* FOR var = start TO limit [STEP step] */
    if (*g_vm.pc != T_VAR)
    {
        error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
        return;
    }
    g_vm.pc++;
    uint8_t var_idx = *g_vm.pc++;

    /* Expect = */
    if (*g_vm.pc != T_EQ_ASSIGN)
    {
        error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
        return;
    }
    g_vm.pc++;

    /* Evaluate start expression */
    double start_val = vm_eval_expression_auto(&g_vm.pc);
    if (error_get_code() != ERR_NONE)
        return;

    /* Skip TO */
    if (*g_vm.pc != T_TO)
    {
        error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
        return;
    }
    g_vm.pc++;

    /* Evaluate limit expression */
    double limit_val = vm_eval_expression_auto(&g_vm.pc);
    if (error_get_code() != ERR_NONE)
        return;

    /* Parse optional STEP */
    double step_val = 1.0; /* Default step */
    if (*g_vm.pc == T_STEP)
    {
        g_vm.pc++; /* Skip STEP */
        step_val = vm_eval_expression_auto(&g_vm.pc);
        if (error_get_code() != ERR_NONE)
            return;
    }

    /* Check for STEP = 0 error */
    if (step_val == 0.0)
    {
        error_set(ERR_FOR_STEP_ZERO, g_vm.current_line);
        return;
    }

    /* Store start value in loop variable */
    if (var_idx < 1 || var_idx > 26)
    {
        error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
        return;
    }
    VarCell *cell = &g_program.vars[var_idx - 1];
    cell->type = VAR_NUM;
    cell->value.num = start_val;

    /* Determine pc_after_for: either next statement on same line or next line */
    VMPosition pc_after_for;
    pc_after_for.line = g_vm.current_line;

    /* Check if there are more statements on the current line */
    if (*g_vm.pc == T_COLON)
    {
        /* There's a colon, so pc_after_for points to statement after colon */
        pc_after_for.pc = g_vm.pc + 1; /* Skip the colon */
    }
    else
    {
        /* No more statements on current line, find start of next line */
        uint8_t *next_line_pc = g_vm.pc;
        while (*next_line_pc != T_EOL && next_line_pc < g_program.prog + g_program.prog_len)
        {
            next_line_pc = token_skip(next_line_pc);
            if (!next_line_pc)
                break;
        }
        if (*next_line_pc == T_EOL)
        {
            next_line_pc++; /* Skip T_EOL */
            /* Skip line header (len + line_num) to get to tokens */
            if (next_line_pc < g_program.prog + g_program.prog_len)
            {
                uint16_t next_line_num = *(uint16_t *)(next_line_pc + 2);
                pc_after_for.line = next_line_num;
                pc_after_for.pc = next_line_pc + 4; /* Skip len + line_num */
            }
            else
            {
                /* End of program */
                pc_after_for.pc = NULL;
                pc_after_for.line = 0;
            }
        }
        else
        {
            /* Error - malformed program */
            pc_after_for.pc = NULL;
            pc_after_for.line = 0;
        }
    }

    vm_push_for(pc_after_for, var_idx, limit_val, step_val);
    if (error_get_code() != ERR_NONE)
        return;
}

static void execute_next(void)
{
    /* NEXT [var] */
    uint8_t var_idx = 0;
    bool has_var = false;

    /* Check if followed by variable */
    if (*g_vm.pc == T_VAR)
    {
        g_vm.pc++;
        var_idx = *g_vm.pc++;
        has_var = true;
    }

    VMPosition pc_after_for_pos;
    uint8_t frame_var_idx;
    double limit, step;

    if (has_var)
    {
        /* Named NEXT - find matching FOR frame */
        int frame_index;
        if (!vm_find_for_by_var(var_idx, &frame_index))
        {
            error_set(ERR_NEXT_WITHOUT_FOR, g_vm.current_line);
            return;
        }

        /* Get frame data */
        ForFrame *frame = &g_vm.for_stack.frames[frame_index];
        pc_after_for_pos = frame->pc_after_for;
        frame_var_idx = frame->var_idx;
        limit = frame->limit;
        step = frame->step;

        /* Remove this frame and all frames above it */
        g_vm.for_stack.top = frame_index;
    }
    else
    {
        /* Unnamed NEXT - use top FOR frame */
        if (!vm_pop_for(&pc_after_for_pos, &frame_var_idx, &limit, &step))
        {
            return; /* Error already set */
        }
    }

    /* Update loop variable */
    if (frame_var_idx < 1 || frame_var_idx > 26)
    {
        error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
        return;
    }
    VarCell *cell = &g_program.vars[frame_var_idx - 1];
    if (cell->type != VAR_NUM)
    {
        error_set(ERR_TYPE_MISMATCH, g_vm.current_line);
        return;
    }
    cell->value.num += step;

    /* Check loop condition */
    bool continue_loop;
    if (step > 0)
    {
        continue_loop = (cell->value.num <= limit);
    }
    else
    {
        continue_loop = (cell->value.num >= limit);
    }

    if (continue_loop)
    {
        /* Push frame back and jump to after FOR */
        vm_push_for(pc_after_for_pos, frame_var_idx, limit, step);
        if (error_get_code() != ERR_NONE)
            return;

        /* Use vm_restore_position to jump cleanly */
        vm_restore_position(pc_after_for_pos);

        /* Don't advance PC normally - this is handled by position restore */
    }
    /* Loop finished - continue to next statement */
}

static void execute_input(void)
{
    /* INPUT variable - read value from user */
    if (*g_vm.pc == T_VAR)
    {
        /* Numeric variable */
        g_vm.pc++; /* Skip T_VAR */
        uint8_t var_idx = *g_vm.pc++;

        if (var_idx < 1 || var_idx > 26)
        {
            error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
            return;
        }

        printf("? ");
        fflush(stdout);

        char input[100];
        if (fgets(input, sizeof(input), stdin))
        {
            double value = atof(input);
            VarCell *cell = &g_program.vars[var_idx - 1];
            cell->type = VAR_NUM;
            cell->value.num = value;
        }
    }
    else if (*g_vm.pc == T_SVAR)
    {
        /* String variable */
        g_vm.pc++; /* Skip T_SVAR */
        uint8_t var_idx = *g_vm.pc++;

        if (var_idx < 1 || var_idx > 26)
        {
            error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
            return;
        }

        printf("? ");
        fflush(stdout);

        char input[100];
        if (fgets(input, sizeof(input), stdin))
        {
            /* Remove newline if present */
            int len = strlen(input);
            if (len > 0 && input[len - 1] == '\n')
            {
                input[len - 1] = '\0';
                len--;
            }

            /* Limit to 7 characters and convert to uppercase */
            if (len > STR_MAX)
            {
                len = STR_MAX;
            }

            VarCell *cell = &g_program.vars[var_idx - 1];
            cell->type = VAR_STR;

            int i;
            for (i = 0; i < len; i++)
            {
                char c = input[i];
                if (c >= 'a' && c <= 'z')
                {
                    c = c - 'a' + 'A'; /* Convert to uppercase */
                }
                cell->value.str[i] = c;
            }
            cell->value.str[i] = '\0';
        }
    }
    else if (*g_vm.pc == T_VIDX)
    {
        /* Indexed numeric variable */
        g_vm.pc++; /* Skip T_VIDX */
        g_vm.pc++; /* Skip placeholder byte */

        /* Evaluate index expression */
        double index_val = vm_eval_expression_auto(&g_vm.pc);
        if (error_get_code() != ERR_NONE)
            return;

        /* Skip T_ENDX terminator */
        if (*g_vm.pc == T_ENDX)
        {
            g_vm.pc++;
        }

        /* Convert to integer index */
        int index = (int)index_val;
        if (index < 1 || index > VARS_MAX)
        {
            error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
            return;
        }

        printf("? ");
        fflush(stdout);

        char input[100];
        if (fgets(input, sizeof(input), stdin))
        {
            double value = atof(input);
            VarCell *cell = &g_program.vars[index - 1];
            cell->type = VAR_NUM;
            cell->value.num = value;
        }
    }
    else if (*g_vm.pc == T_SVIDX)
    {
        /* Indexed string variable */
        g_vm.pc++; /* Skip T_SVIDX */
        g_vm.pc++; /* Skip placeholder byte */

        /* Evaluate index expression */
        double index_val = vm_eval_expression_auto(&g_vm.pc);
        if (error_get_code() != ERR_NONE)
            return;

        /* Skip T_ENDX terminator */
        if (*g_vm.pc == T_ENDX)
        {
            g_vm.pc++;
        }

        /* Convert to integer index */
        int index = (int)index_val;
        if (index < 1 || index > VARS_MAX)
        {
            error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
            return;
        }

        printf("? ");
        fflush(stdout);

        char input[100];
        if (fgets(input, sizeof(input), stdin))
        {
            /* Remove newline if present */
            int len = strlen(input);
            if (len > 0 && input[len - 1] == '\n')
            {
                input[len - 1] = '\0';
                len--;
            }

            /* Limit to 7 characters and convert to uppercase */
            if (len > STR_MAX)
            {
                len = STR_MAX;
            }

            VarCell *cell = &g_program.vars[index - 1];
            cell->type = VAR_STR;

            int i;
            for (i = 0; i < len; i++)
            {
                char c = input[i];
                if (c >= 'a' && c <= 'z')
                {
                    c = c - 'a' + 'A'; /* Convert to uppercase */
                }
                cell->value.str[i] = c;
            }
            cell->value.str[i] = '\0';
        }
    }
    else
    {
        error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
        return;
    }
}

static void execute_aread(void)
{
    /* AREAD variable - read previous screen value into variable */
    if (*g_vm.pc == T_VAR)
    {
        /* Numeric variable */
        g_vm.pc++; /* Skip T_VAR */
        uint8_t var_idx = *g_vm.pc++;

        if (var_idx < 1 || var_idx > 26)
        {
            error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
            return;
        }

        VarCell *cell = &g_program.vars[var_idx - 1];
        cell->type = VAR_NUM;
        if (g_aread_is_string)
        {
            /* Convert string to number */
            cell->value.num = atof(g_aread_string);
        }
        else
        {
            cell->value.num = g_aread_value;
        }
        /* Clear AREAD after use */
        g_aread_value = 0.0;
        g_aread_string[0] = '\0';
        g_aread_is_string = false;
    }
    else if (*g_vm.pc == T_SVAR)
    {
        /* String variable */
        g_vm.pc++; /* Skip T_SVAR */
        uint8_t var_idx = *g_vm.pc++;

        if (var_idx < 1 || var_idx > 26)
        {
            error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
            return;
        }

        VarCell *cell = &g_program.vars[var_idx - 1];
        cell->type = VAR_STR;

        if (g_aread_is_string)
        {
            /* Use string directly */
            strncpy(cell->value.str, g_aread_string, sizeof(cell->value.str) - 1);
            cell->value.str[sizeof(cell->value.str) - 1] = '\0';
        }
        else
        {
            /* Convert numeric value to string */
            snprintf(cell->value.str, sizeof(cell->value.str), "%.6g", g_aread_value);
        }
        /* Clear AREAD after use */
        g_aread_value = 0.0;
        g_aread_string[0] = '\0';
        g_aread_is_string = false;
    }
    else if (*g_vm.pc == T_VIDX)
    {
        /* Indexed numeric variable */
        g_vm.pc++; /* Skip T_VIDX */
        g_vm.pc++; /* Skip placeholder byte */

        /* Evaluate index expression */
        double index_val = vm_eval_expression_auto(&g_vm.pc);
        if (error_get_code() != ERR_NONE)
            return;

        /* Skip T_ENDX terminator */
        if (*g_vm.pc == T_ENDX)
        {
            g_vm.pc++;
        }

        /* Convert to integer index */
        int index = (int)index_val;
        if (index < 1 || index > VARS_MAX)
        {
            error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
            return;
        }

        VarCell *cell = &g_program.vars[index - 1];
        cell->type = VAR_NUM;
        if (g_aread_is_string)
        {
            /* Convert string to number */
            cell->value.num = atof(g_aread_string);
        }
        else
        {
            cell->value.num = g_aread_value;
        }
        /* Clear AREAD after use */
        g_aread_value = 0.0;
        g_aread_string[0] = '\0';
        g_aread_is_string = false;
    }
    else if (*g_vm.pc == T_SVIDX)
    {
        /* Indexed string variable */
        g_vm.pc++; /* Skip T_SVIDX */
        g_vm.pc++; /* Skip placeholder byte */

        /* Evaluate index expression */
        double index_val = vm_eval_expression_auto(&g_vm.pc);
        if (error_get_code() != ERR_NONE)
            return;

        /* Skip T_ENDX terminator */
        if (*g_vm.pc == T_ENDX)
        {
            g_vm.pc++;
        }

        /* Convert to integer index */
        int index = (int)index_val;
        if (index < 1 || index > VARS_MAX)
        {
            error_set(ERR_INDEX_OUT_OF_RANGE, g_vm.current_line);
            return;
        }

        VarCell *cell = &g_program.vars[index - 1];
        cell->type = VAR_STR;

        if (g_aread_is_string)
        {
            /* Use string directly */
            strncpy(cell->value.str, g_aread_string, sizeof(cell->value.str) - 1);
            cell->value.str[sizeof(cell->value.str) - 1] = '\0';
        }
        else
        {
            /* Convert numeric value to string */
            snprintf(cell->value.str, sizeof(cell->value.str), "%.6g", g_aread_value);
        }
        /* Clear AREAD after use */
        g_aread_value = 0.0;
        g_aread_string[0] = '\0';
        g_aread_is_string = false;
    }
    else
    {
        error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
        return;
    }
}

static void execute_degree(void)
{
    g_vm.angle_mode = ANGLE_DEGREE;
}

static void execute_radian(void)
{
    g_vm.angle_mode = ANGLE_RADIAN;
}

static void execute_grad(void)
{
    g_vm.angle_mode = ANGLE_GRAD;
}

static void execute_clear(void)
{
    /* Clear all variables A-Z and indexed variables A(1) through A(VARS_MAX) */
    for (int i = 0; i <= VARS_MAX; i++)
    {
        g_program.vars[i].type = VAR_NUM;
        g_program.vars[i].value.num = 0.0;
    }
}

static void execute_beep(void)
{
    putchar('\a'); /* ASCII bell character */
    fflush(stdout);
}

static void execute_pause(void)
{
    /* PAUSE works exactly like PRINT, then waits 100ms */
    print_expressions();

    /* Wait 100ms */
    usleep(100000); /* 100,000 microseconds = 100ms */

    /* PAUSE clears AREAD after displaying */
    g_aread_value = 0.0;
    g_aread_string[0] = '\0';
    g_aread_is_string = false;
}

static void execute_using(void)
{
    /* No-op for now - just ignore */
}

static void execute_default(void)
{
    error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
}

static void execute_rem(void)
{
    /* Skip remaining tokens on line until EOL */
    while (*g_vm.pc != T_EOL && *g_vm.pc != 0)
    {
        g_vm.pc++;
    }
}

static void execute_colon(void)
{
    /* No-op - colon is just a statement separator */
}

static void execute_eol(void)
{
    /* End of line - advance to next line */
    vm_next_line();
}

static void execute_if(void)
{
    /* IF condition [THEN line_number | statement] */

    /* First, evaluate the condition - same for both forms */
    uint8_t *line_end = program_find_line_end_from_pos(g_vm.pc);

    bool condition = vm_eval_condition(&g_vm.pc, line_end);
    if (error_get_code() != ERR_NONE)
        return;

    /* Now check what follows the condition */
    if (*g_vm.pc == T_THEN)
    {
        /* IF condition THEN target */
        g_vm.pc++; /* Skip T_THEN */

        if (condition)
        {
            /* Evaluate the target after THEN (could be variable, number, or string label) */
            if (*g_vm.pc == T_STR || *g_vm.pc == T_SVAR || *g_vm.pc == T_SVIDX)
            {
                /* String label - evaluate and look up */
                char label_str[8]; /* 7 chars + null */
                if (!eval_string_expression(&g_vm.pc, label_str))
                    return;

                uint16_t target_line = program_find_label(label_str);
                if (target_line == 0)
                {
                    error_set(ERR_BAD_LINE_NUMBER, g_vm.current_line);
                    return;
                }

                vm_goto_line(target_line);
            }
            else
            {
                /* Numeric expression - evaluate and use as line number */
                double line_num = vm_eval_expression_auto(&g_vm.pc);
                if (error_get_code() != ERR_NONE)
                    return;

                vm_goto_line((int)line_num);
            }
        }
        else
        {
            /* Condition false - advance to end of line */
            while (*g_vm.pc != T_EOL && *g_vm.pc != 0)
            {
                g_vm.pc++;
            }
        }
    }
    else
    {
        /* IF condition statement (no THEN) */
        if (condition)
        {
            /* Execute the statement - PC is already positioned at the statement */
            /* Statement will be executed in next iteration of main loop */
        }
        else
        {
            /* Condition false - advance to end of line */
            while (*g_vm.pc != T_EOL && *g_vm.pc != 0)
            {
                g_vm.pc++;
            }
        }
    }
}

/* Run program */
void vm_run(void)
{
    /* Clear any previous errors */
    error_clear();

    /* Start at first line */
    vm_start_program();
    if (!g_vm.running)
    {
        printf("No program loaded\n");
        return;
    }

    /* Main execution loop */
    while (g_vm.running && error_get_code() == ERR_NONE)
    {
        vm_execute_statement();
    }

    /* Handle any errors */
    if (error_get_code() != ERR_NONE)
    {
        error_print();
    }
}

/* Single step */
void vm_step(void)
{
    if (g_vm.running && error_get_code() == ERR_NONE)
    {
        vm_execute_statement();
    }
}

/* Halt VM */
void vm_halt(void)
{
    g_vm.running = false;
}
