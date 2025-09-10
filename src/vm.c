#include "vm.h"
#include "program.h"
#include "errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

/* Global VM state */
VM g_vm;
AngleMode g_angle_mode = ANGLE_RADIAN;  /* Default to radians */
char g_aread_string[8] = "";            /* AREAD string value */
double g_aread_value = 0.0;             /* AREAD numeric value */
bool g_aread_is_string = false;         /* Whether AREAD value is a string */

/* Initialize VM */
void vm_init(void)
{
    g_vm.pc = NULL;
    g_vm.current_line = 0;
    g_vm.running = false;
    g_vm.expr_stack.top = 0;
    g_vm.call_stack.top = 0;
    g_vm.for_stack.top = 0;
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
    if (g_vm.expr_stack.top <= 0)
    {
        error_set(ERR_STACK_OVERFLOW, g_vm.current_line);
        return 0.0;
    }
    return g_vm.expr_stack.values[--g_vm.expr_stack.top];
}

/* Push call frame onto call stack */
void vm_push_call(uint8_t *return_pc, int return_line)
{
    if (g_vm.call_stack.top >= CALL_STACK_SIZE)
    {
        error_set(ERR_STACK_OVERFLOW, g_vm.current_line);
        return;
    }
    g_vm.call_stack.frames[g_vm.call_stack.top].return_pc = return_pc;
    g_vm.call_stack.frames[g_vm.call_stack.top].return_line = return_line;
    g_vm.call_stack.top++;
}

/* Pop call frame from call stack */
bool vm_pop_call(uint8_t **return_pc, int *return_line)
{
    if (g_vm.call_stack.top <= 0)
    {
        error_set(ERR_RETURN_WITHOUT_GOSUB, g_vm.current_line);
        return false;
    }
    g_vm.call_stack.top--;
    *return_pc = g_vm.call_stack.frames[g_vm.call_stack.top].return_pc;
    *return_line = g_vm.call_stack.frames[g_vm.call_stack.top].return_line;
    return true;
}

/* Push FOR frame onto FOR stack */
void vm_push_for(uint8_t *pc_after_for, uint8_t var_idx, double limit, double step)
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
bool vm_pop_for(uint8_t **pc_after_for, uint8_t *var_idx, double *limit, double *step)
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
    switch (g_angle_mode)
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
    switch (g_angle_mode)
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

/* Forward declarations for expression parsing */
static double parse_expression(uint8_t **pc_ptr, uint8_t *end);
static double parse_term(uint8_t **pc_ptr, uint8_t *end);
static double parse_power(uint8_t **pc_ptr, uint8_t *end);
static double parse_factor(uint8_t **pc_ptr, uint8_t *end);

/* Evaluate expression using recursive descent parser */
double vm_eval_expression(uint8_t **pc_ptr, uint8_t *end)
{
    return parse_expression(pc_ptr, end);
}

/* Parse expression: term ((+|-) term)* */
static double parse_expression(uint8_t **pc_ptr, uint8_t *end)
{
    double result = parse_term(pc_ptr, end);

    while (*pc_ptr < end)
    {
        uint8_t op = **pc_ptr;
        if (op == T_PLUS)
        {
            (*pc_ptr)++;
            result += parse_term(pc_ptr, end);
        }
        else if (op == T_MINUS)
        {
            (*pc_ptr)++;
            result -= parse_term(pc_ptr, end);
        }
        else
        {
            break;
        }
    }

    return result;
}

/* Parse term: power ((*|/) power)* */
static double parse_term(uint8_t **pc_ptr, uint8_t *end)
{
    double result = parse_power(pc_ptr, end);

    while (*pc_ptr < end)
    {
        uint8_t op = **pc_ptr;
        if (op == T_MUL)
        {
            (*pc_ptr)++;
            result *= parse_power(pc_ptr, end);
        }
        else if (op == T_DIV)
        {
            (*pc_ptr)++;
            double divisor = parse_power(pc_ptr, end);
            if (divisor == 0.0)
            {
                error_set(ERR_DIVISION_BY_ZERO, g_vm.current_line);
                return 0.0;
            }
            result /= divisor;
        }
        else
        {
            break;
        }
    }

    return result;
}

/* Parse power: factor (^ factor)* (right associative) */
static double parse_power(uint8_t **pc_ptr, uint8_t *end)
{
    double result = parse_factor(pc_ptr, end);

    if (*pc_ptr < end && **pc_ptr == T_POW)
    {
        (*pc_ptr)++;
        double exponent = parse_power(pc_ptr, end); /* Right associative */
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

/* Parse factor: number | variable | variable(expr) | (expr) | -factor */
static double parse_factor(uint8_t **pc_ptr, uint8_t *end)
{
    if (*pc_ptr >= end)
    {
        error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
        return 0.0;
    }

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
        double index_val = vm_eval_expression(pc_ptr, end);

        /* Skip T_ENDX terminator */
        if (*pc_ptr < end && **pc_ptr == T_ENDX)
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
        double result = parse_expression(pc_ptr, end);
        if (*pc_ptr < end && **pc_ptr == T_RP)
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
        return -parse_factor(pc_ptr, end);
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
        double arg = parse_expression(pc_ptr, end);
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
        double arg = parse_expression(pc_ptr, end);
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
        double arg = parse_expression(pc_ptr, end);
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
        double arg = parse_expression(pc_ptr, end);
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
        double arg = parse_expression(pc_ptr, end);
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
        double arg = parse_expression(pc_ptr, end);
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
        double arg = parse_expression(pc_ptr, end);
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
        double arg = parse_expression(pc_ptr, end);
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
        double arg = parse_expression(pc_ptr, end);
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
        double arg = parse_expression(pc_ptr, end);
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
        double arg = parse_expression(pc_ptr, end);
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
        double arg = parse_expression(pc_ptr, end);
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
        double arg = parse_expression(pc_ptr, end);
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
        double arg = parse_expression(pc_ptr, end);
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
        double arg = parse_expression(pc_ptr, end);
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

/* Evaluate condition for IF statement */
bool vm_eval_condition(uint8_t **pc_ptr, uint8_t *end)
{
    /* Parse left side of comparison */
    double left_val = vm_eval_expression(pc_ptr, end);

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
    double right_val = vm_eval_expression(pc_ptr, end);

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

/* Execute a single statement */
void vm_execute_statement(void)
{
    if (!g_vm.pc || !g_vm.running)
        return;

    uint8_t token = *g_vm.pc;
    g_vm.pc++;

    switch (token)
    {
    case T_VAR: /* Direct assignment: A = expr */
    {
        uint8_t var_idx = *g_vm.pc++;

        /* Expect = */
        if (*g_vm.pc != T_EQ_ASSIGN)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return;
        }
        g_vm.pc++;

        /* Evaluate right-hand side */
        uint8_t *line_end = program_find_line_end(g_vm.pc);
        double value = vm_eval_expression(&g_vm.pc, line_end);

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

        break;
    }

    case T_SVAR: /* String assignment: A$ = string_expr */
    {
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

        break;
    }

    case T_VIDX: /* Indexed assignment: A(expr) = expr */
    {
        /* T_VIDX always refers to variable A with indexing */
        /* No placeholder byte - directly parse the index expression */

        /* Evaluate index */
        uint8_t *line_end = program_find_line_end(g_vm.pc);
        double index_val = vm_eval_expression(&g_vm.pc, line_end);

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
        double value = vm_eval_expression(&g_vm.pc, line_end);

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
        break;
    }

    case T_SVIDX: /* Indexed string assignment: A$(expr) = string_expr */
    {
        /* T_SVIDX works like T_VIDX - parse expression between T_SVIDX and T_ENDX */
        uint8_t *line_end = program_find_line_end(g_vm.pc);
        double index_val = vm_eval_expression(&g_vm.pc, line_end);

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

        break;
    }

    case T_LET:
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
            uint8_t *line_end = program_find_line_end(g_vm.pc);
            double value = vm_eval_expression(&g_vm.pc, line_end);

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
            uint8_t *line_end = program_find_line_end(g_vm.pc);
            double index_val = vm_eval_expression(&g_vm.pc, line_end);

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
            double value = vm_eval_expression(&g_vm.pc, line_end);

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
        break;
    }

    case T_PRINT:
    {
        /* Simple PRINT implementation */
        uint8_t *line_end = program_find_line_end(g_vm.pc);

        while (g_vm.pc < line_end && *g_vm.pc != T_COLON && *g_vm.pc != T_EOL)
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
                double index_val = vm_eval_expression(&g_vm.pc, line_end);
                if (error_get_code() != ERR_NONE)
                    return;

                /* Skip T_ENDX terminator */
                if (g_vm.pc < line_end && *g_vm.pc == T_ENDX)
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
                double value = vm_eval_expression(&g_vm.pc, line_end);
                if (error_get_code() != ERR_NONE)
                    return;
                printf("%g", value);
            }
        }
        printf("\n");
        /* PRINT clears AREAD after displaying */
        g_aread_value = 0.0;
        g_aread_string[0] = '\0';
        g_aread_is_string = false;
        break;
    }

    case T_GOTO:
    {
        if (*g_vm.pc != T_NUM)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return;
        }
        g_vm.pc++;
        double line_num = *(double *)g_vm.pc;
        g_vm.pc += sizeof(double);

        /* Jump to line */
        uint8_t *target = program_find_line_tokens((int)line_num);
        if (!target)
        {
            error_set(ERR_BAD_LINE_NUMBER, g_vm.current_line);
            return;
        }

        g_vm.pc = target;
        g_vm.current_line = (int)line_num;
        return; /* Don't advance PC normally */
    }

    case T_IF:
    {
        /* IF condition [THEN line_number | statement] */
        uint8_t *line_end = program_find_line_end(g_vm.pc);

        /* Look for THEN token to determine IF type */
        uint8_t *then_pos = g_vm.pc;
        bool has_then = false;
        while (then_pos < line_end && *then_pos != T_THEN && *then_pos != T_EOL)
        {
            then_pos = token_skip(then_pos);
            if (!then_pos)
                break;
        }

        if (then_pos < line_end && *then_pos == T_THEN)
        {
            has_then = true;
        }

        if (has_then)
        {
            /* IF condition THEN line_number */
            bool condition = vm_eval_condition(&g_vm.pc, then_pos);
            if (error_get_code() != ERR_NONE)
                return;

            /* Skip to THEN */
            g_vm.pc = then_pos + 1;

            if (condition)
            {
                /* Must be followed by line number */
                if (*g_vm.pc != T_NUM)
                {
                    error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
                    return;
                }
                g_vm.pc++;
                double line_num = *(double *)g_vm.pc;
                g_vm.pc += sizeof(double);

                uint8_t *target = program_find_line_tokens((int)line_num);
                if (!target)
                {
                    error_set(ERR_BAD_LINE_NUMBER, g_vm.current_line);
                    return;
                }

                g_vm.pc = target;
                g_vm.current_line = (int)line_num;
                return; /* Don't advance PC normally */
            }
            else
            {
                /* Condition false - skip to end of line */
                g_vm.pc = line_end;
            }
        }
        else
        {
            /* IF condition statement (no THEN) */
            /* Parse: IF expression comparison_op expression statement */

            uint8_t *saved_pc = g_vm.pc;

            /* Parse first expression */
            vm_eval_expression(&g_vm.pc, line_end);
            if (error_get_code() != ERR_NONE)
                return;

            /* Skip comparison operator */
            if (g_vm.pc < line_end && (*g_vm.pc == T_EQ || *g_vm.pc == T_EQ_ASSIGN || *g_vm.pc == T_NE ||
                                       *g_vm.pc == T_LT || *g_vm.pc == T_LE ||
                                       *g_vm.pc == T_GT || *g_vm.pc == T_GE))
            {
                g_vm.pc++;
            }

            /* Parse second expression */
            vm_eval_expression(&g_vm.pc, line_end);
            if (error_get_code() != ERR_NONE)
                return;

            /* Now g_vm.pc should be at the statement */
            uint8_t *statement_pos = g_vm.pc;

            /* Reset PC and evaluate condition properly */
            g_vm.pc = saved_pc;
            error_clear(); /* Clear any errors from the parsing above */

            bool condition = vm_eval_condition(&g_vm.pc, statement_pos);
            if (error_get_code() != ERR_NONE)
                return;

            if (condition)
            {
                /* Execute the statement - PC should be at statement token */
                /* Statement will be executed in next iteration */
            }
            else
            {
                /* Condition false - skip to end of line */
                g_vm.pc = line_end;
            }
        }
        break;
    }

    case T_GOSUB:
    {
        if (*g_vm.pc != T_NUM)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return;
        }
        g_vm.pc++;
        double line_num = *(double *)g_vm.pc;
        g_vm.pc += sizeof(double);

        /* Push return address onto call stack */
        vm_push_call(g_vm.pc, g_vm.current_line);
        if (error_get_code() != ERR_NONE)
            return;

        /* Jump to subroutine */
        uint8_t *target = program_find_line_tokens((int)line_num);
        if (!target)
        {
            error_set(ERR_BAD_LINE_NUMBER, g_vm.current_line);
            return;
        }

        g_vm.pc = target;
        g_vm.current_line = (int)line_num;
        return; /* Don't advance PC normally */
    }

    case T_RETURN:
    {
        /* Pop return address from call stack */
        uint8_t *return_pc;
        int return_line;
        if (!vm_pop_call(&return_pc, &return_line))
        {
            return; /* Error already set */
        }

        g_vm.pc = return_pc;
        g_vm.current_line = return_line;
        return; /* Don't advance PC normally */
    }

    case T_FOR:
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

        /* Parse start expression */
        uint8_t *line_end = program_find_line_end(g_vm.pc);
        uint8_t *to_pos = g_vm.pc;

        /* Find TO keyword */
        while (to_pos < line_end && *to_pos != T_TO && *to_pos != T_EOL)
        {
            to_pos = token_skip(to_pos);
            if (!to_pos)
                break;
        }

        if (to_pos >= line_end || *to_pos != T_TO)
        {
            error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
            return;
        }

        /* Evaluate start expression */
        double start_val = vm_eval_expression(&g_vm.pc, to_pos);
        if (error_get_code() != ERR_NONE)
            return;

        /* Skip TO */
        g_vm.pc = to_pos + 1;

        /* Find STEP keyword or end of statement */
        uint8_t *step_pos = g_vm.pc;
        while (step_pos < line_end && *step_pos != T_STEP &&
               *step_pos != T_COLON && *step_pos != T_EOL)
        {
            step_pos = token_skip(step_pos);
            if (!step_pos)
                break;
        }

        /* Evaluate limit expression */
        double limit_val = vm_eval_expression(&g_vm.pc, step_pos);
        if (error_get_code() != ERR_NONE)
            return;

        /* Parse optional STEP */
        double step_val = 1.0; /* Default step */
        if (step_pos < line_end && *step_pos == T_STEP)
        {
            g_vm.pc = step_pos + 1; /* Skip STEP */
            uint8_t *stmt_end = step_pos;
            while (stmt_end < line_end && *stmt_end != T_COLON && *stmt_end != T_EOL)
            {
                stmt_end = token_skip(stmt_end);
                if (!stmt_end)
                    break;
            }
            step_val = vm_eval_expression(&g_vm.pc, stmt_end);
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
        uint8_t *pc_after_for;

        /* Check if there are more statements on the current line */
        if (g_vm.pc < line_end && *g_vm.pc == T_COLON)
        {
            /* There's a colon, so pc_after_for points to statement after colon */
            pc_after_for = g_vm.pc + 1; /* Skip the colon */
        }
        else
        {
            /* No more statements on current line, jump to next line */
            pc_after_for = line_end;
            if (*pc_after_for == T_EOL)
            {
                pc_after_for++; /* Skip T_EOL */
                /* Skip line header (len + line_num) to get to tokens */
                if (pc_after_for < g_program.prog + g_program.prog_len)
                {
                    pc_after_for += 2; /* Skip u16 len */
                    pc_after_for += 2; /* Skip u16 line_num */
                }
            }
        }

        vm_push_for(pc_after_for, var_idx, limit_val, step_val);
        if (error_get_code() != ERR_NONE)
            return;

        break;
    }

    case T_NEXT:
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

        uint8_t *pc_after_for;
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
            pc_after_for = frame->pc_after_for;
            frame_var_idx = frame->var_idx;
            limit = frame->limit;
            step = frame->step;

            /* Remove this frame and all frames above it */
            g_vm.for_stack.top = frame_index;
        }
        else
        {
            /* Unnamed NEXT - use top FOR frame */
            if (!vm_pop_for(&pc_after_for, &frame_var_idx, &limit, &step))
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
            vm_push_for(pc_after_for, frame_var_idx, limit, step);
            if (error_get_code() != ERR_NONE)
                return;

            g_vm.pc = pc_after_for;

            /* Update current line number if jumping to a different line */
            /* Iterate through all lines to find which contains pc_after_for */
            LineRecord *line = program_first_line();
            while (line)
            {
                uint8_t *line_tokens = line->tokens;
                uint8_t *line_end = program_find_line_end(line_tokens);
                if (pc_after_for >= line_tokens && pc_after_for < line_end)
                {
                    g_vm.current_line = line->line_num;
                    break;
                }
                line = program_next_line(line);
            }

            return; /* Don't advance PC normally */
        }
        /* Loop finished - continue to next statement */
        break;
    }

    case T_END:
    case T_STOP:
        g_vm.running = false;
        return;

    case T_REM:
        /* Skip to end of line */
        while (g_vm.pc < program_find_line_end(g_vm.pc) && *g_vm.pc != T_EOL)
        {
            g_vm.pc++;
        }
        break;

    case T_COLON:
        /* Statement separator - continue to next statement */
        break;

    case T_EOL:
    {
        /* End of line - find and advance to next line */
        LineRecord *current_record = program_find_line(g_vm.current_line);
        LineRecord *next_record = program_next_line(current_record);
        if (next_record)
        {
            g_vm.pc = next_record->tokens;
            g_vm.current_line = next_record->line_num;
        }
        else
        {

            g_vm.running = false;
        }
        break;
    }

    case T_INPUT:
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
            uint8_t *line_end = program_find_line_end(g_vm.pc);
            double index_val = vm_eval_expression(&g_vm.pc, line_end);
            if (error_get_code() != ERR_NONE)
                return;

            /* Skip T_ENDX terminator */
            if (g_vm.pc < line_end && *g_vm.pc == T_ENDX)
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
            uint8_t *line_end = program_find_line_end(g_vm.pc);
            double index_val = vm_eval_expression(&g_vm.pc, line_end);
            if (error_get_code() != ERR_NONE)
                return;

            /* Skip T_ENDX terminator */
            if (g_vm.pc < line_end && *g_vm.pc == T_ENDX)
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
        break;
    }

    case T_AREAD:
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
            uint8_t *line_end = program_find_line_end(g_vm.pc);
            double index_val = vm_eval_expression(&g_vm.pc, line_end);
            if (error_get_code() != ERR_NONE)
                return;

            /* Skip T_ENDX terminator */
            if (g_vm.pc < line_end && *g_vm.pc == T_ENDX)
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
            uint8_t *line_end = program_find_line_end(g_vm.pc);
            double index_val = vm_eval_expression(&g_vm.pc, line_end);
            if (error_get_code() != ERR_NONE)
                return;

            /* Skip T_ENDX terminator */
            if (g_vm.pc < line_end && *g_vm.pc == T_ENDX)
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
        break;
    }

    /* Mode statements */
    case T_DEGREE:
        g_angle_mode = ANGLE_DEGREE;
        break;

    case T_RADIAN:
        g_angle_mode = ANGLE_RADIAN;
        break;

    case T_GRAD:
        g_angle_mode = ANGLE_GRAD;
        break;

    /* Memory management */
    case T_CLEAR:
    {
        /* Clear all variables A-Z and indexed variables A(1) through A(VARS_MAX) */
        for (int i = 0; i <= VARS_MAX; i++)
        {
            g_program.vars[i].type = VAR_NUM;
            g_program.vars[i].value.num = 0.0;
        }
        break;
    }

    /* Device commands */
    case T_BEEP:
        putchar('\a');  /* ASCII bell character */
        fflush(stdout);
        break;

    case T_PAUSE:
    {
        /* PAUSE works exactly like PRINT, then waits 100ms */
        uint8_t *line_end = program_find_line_end(g_vm.pc);
        
        /* Print expressions just like PRINT statement */
        while (g_vm.pc < line_end && *g_vm.pc != T_EOL && *g_vm.pc != T_COLON)
        {
            if (*g_vm.pc == T_STR)
            {
                g_vm.pc++; /* Skip T_STR token */
                uint8_t len = *g_vm.pc++;
                for (int i = 0; i < len; i++)
                {
                    putchar(*g_vm.pc++);
                }
            }
            else if (*g_vm.pc == T_COMMA)
            {
                printf("\t"); /* Tab for comma separator */
                g_vm.pc++;
            }
            else if (*g_vm.pc == T_SEMI)
            {
                /* Semicolon - no extra space */
                g_vm.pc++;
            }
            else
            {
                /* Evaluate and print numeric expression */
                double value = vm_eval_expression(&g_vm.pc, line_end);
                if (error_get_code() != ERR_NONE)
                    return;
                
                /* Print number with same formatting as PRINT */
                if (value >= 0)
                    printf(" %.6g", value);
                else
                    printf("%.6g", value);
            }
        }
        
        printf("\n"); /* Always end with newline like PRINT */
        fflush(stdout);
        
        /* Wait 100ms */
        #ifdef _WIN32
            Sleep(100);  /* Windows */
        #else
            usleep(100000);  /* Unix: 100,000 microseconds = 100ms */
        #endif
        /* PAUSE clears AREAD after displaying */
        g_aread_value = 0.0;
        g_aread_string[0] = '\0';
        g_aread_is_string = false;
        break;
    }

    case T_USING:
        /* No-op for now - just ignore */
        break;

    default:
        error_set(ERR_SYNTAX_ERROR, g_vm.current_line);
        break;
    }
}

/* Run program */
void vm_run(void)
{
    /* Clear any previous errors */
    error_clear();

    /* Start at first line */
    g_vm.pc = program_first_line_tokens();
    if (!g_vm.pc)
    {
        printf("No program loaded\n");
        return;
    }

    /* Extract line number from record header */
    uint16_t line_num = *(uint16_t *)(g_vm.pc - 2);
    g_vm.current_line = line_num;

    g_vm.running = true;

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
