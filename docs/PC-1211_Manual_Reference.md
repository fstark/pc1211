# Sharp PC-1211 Instruction Manual — Extracted Reference

> Source: `reference/Sharp PC-1211 - Instruction Manual.pdf` (106 pages, OCR text)
> Page numbers in [brackets] refer to the original PDF for visual verification.

---

## 1. Specifications [p.97]

- **Model**: PC-1211
- **Digits**: 10 (mantissa) + 2 (exponent)
- **Calculation system**: Formula entry with priority judging
- **Language**: BASIC
- **Program memory**: Max 1424 steps (shared with flexible data memory)
- **Fixed memory**: 26 variables (A–Z / A$–Z$)
- **Flexible memory**: Max 178 (shared with program memory, 8 steps = 1 memory)
- **Reserve memory**: Max 48 steps (max 18 reserve programs)
- **Input buffer**: 80 characters
- **Data stack**: 8 stages
- **Function stack**: 16 stages (parentheses up to 15 levels)
- **Subroutine stack**: 4 stages
- **FOR-NEXT stack**: 4 stages
- **Display**: 24-digit alphanumeric dot matrix LCD
- **Line numbers**: 1–999

---

## 2. Modes [p.14]

The PC-1211 has four modes, cycled by the MODE key:

- **DEF** (Definable): Execute defined programs via labeled keys
- **RUN**: Execute programs or manual calculations
- **PRO** (Program): Write/edit programs
- **RESERVE**: Write/edit reserve programs

---

## 3. Computing Range [p.15]

```
-9.999999999 × 10^99  to  -1 × 10^-99,  0,  1 × 10^-99  to  9.999999999 × 10^99
```

- **Overflow** (|result| > 9.999999999 × 10^99): Error (code 1)
- **Underflow** (0 < |result| < 1 × 10^-99): Treated as 0

Internal storage: `A × 10^B` where `1 ≤ |A| ≤ 9.999999999`, `-99 ≤ B ≤ 99`, or `0`.

**Display rules** (manual calculation):
- Fixed decimal: `-9999999999 ≤ x ≤ -1×10^-9`, `x = 0`, `1×10^-9 ≤ x ≤ 9999999999`
- Numbers outside this range: floating decimal (scientific notation)
- Input mantissa: upper 10 digits effective
- Input exponent: last 2 digits effective

---

## 4. Priority of Calculations [p.27–28]

From highest to lowest priority:

1. Recalling π and fixed memories A–Z
2. Recalling dimension memories A()
3. Power directly preceded by an implicit multiplication: `2A^3` = `2 * (A^3)`
4. Implicit multiplication (omitted `*`): `2A`, `πB`, `AB`
5. Functions: SIN, COS, TAN, ASN, ACS, ATN, LN, LOG, EXP, DMS, DEG, INT, ABS, SGN, √
6. Power (`^`) — other than case 3
7. Unary sign (+, -)
8. Multiplication and division (`*`, `/`)
9. Addition and subtraction (`+`, `-`)
10. Logical/relational operators (`=`, `>`, `<`, `>=`, `<=`, `<>`)

**Rules:**
- Parentheses override priority (innermost first)
- Compound functions (`LN ABS A`, `EXP √B`) evaluate right to left
- Chained powers (`3^4^2`) evaluate right to left
- Implicit multiplication has priority over functions: `SIN 2A` = `SIN(2*A)`
- But when implicit multiplication is directly followed by power, power wins: `2πA^3` = `2*π*(A^3)`

**Stack example** [p.28]: `1.2 + A * (3.5 + SIN B) ^ A(25)` — detailed trace of data stack and function stack behavior provided in manual.

---

## 5. Variables and Memory [p.47–52]

### Variable Types

- **Numerical variable**: A–Z, or A(1)–A(204). Stores numbers.
- **Character variable**: A$–Z$, or A$(1)–A$(204). Stores up to 7 characters.
- **Type mismatch**: Assigning numeric to character variable or vice versa → Error 1 (except when variable is cleared/zero).

### Fixed vs. Flexible Memory [p.49]

- **Fixed memories**: A–Z (serial numbers 1–26). `A(1)` = `A`, `A(2)` = `B`, ..., `A(26)` = `Z`. Same for A$().
- **Flexible memories**: A(27)–A(204). Share space with program memory. 8 program steps = 1 flexible memory.
- Values in parentheses: only integer part is effective.
- If specified value < 1 or exceeds available flexible memory range → Error 4.

### Indirect Designation [p.49–50]

- `A(A)` — specifies the numerical variable whose serial number is the value of A
- `A$(A(3))` — specifies the character variable whose serial number is the value of C (memory 3)
- Can nest up to 15 levels deep: `A(A(A(C)))` — resolves chain of indirections

### Initialization

- Numerical variables start at 0
- Character variables start as empty string ""

### CLEAR vs. NEW [p.52]

- **CLEAR**: Clears all data memories (fixed and flexible). Program and reserve memories protected.
- **NEW**: Clears program memory AND data memory (in DEF/RUN/PRO modes), or reserve memory (in RESERVE mode).

---

## 6. Program Structure [p.32–33]

### Line Format
```
[line-number] [label] [statement] : [statement] : ... ENTER
```

- **Line numbers**: Integers 1–999, always required, stored as 2-step information
- **Labels**: String literals in quotes following line number (e.g., `10 "MAIN" PRINT A`)
- **Statements**: Separated by colons (`:`)
- **Max line length**: 80 steps
- **Statement keywords** (INPUT, PRINT, etc.) are each stored as 1 step internally
- Lines are stored in numerical order regardless of entry order
- The colon after the line number in display is automatic (not stored in memory)

### Program Memory [p.33]

- Total capacity: 1424 steps
- Programs stored step-by-step contiguously
- Lines ordered numerically in memory
- Inserting/deleting lines shifts subsequent content

---

## 7. Functions [p.18–22, p.98]

### Trigonometric [p.18–19]
| Function | Abbreviation | Description |
|----------|-------------|-------------|
| SIN | SI. | Sine |
| COS | — | Cosine |
| TAN | TA. | Tangent |
| ASN | AS. | Arcsine (sin⁻¹) |
| ACS | AC. | Arccosine (cos⁻¹) |
| ATN | AT. | Arctangent (tan⁻¹) |

Angular mode affects SIN/COS/TAN input and ASN/ACS/ATN output.

### Logarithmic/Exponential [p.20]
| Function | Abbreviation | Description |
|----------|-------------|-------------|
| LN | — | Natural logarithm (logₑ) |
| LOG | LO. | Common logarithm (log₁₀) |
| EXP | EX. | e^x (antilogarithm of LN) |
| √ | — | Square root |

### Angle Conversion [p.20–21]
| Function | Abbreviation | Description |
|----------|-------------|-------------|
| DMS | DM. | Decimal degrees → degrees/minutes/seconds |
| DEG | — | Degrees/minutes/seconds → decimal degrees |

**DMS format**: Integer part = degrees, 1st–2nd decimal = minutes, 3rd–4th decimal = seconds, remaining = decimal seconds.

### Numeric [p.21]
| Function | Abbreviation | Description |
|----------|-------------|-------------|
| INT | — | Largest integer not exceeding the value. INT(12.34)=12, INT(-2.45)=-3 |
| ABS | AB. | Absolute value |
| SGN | SG. | Sign: +1 if x>0, 0 if x=0, -1 if x<0 |

### Function Syntax Rules [p.19]

- Constant or memory argument: `SIN 30`, `SIN A` (no parentheses needed)
- Expression argument: `LN(A*B)`, `SIN(π/2)` (parentheses required)
- Compound functions evaluate right-to-left: `LN ABS A` = `LN(ABS(A))`

---

## 8. Statements — Detailed Semantics

### 8.1 LET [p.53]

Assignment statement. LET keyword can be omitted **except** immediately after IF.

**General form (1)**: `LET [numerical-var] = <expression>`
```
LET A = 123       → stores 123 in A
A(30) = 3*6       → stores 18 in A(30)
A(2*B) = C + D    → stores C+D in A(2*B)
```

**General form (2)**: `LET [char-var] = "characters"`
```
LET Z$ = "BASIC"  → stores "BASIC" in Z$
```
If string exceeds 7 characters, first 7 are stored, rest discarded.

**General form (3)**: `LET [char-var] = [char-var]`
```
LET A$(25) = Z$   → copies Z$ contents into A$(25)
```

**Multi-assignment** with commas (LET cannot appear after the comma):
```
10: LET A=2, B=7, C$="A=2 B=7"
```

### 8.2 INPUT [p.54–56]

Data input during program execution.

**General form (1)**: `INPUT [var], [var], ...`
- Displays `?` and waits for keyboard input
- Repeats for each variable

**General form (2)**: `INPUT "message", [var], "message", [var], ...`
- Displays the message instead of `?`
- Message disappears when data is entered

**General form (3)**: `INPUT "message"; [var], "message"; [var], ...`
- Semicolon (`;`) after message: message stays visible, data appears after it
- Example: `INPUT "DATA E=";E` → displays `DATA E=` then shows typed value after it

**Rules:**
- Forms (1)–(3) can be mixed: `INPUT A, "B=", B, "WHO?"; C$`
- No limit on message length
- CA key during input: form (1)/(2) reshows `?` or message; form (3) reshows with message
- Variables must be fixed memory, dimension with fixed memory index, or integer index. Cannot use `A(A(30))` or `A(5*9)` in INPUT.

**Skip operation**: Pressing ENTER without input skips remaining statements on the line, goes to next line.

### 8.3 PRINT [p.56–58]

Output instruction. Displays specified content, then pauses for ENTER key.

**General form (1)**: `PRINT <expression>` — displays numeric result

**General form (2)**: `PRINT "characters"` — displays string literal

**General form (3)**: `PRINT [char-var]` — displays contents of character variable

**General form (4)**: `PRINT X, Y` — 24-digit display split into two 12-digit halves (left and right). First item on right, second on left.
- Numeric values right-justified in their 12-digit section
- Characters left-justified in their 12-digit section
- Excess characters truncated, decimal digits trimmed for numbers

**General form (5)**: `PRINT X; Y; Z; ...` — semicolon-separated items displayed consecutively from left
- Example: `PRINT "TOTAL="; B; " QTY="; C`

**Rules:**
- When display exceeds 24 characters, only first 24 shown
- 2nd and subsequent items in forms (4) and (5) must be simple variables (fixed memory, dimension with fixed/integer index)

### 8.4 PAUSE [p.58]

Same general forms as PRINT, but displays for ~0.85 seconds then auto-continues (no ENTER needed).

### 8.5 USING [p.59–60]

Format specification for numeric output in PRINT/PAUSE.

**General form (1)**: `USING "###.##"` — sets format
- `#` = digit position
- `.` = decimal point position
- `^` (at end) = floating-point (scientific) notation
- Integer `#` count includes sign position

**General form (2)**: `USING` (at statement end) — cancels format, returns to default display

**General form (3)**: Inline with PRINT/PAUSE:
- `PRINT USING "###.##"; A` — format + display in one statement
- `PRINT USING; A` — cancel format + display

**Format examples** (for value 123.4567891) [p.60]:
| Format | Display | Notes |
|--------|---------|-------|
| (cancel) | 123.4567891 | |
| `#` or `##` | Error (code 6) | Too few digits |
| `####` | 123 | |
| `####.` | 123. | |
| `######.#` | 123.4 | |
| `#######.###` | 123.456 | |
| `####.#######` | 123.4567891 | |
| `####.##########` | 123.456789100 | Trailing zeros |
| `##.###^` | 1.234E 02 | Scientific |
| `.#########^` | 1.234567891E 02 | Scientific |

**Error**: If value cannot fit the format → Error 6.

**Note** [p.61]: In PRINT/PAUSE general form (4) with two values, if USING is applied, both values use the format of the latter value.

### 8.6 GOTO [p.61–62]

Jump instruction. **A line must end with a GOTO statement — no statement can follow it.**

**General form (1)**: `GOTO <expression>`
- Jumps to line number from integer part of expression
- Valid range: 1–999, other values → Error 2
- Example: `GOTO 10`, `GOTO 5*9`, `GOTO A`

**General form (2)**: `GOTO "label"` or `GOTO A$`
- Jumps to line with matching label
- Label effective up to 7 characters (excess ignored)
- If label not found → Error 2

### 8.7 IF [p.62–63]

Decision instruction. If condition is true, executes the remaining statements on the line. If false, skips to next line.

**General form (1)**: `IF <expr> <relop> <expr> <execution-statements>`
- Relational operators: `<`, `<=`, `=`, `>`, `>=`, `<>`
- True (result = 1): execute following statements
- False (result = 0): skip to next line

**Example**: `40 IF A*B>=C PAUSE A*B: GOTO 90`
- If true: executes PAUSE and GOTO
- If false: skips entire rest of line, goes to line 50

**General form (2)**: `IF <expression> <execution-statements>`
- Expression > 0: true (execute)
- Expression ≤ 0: false (skip to next line)
- Example: `IF A GOTO 80` — jumps if A > 0

**General form (3)**: `IF <string1> = <string2> <execution-statements>`
- String comparison (equality only)
- Strings can be literals or character variables
- Only first 7 characters compared
- Example: `IF A$="GUARD" GOTO 100`

**General form (4)**: `IF [char-var] <execution-statements>`
- True if character variable contains any characters
- False if character variable is empty
- Example: `IF A$ GOTO 80`

**Important**: LET is **required** after IF: `IF B>C LET B=B+1`

### 8.8 THEN [p.63]

Alternative to GOTO in IF statements: `IF B>=C THEN 50` is equivalent to `IF B>=C GOTO 50`.

Same general forms as GOTO. Can only appear as execution statement in IF.

### 8.9 GOSUB / RETURN [p.64]

**GOSUB**: Same target forms as GOTO (expression, string literal, string variable), but:
- Pushes return address
- **Can be followed by other statements** (unlike GOTO)
- Max nesting depth: 4 stages

**RETURN**: Must be at end of line (no statements can follow).
- Returns to statement after the GOSUB
- RETURN without preceding GOSUB → Error 3

### 8.10 FOR / NEXT [p.66–68]

**General form (1)**: `FOR [num-var] = <expr1> TO <expr2>`
**General form (2)**: `FOR [num-var] = <expr1> TO <expr2> STEP <expr3>`

- `<expr1>`: initial value
- `<expr2>`: end value — **integer part only**, limited to < 3 digits
- `<expr3>`: increment — **integer part only**, limited to < 3 digits. If integer part = 0 → Error 1
- Default STEP = 1
- Positive step: loops while variable ≤ end value
- Negative step: loops while variable ≥ end value

**NEXT [num-var]**: Must match corresponding FOR variable.

**Nesting**: Max 4 stages deep.

**Error conditions**:
- Crossing FOR-NEXT loops → Error 4 (e.g., FOR A / FOR B / NEXT A / NEXT B)
- Joining a loop mid-course (GOTO into loop body) → Error 4 when NEXT is reached
- NEXT without matching FOR → Error 3

**Performance note** [p.67]: Using memories W(23) and later for loop variables is faster than earlier memories.

### 8.11 STOP [p.70]

`STOP` — Temporarily halts execution, displays `BREAK AT <line>`. Manual calculation is possible. Use `CONT` to resume.

### 8.12 END [p.70]

`END` — Terminates program execution, displays prompt.

### 8.13 BEEP [p.70]

`BEEP <expression>` — Beeps N times (positive integer part of expression).

### 8.14 CLEAR [p.70]

`CLEAR` — Clears all data memories (fixed + flexible). Program and reserve memories protected.

### 8.15 DEGREE / RADIAN / GRAD [p.71]

Set angular mode for trig functions:
- `DEGREE` — degrees (90° = π/2 rad = 100g)
- `RADIAN` — radians
- `GRAD` — gradians

### 8.16 AREAD [p.71–72]

`AREAD [variable]` — Reads the display content into a variable at program start.

- Only effective as the **first statement encountered** in a DEF-mode program
- Skipped if not at the head of a definable program
- Can read into numeric or character variable

**Display-to-variable mapping** [p.72]:
- For PRINT general form (4): reads the right-hand value
- For PRINT general form (5): reads the first value

### 8.17 REM [p.72]

`REM <note>` — Comment statement. Line content after REM is skipped during execution but stored in program memory.

---

## 9. Commands [p.72–75]

Commands are immediate-mode instructions executed with ENTER.

### RUN [p.72–73]
- `RUN` — Execute program from beginning
- `RUN <expression>` — Execute from specified line number
- `RUN "label"` or `RUN A$` — Execute from labeled line
- Effective in DEF and RUN modes only
- Non-existent line → Error 2

### DEBUG [p.73]
Same forms as RUN. Executes line-by-line, displaying each line number and pausing (break condition). Press ENTER to advance to next line. Effective in DEF and RUN modes.

### CONT [p.73]
`CONT` — Resumes program from:
1. STOP statement pause
2. CA/BREAK key press during execution
3. DEBUG line-by-line pause
4. After PRINT statement display

### LIST [p.74–75]
- `LIST` — Display head (first) line of program
- `LIST <expression>` — Display specified line (1–999)
- `LIST "label"` or `LIST A$` — Display labeled line
- Effective in PRO mode only
- Non-existent line/label → Error 2
- Use ↑/↓ keys to navigate to adjacent lines

### NEW [p.75]
`NEW` — In DEF/RUN/PRO: clears program and data memory. In RESERVE: clears reserve memory.

### MEM [p.75]
`MEM` — Displays remaining program steps and available flexible memories.
- Example: `1424STEPS 178MEMORIES` (fully empty machine)

---

## 10. Error Codes [p.77]

| Code | Nature | Description |
|------|--------|-------------|
| 1 | Grammatical / Operational / Memory type error | Absolute value of result exceeds 10^100; division by zero; numeric variable used as character or vice versa |
| 2 | Line error | Line or label specified by GOTO, GOSUB, RUN, DEBUG, or LIST does not exist |
| 3 | Depth error | GOSUB or FOR-NEXT depth exceeds 4; RETURN without GOSUB; NEXT without FOR |
| 4 | Step-over error | Program too large for remaining memory; reserve program too large; dimension memory index out of range |
| 5 | Cassette control error | Verify error, checksum error during tape operations |
| 6 | Format error | Numeric data cannot be displayed in specified USING format |

**Error display**:
- Manual calculation: shows error code
- Program calculation: shows error code and line number
- Pressing CA key in error state: shows expression with cursor at error position

---

## 11. Logical Operators [p.22]

Logical operators return 1 (true) or 0 (false):

| Operator | True when |
|----------|-----------|
| `=` | x equals y |
| `>` | x greater than y |
| `<` | x less than y |
| `>=` | x greater or equal y |
| `<=` | x less or equal y |
| `<>` | x not equal y |

**Logical OR**: `(A<0) + (A>8)` — true if A<0 or A>8 (result can be 2 if both true)

**Logical AND**: `(B>1) * (B<6)` — true if 1<B<6

**Note**: `A = <expr>` is always treated as assignment, not comparison. Use `<expr> = A` for comparison. (This restriction does not apply inside IF statements.)

---

## 12. Cassette Tape Operations [p.78–83]

The PC-1211 uses an optional cassette interface (CE-121) to save/load programs and data to tape.

### 12.1 CSAVE [p.78]

`CSAVE "filename"` — Records programs or reserve programs to tape (manual operation only).

- In DEF/RUN/PRO modes: records file name then entire program memory
- In RESERVE mode: records file name then all reserve programs
- File name: max 7 characters (excess discarded)
- If no program loaded: prompt appears immediately
- **Warning**: Do not record different programs with same file name on same tape side

### 12.2 CLOAD [p.79]

`CLOAD "filename"` — Transfers programs from tape to computer (manual operation only).

- Automatically searches for matching file name on tape
- In DEF/RUN/PRO: loads to program memory
- In RESERVE: loads to reserve memory
- **Caution**: Computer cannot distinguish program vs. reserve program by file name alone — wrong mode = wrong transfer
- If file name not found, search continues past end of tape (use CA/BREAK to cancel)
- Error during transfer → program memory becomes invalid

### 12.3 CLOAD? [p.79–80]

`CLOAD? "filename"` — Verifies tape contents match computer memory (manual operation only).

- Compares program/reserve memory with tape recording
- Disagreement → Error 5
- Empty memory: prompt appears immediately

### 12.4 CHAIN [p.80–82]

Program execution from tape. Can be used **in programs** (not just manual operation).

**General form (1)**: `CHAIN "filename"`
- Loads program from tape, executes from beginning
- Enables programs larger than memory by chaining segments

**General form (2)**: `CHAIN "filename", <expression>`
- Loads program, starts execution at specified line number (1–999, integer part)

**General form (3)**: `CHAIN "filename", "label"` or `CHAIN "filename", A$`
- Loads program, starts at labeled line (max 7 chars)

**Key behavior**: Data memories are preserved across CHAIN calls — programs can pass data through variables.

### 12.5 PRINT# [p.82]

`PRINT# "filename"` — Records data memory contents to tape.

**General form (1)**: `PRINT# "filename"` — records all data memories starting from A/A$

**General form (2)**: `PRINT# "filename"; A(n)` — records from specified memory onward
- Label must be A–Z or A(integer) where integer is 1–204 or a fixed memory
- Executable by program or manually (manual requires ENTER at end)

### 12.6 INPUT# [p.83]

`INPUT# "filename"` — Loads data from tape into data memory.

**General form (1)**: `INPUT# "filename"` — loads into memory starting from A/A$

**General form (2)**: `INPUT# "filename"; A(n)` — loads from specified memory onward

- If fewer data items on tape than memories: transfer ends when tape data exhausted
- If more data items on tape: transfer ends when all target memories filled
- Computer can distinguish program file names from data file names even if identical

---

## 13. Complete Abbreviation Table [p.98–102]

| Statement | Abbreviations (all valid) |
|-----------|--------------------------|
| LET | LE. |
| INPUT | I. IN. INP. INPU. |
| PRINT | P. PR. PRI. PRIN. |
| PAUSE | PA. PAU. PAUS. |
| USING | U. US. USI. USIN. |
| GOTO | G. GO. GOT. |
| IF | — |
| THEN | T. TH. THE. |
| GOSUB | GOS. GOSU. |
| RETURN | RE. RET. RETU. RETUR. |
| FOR | F. FO. |
| STEP | STE. |
| NEXT | N. NE. NEX. |
| STOP | S. ST. STO. |
| END | E. EN. |
| BEEP | B. BE. BEE. |
| CLEAR | CL. CLE. CLEA. |
| DEGREE | DEG. DEGR. DEGRE. |
| RADIAN | RA. RAD. RADI. RADIA. |
| GRAD | GR. GRA. |
| AREAD | A. AR. ARE. AREA. |
| REM | — |
| RUN | R. RU. |
| DEBUG | D. DE. DEB. DEBU. |
| CONT | C. CO. CON. |
| LIST | L. LI. LIS. |
| NEW | — |
| MEM | M. ME. |
| CSAVE | CS. CSA. CSAV. |
| CLOAD | CLO. CLOA. |
| CLOAD? | CLO.? CLOA.? |
| CHAIN | CH. CHA. CHAI. |
| PRINT# | P.# PR.# PRI.# PRIN.# |
| INPUT# | I.# IN.# INP.# INPU.# |

### Function Abbreviations

| Function | Abbreviation |
|----------|-------------|
| SIN | SI. |
| COS | — |
| TAN | TA. |
| ASN | AS. |
| ACS | AC. |
| ATN | AT. |
| LN | — |
| LOG | LO. |
| EXP | EX. |
| √ | — |
| DMS | DM. |
| DEG | — |
| INT | — |
| ABS | AB. |
| SGN | SG. |
