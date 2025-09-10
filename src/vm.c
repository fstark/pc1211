#include "vm.h"
#include "program.h"
#include "errors.h"
#include <stdio.h>
#include <math.h>
#include <stdint.h>

/* Global VM state */
VM g_vm;

/* Initialize VM */
void vm_init(void)
{
    g_vm.pc = NULL;
    g_vm.current_line = 0;
    g_vm.running = false;
    g_vm.expr_stack.top = 0;
    g_vm.call_stack.top = 0;
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
        (*pc_ptr)++; /* Skip placeholder byte */

        /* Evaluate index expression */
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

    case T_VIDX: /* Indexed assignment: A(expr) = expr */
    {
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
