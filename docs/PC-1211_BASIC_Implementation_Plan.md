# PC-1211 BASIC --- Implementation Plan

This plan builds the interpreter step by step, getting a core running
quickly and then extending until it matches the full **v0.5 spec**. All
with **static allocation only**.

------------------------------------------------------------------------

# Phase 0 --- Project skeleton (static-only groundwork)

**Goal:** Compile a no-op interpreter with the right scaffolding and
zero dynamic memory.

-   **Layout**

        /src
          Makefile               // simple build system → pc1211 executable
          tokenizer.c / .h
          vm.c / .h
          runtime.c / .h         // PRINT, INPUT, number/string helpers
          opcodes.h              // v0.5 enum + token metadata
          program.c / .h         // loader, line records, linear scan, LIST
          errors.c / .h          // error handling (Type 1/Type 2 errors)
          tests/
            t_runner.c           // tiny test runner (no deps)

-   **Build flags:**
    `-std=c99 -Wall -Wextra -Werror -pedantic -fno-builtin`

-   **Static buffers:** add the arrays defined in v0.5 (prog buffer 2048 bytes,
    variable cells, stacks). All variables init to NUM=0, STR="".

-   **CLI stub:** `pc1211 <source.bas>` → prints "loaded N lines".

-   **Error system:** Define Type 1 (runtime) and Type 2 (structural) error codes.

**Exit criteria:** Builds cleanly. Runs and prints a harmless banner. No
heap usage.

------------------------------------------------------------------------

# Phase 1 --- Loader + Tokenizer (core path)

**Goal:** Turn a `.bas` file into the v0.5 token buffer; no execution
yet.

-   Implement **record format**:
    `(u16 len)(u16 line)(tokens...)(T_EOL)`.
-   Tokenize **numbers, strings (≤7), VAR A..Z, A(expr) → VIDX**,
    operators, `:` and end-of-line.
-   Recognize **keywords + abbreviations** (e.g., `SIN` / `SI.`);
    case-insensitive; `√` maps to `SQR` (ASCII `SQR` accepted).
-   **String handling**: ASCII only, uppercase conversion. 7-char limit only applies to variable storage, not source literals or comments.
-   **Line numbers**: Range 1-999; duplicate lines replace previous.
-   **Line insertion**: O(n²) insertion maintaining memory order (authentic behavior).
-   Encode **statements**:
    -   `LET` optional for assignments, but required for conditional
        assignments: `IF A=3 LET B=42` (not `IF A=3 THEN LET B=42`).
    -   **PC-1211 IF syntax**: `IF condition THEN line_number` or 
        `IF condition statement` (store line as `u16` after THEN).
    -   `GOTO`, `GOSUB`, `RETURN`, `END`, `STOP`, `REM` (store comment text), `FOR/TO/STEP`,
        `NEXT`, `PRINT`, `INPUT` (parse as BASIC expression).
-   Reject/ignore (for now): `DEGREE/RADIAN/GRAD`, `CLEAR`, `BEEP`,
    `PAUSE`, `AREAD`, `USING`.
-   **DMS/DEG functions**: Implement as stubs with assert(false) for now.

**Tests (token-only):** 
- Round-trip dump: a **token disassembler** that prints a readable listing, to verify encodings. 
- **LIST command**: Early implementation for debugging internal representation.
- Edge cases: long line up to `TOKBUF_LINE_MAX`, long strings in source (REM comments, literals), mixed abbreviations.
- Line insertion: Test duplicate line replacement and memory reordering.

**Exit criteria:** Token dump of sample programs matches expectations. LIST command works.

------------------------------------------------------------------------

# Phase 2 --- Minimal VM loop + numeric expressions

**Goal:** Execute **assignment, PRINT, GOTO, END** with numeric
expressions.

-   Implement **expression evaluator** (array-based shunting-yard):
    -   Literals, VAR, VIDX (index = trunc toward 0).
    -   Enforce `1 <= index <= VARS_MAX`. Anything else is **Type 1 error**.
    -   Operators: `+ - * / ^`, `()`; precedence + right-assoc for `^`.
    -   **Division by zero**: Type 1 error.
    -   **Math overflow**: Type 1 error; **underflow**: round to 0.
-   Implement statements:
    -   `LET` (`=` assignment), `PRINT` (basic spacing), `GOTO`, `END`,
        `STOP`, `REM`.
    -   **GOTO to non-existent line**: Type 2 error.
-   VM stepping: `:` continues in-line; `T_EOL` jumps to next record;
    **line scan** for jumps.
-   **Error handling**: Stop execution, display line number and error code.

**Tests:** - Simple math + print; multi-statement lines with `:`. - A(n)
for indices in 1..VARS_MAX; error if outside range.

**Exit criteria:** Numeric programs without branches run correctly.

------------------------------------------------------------------------

# Phase 3 --- Control flow core: IF/THEN, GOSUB/RETURN

**Goal:** Conditional logic + subroutines.

-   Add comparison ops for IF: `= <> < <= > >=`.
    -   **Numeric comparisons**: All operators work.
    -   **String comparisons**: Only `=` and `<>` allowed.
    -   **Mixed comparisons**: Type 1 error (no coercion).
-   **PC-1211 IF syntax**: `IF condition THEN line_number` (conditional GOTO)
    or `IF condition statement` (conditional execution, no THEN keyword).
-   `GOSUB`/`RETURN` with **fixed-depth** static stack (errors on
    under/overflow).

**Tests:** - `IF condition THEN line_number` (conditional GOTO); 
`IF condition statement` (conditional execution, no THEN). 
- Nested GOSUB (depth >1), error on `RETURN` without `GOSUB`.

**Exit criteria:** Branching and subroutines stable. ✅ **COMPLETED**

## Post-Phase 7 Enhanced GOTO/GOSUB Implementation

The PC-1211 BASIC interpreter now supports comprehensive GOTO/GOSUB functionality that goes beyond the original specification:

### 1. Expression-Based GOTO/GOSUB ✅ **COMPLETED**
GOTO/GOSUB now accept **expressions** instead of just literal line numbers:
- `GOTO A*10+50` - Jump to calculated line number
- `GOSUB B+200` - Call subroutine at calculated line number
- Full expression support: variables, arithmetic, parentheses

### 2. String Literal Labels ✅ **COMPLETED**  
Lines can now start with string literals as labels:
- `"MAIN" PRINT "Hello World"` - Define a label at line start
- `GOTO "MAIN"` - Jump to string literal label
- `GOSUB "SUBROUTINE"` - Call subroutine with string literal label
- Labels are case-insensitive and limited to 7 characters (PC-1211 string limit)

### 3. Computed Labels (String Variables) ✅ **COMPLETED**
String variables can be used as dynamic label targets:
- `A$="EXIT": GOTO A$` - Jump to label stored in string variable
- `B$="SUB1": GOSUB B$` - Call subroutine with label from string variable
- Full dynamic programming support with string variable evaluation

### 4. Mixed Label/Line Number Support ✅ **COMPLETED**
Programs can mix traditional line numbers with string labels seamlessly:
```basic
10 A$="FINISH"
20 GOTO 100        ; Traditional line number
30 PRINT "Never reached"
100 GOTO "MIDDLE"  ; String literal label  
200 "MIDDLE" GOSUB A$  ; Computed label from variable
300 END
400 "FINISH" PRINT "Done!": RETURN
```

### 5. Comprehensive Error Handling ✅ **COMPLETED**
- **Type Mismatch (Error 5)**: When string variable contains non-string data
- **Bad Line Number (Error 2)**: When label or computed line number doesn't exist
- **Buffer Overflow Protection**: Labels exceeding 7 characters are truncated
- **Runtime Validation**: Label existence checked at execution time

### Implementation Details
- **Label Table**: Dynamic label→line mapping with 100 label capacity
- **Token Support**: T_STR for string literals, T_SVAR for string variables
- **Memory Management**: Automatic label table clearing on program load
- **Performance**: O(n) label lookup with linear search through label table

### Test Coverage
All functionality verified with comprehensive test suite:
- Expression GOTO/GOSUB with mathematical operations
- String literal labels with GOTO/GOSUB targeting
- Computed labels using string variables
- Mixed usage scenarios with labels and line numbers
- Error conditions and boundary cases

------------------------------------------------------------------------

# Phase 4 --- FOR/NEXT (unstructured-friendly)

**Goal:** Loops that tolerate PC-1211 unstructured flow (e.g., `NEXT`
elsewhere).

-   `FOR v = start TO limit [STEP step]`:
    -   Push frame `{pc_after_for, var_idx, limit, step}`; assign
        `v=start`.
    -   **STEP = 0**: Type 1 error.
    -   **Same-line support**: `FOR I=1 TO 5:PRINT I` correctly sets
        `pc_after_for` to statement after colon, not next line.
-   `NEXT [v]`:
    -   If named: find matching topmost frame by var; if unnamed: use
        top.
    -   `v += step`; continue if
        `(step>0 && v<=limit) || (step<0 && v>=limit)` → jump to
        `pc_after_for`; else pop.
-   **Don't** enforce structural pairing at tokenize time.

**Tests:** - `FOR I=1 TO 5:PRINT I` (same-line statements). 
- Unstructured "NEXT at target line" example. - Nested loops;
negative step; `NEXT` without `FOR` error.

**Exit criteria:** Looping semantics match examples, including
cross-line `NEXT`. ✅ **COMPLETED**

------------------------------------------------------------------------

# Phase 5 --- Strings (7 chars) + INPUT

**Goal:** Full cell typing (NUM or STR) and basic input.

-   Cell type overwrite: assigning string replaces numeric and vice
    versa.
-   `PRINT` prints strings as-is (no special formatting for now).
-   `INPUT` parses as full BASIC expression and evaluates; any error aborts input.
    Supports `VAR` and `VIDX` targets. Strings: ASCII, uppercase, max 7 chars.

**Tests:** - Assign/print string; overwrite with number; vice versa. -
Mixed `INPUT A, A(27), B`.

**Exit criteria:** String behavior matches spec and truncation rules. ✅ **COMPLETED**

------------------------------------------------------------------------

# Phase 6 --- Full function set (numeric)

**Goal:** Implement all function opcodes from v0.5.

-   **Trig (radians)**: `SIN, COS, TAN` - standard libc implementations.
-   **Inverse trig**: `ASN(asin), ACS(acos), ATN(atan)` - use libc, natural domain errors.
-   **Logs/exp**: `LOG(10), LN, EXP` - use libc, natural domain errors.
-   **Math functions**:
    -   `SQR` (sqrt) - use libc, natural domain errors.
    -   `INT` (floor function) - `INT(-3.7) = -4`, always rounds down.
    -   `ABS` - absolute value, standard implementation.
    -   `SGN` - sign function returning -1, 0, or 1.
-   **Conversions**:
    -   `DMS(x)` - decimal degrees → DD.MMSS format (15.4125 → 15.2445).
    -   `DEG(x)` - DD.MMSS format → decimal degrees (15.2445 → 15.4125).
    -   DMS format: integer=degrees, digits 1-2=minutes, 3-4=seconds, 5+=fractional seconds.
-   **Domain errors**: Use libc behavior, generate ERR_MATH_DOMAIN for invalid inputs.

**Tests:** - Spot-check values; `INT(-3.7) = -4`; `SGN` at −1, 0, +1.
- `DMS/DEG` round trips: `DEG(DMS(15.4125)) = 15.4125`.

**Exit criteria:** All functions callable in expressions; no crashes on
domain errors; DMS/DEG conversions work correctly. ✅ **COMPLETED**

------------------------------------------------------------------------

# Phase 7 --- Mode & device statements ✅ COMPLETE

**Goal:** Full implementation of angle modes and device statements.

-   **Angle mode system**: ✅ IMPLEMENTED
    -   `DEGREE`: Set trig mode so `COS(90) = 0` (90° = π/2 rad = 100 grad).
    -   `RADIAN`: Set trig mode to radians (default).
    -   `GRAD`: Set trig mode to gradians (400 grad = 360°).
    -   Affects 6 trig functions: SIN, COS, TAN, ASN, ACS, ATN.
    -   Does NOT affect DMS/DEG conversion functions.
-   **Memory management**: ✅ IMPLEMENTED
    -   `CLEAR`: Set all variables to 0 (A-Z, A(1)-A(VARS_MAX), clears A$-Z$).
    -   Does not change angle mode or program code.
-   **AREAD statement**: ✅ IMPLEMENTED
    -   `AREAD variable`: Read previous screen value into variable.
    -   Supports: `AREAD A`, `AREAD B$`, `AREAD A(5)`, `AREAD B$(3)`.
    -   Command line: `--aread-value N` (numeric) or `--aread-string S` (string).
    -   Type conversion: string→numeric, numeric→string as needed.
    -   Consumed on first use; PRINT/PAUSE clear AREAD to 0.
-   **Device commands**: ✅ IMPLEMENTED
    -   `BEEP`: Emit ASCII bell character (^G).
    -   `PAUSE expr_list`: Like PRINT but waits 100ms after displaying.
    -   `USING`: No-op (ignored).

**Tests:** ✅ All mode/device statements working correctly. Angle conversions
verified: SIN(90°) = SIN(π/2 rad) = SIN(100 grad) = 1. AREAD handles both
numeric and string inputs with proper type conversion.

**Exit criteria:** ✅ COMPLETE - All mode/device statements functional; angle 
modes work correctly with trig functions; AREAD supports both string and 
numeric command-line inputs.

------------------------------------------------------------------------

# Phase 8 --- Diagnostics, tools, and polish

**Goal:** Make this a comfortable ref implementation.

-   Uniform **error reporting** with current **line number** and error code.
    Type 1 (runtime) and Type 2 (structural) error classification.
-   **Disassembler**: `--dump` prints token records; `--run` executes.
-   **LIST command**: Complete implementation for program listing.
-   **Tiny monitor** (optional): `--trace` prints `LINE:PC op` while
    stepping.
-   **Makefile**: Simple build system producing `pc1211` executable.
-   **Static assertions**: ensure buffer sizes and struct layouts at
    compile time.
-   **Conformance test pack**: each feature has a `.bas` and expected
    stdout.

**Exit criteria:** You can diagnose issues quickly; a small suite passes
consistently.

------------------------------------------------------------------------

# Phase 9 --- Spec compliance sweep (v0.5)

**Goal:** Verify behavior matches the doc.

-   Re-run all earlier tests + new edge cases:
    -   Mixed numeric/string comparisons → Type 1 error (no coercion).
    -   String comparisons: only `=`/`<>` allowed for strings.
    -   Indexing: `A(0)`/negatives → Type 1 error; `n > VARS_MAX` → Type 1 error.
    -   Stack limit errors for FOR/GOSUB.
    -   Line number range: 1-999, non-existent GOTO → Type 2 error.
    -   Math domain errors → Type 1 error.
    -   FOR STEP=0 → Type 1 error.
-   Lock **opcode enum** and **keyword table**; freeze as "v0.5".

**Exit criteria:** Green across the suite; frozen ABI for tokens.

------------------------------------------------------------------------

## Cross-cutting guidance

-   **Static-only rule:** No `malloc`. All buffers have compile-time
    sizes; fail with clear errors when exceeded (program too large, line
    too long, stack overflow). Program space: 2048 bytes.
-   **Variables:** Initialize to NUM=0, STR="". Support up to VARS_MAX cells.
-   **Strings:** ASCII only, uppercase, max 7 chars, no coercion.
-   **Line numbers:** Range 1-999. Duplicate lines replace previous.
-   **Error handling:** Stop on error, display line number and error code.
-   **Determinism:** Avoid locale/UB; use `strtod` for numbers;
    `printf("%g")` for output unless you add `USING` later.
-   **Token table:** Keep a single authoritative `opcodes.h` (your v0.5
    enum). Tokenizer and VM include it---no drift.
-   **Indexing `A(n)`:** Only `A` supports `(...)`; `n` is **1-based**;
    truncate toward 0; require `1 <= n <= VARS_MAX`.

------------------------------------------------------------------------

## Suggested initial test set (tiny)

1.  **Hello math**

        10 A=1.5
        20 PRINT A*2, A^2; : PRINT A
        30 END

2.  **Flow**

        10 I=0
        20 IF I=0 THEN 100
        30 PRINT 0 : END
        100 PRINT 1 : END

3.  **FOR/NEXT across lines**

        10 FOR I=0 TO 2
        20 IF I=0 THEN 100
        30 PRINT I
        40 NEXT I
        50 END
        100 NEXT I

4.  **Strings & cells**

        10 A="ABCDEFGH" : PRINT A
        20 A=3.14 : PRINT A
        30 A(27)="OK" : PRINT A(27)
        40 END

5.  **Functions**

        10 PRINT INT(-3.7), SGN(-2), SGN(0), SGN(5)
        20 REM DMS/DEG will assert for now
        30 PRINT SQR(16), ABS(-5)
        40 END

6.  **Error handling**

        10 A="TEST" : B=5
        20 IF A=B THEN 50    REM Type 1 error: mixed comparison
        30 PRINT "Should not reach"
        50 END

7.  **LIST command test**

        10 REM This is a comment
        20 FOR I=1 TO 3
        30 PRINT I
        40 NEXT I

------------------------------------------------------------------------

## What you'll have after Phase 4 (core working)

-   File-in tokenizer producing stable token buffer
-   VM executing: assignment, numeric expressions, PRINT, IF/THEN
    (line+stmt), GOTO, GOSUB/RETURN, FOR/NEXT (unstructured), END/STOP,
    REM
-   Strings (≤7) stored in same cells, A(n) indexing, INPUT
-   Linear line scan; PC is a token pointer; all **static**

Then Phases 6--7 finish the **full v0.5**: complete functions + safe
stubs (or real) for mode/device statements.

------------------------------------------------------------------------

## Recommended Implementation Strategy: Grouped Work Items

### **Work Item 1: Foundation (Phases 0-1)**
**Goal:** Get a working tokenizer with basic infrastructure

- **Phase 0**: Project skeleton, build system, static buffers
- **Phase 1**: Complete tokenizer + LIST command for debugging

**Why group these:** You need both to have anything meaningful to test. The LIST command from Phase 1 is essential for debugging the tokenizer output.

**Deliverable:** `pc1211 program.bas --dump` shows tokenized output, `pc1211 program.bas --list` shows readable program listing.

### **Work Item 2: Core VM (Phases 2-4)**  
**Goal:** Get a functional BASIC interpreter for structured programs

- **Phase 2**: Expression evaluation + basic statements (LET, PRINT, GOTO, END)
- **Phase 3**: Control flow (IF/THEN, GOSUB/RETURN) 
- **Phase 4**: Loops (FOR/NEXT with unstructured support)

**Why group these:** These phases build on each other incrementally, and you'll want to test them together. By the end, you have a complete structured programming environment.

**Deliverable:** Can run non-trivial BASIC programs with loops, conditionals, and subroutines.

### **Work Item 3: Complete Language (Phases 5-7)** ✅ COMPLETE
**Goal:** Full PC-1211 BASIC compatibility  

- **Phase 5**: String handling + INPUT ✅ COMPLETE
- **Phase 6**: Complete function library (math functions) ✅ COMPLETE  
- **Phase 7**: Mode commands and device statements ✅ COMPLETE

**Why group these:** These are "feature completion" phases that don't fundamentally change the VM architecture.

**Deliverable:** ✅ ACHIEVED - Full PC-1211 BASIC v0.5 specification compliance with angle modes, AREAD statement, and all math functions.

### **Recommended Starting Point: Work Item 1**

Start with **Work Item 1 (Phases 0-1)** because:

1. **Quick win**: You'll have a working tokenizer and can see your programs being parsed
2. **Foundation**: Establishes all the infrastructure patterns for the rest
3. **Debugging capability**: LIST command lets you verify the tokenizer is working correctly
4. **Natural checkpoint**: After this, you can manually verify that any BASIC program tokenizes correctly

After Work Item 1, you'll have:
- ✅ Complete build system
- ✅ Working tokenizer for all PC-1211 BASIC syntax  
- ✅ LIST command to verify tokenization
- ✅ Error handling framework
- ✅ All static data structures defined
