# PC-1211 BASIC --- Token Interpreter (static build, v0.5)

## Quick summary of changes

-   Integrated your **instruction list** (names + abbreviations).
-   Assigned **compact opcodes** (1 byte each) for operators, functions,
    and statements.
-   Marked **runtime behavior** per our constraints (what runs now,
    what's a no-op, what's postponed).
-   Kept the **static memory** design (no `malloc`).

------------------------------------------------------------------------

## 1) Static memory (unchanged)

``` c
enum {
  PROG_MAX_BYTES   = 16384,
  LINES_MAX        = 1024,
  VARS_MAX         = 512,    // A(n) range: 1..VARS_MAX (A..Z = 1..26)
  STR_MAX          = 7,
  GOSUB_MAX        = 32,
  FOR_MAX          = 64,
  EXPR_VAL_MAX     = 64,
  TOKBUF_LINE_MAX  = 256
};
```

Static globals: `prog[]/prog_len`, optional `line_index[]`, `V[]`
(tagged union {double \| char\[8\]}), `gosub_stack[]`, `for_stack[]`,
expression stacks. No dynamic allocation anywhere.

------------------------------------------------------------------------

## 2) Token stream format (unchanged)

    u16 LEN | u16 LINE | TOKENS... | T_EOL (0x00)

-   **PC** is a byte offset to the next token.
-   **Find line** by **linear scan** (or static `line_index` if you
    enable it).

------------------------------------------------------------------------

## 3) Variables and types (unchanged, PC-1211 model)

-   1-based cells `V[1..]`: `1..26` map to `A..Z`; `27+` only via
    `A(n)`.
-   A cell is either **NUM** (double) or **STR\[≤7\]**; assigning one
    overwrites the other.
-   No `A$` namespace---the **LET `A$`** example from the card is
    **ignored** in this re-implementation (we keep your "single shared
    cell" rule).

------------------------------------------------------------------------

## 4) Opcode table (operators, functions, statements)

I kept operator/function mnemonics as on the card and noted
abbreviations. All opcodes are **one byte**; immediates follow the
opcode as noted.

### 4.1 Core tokens

    00  T_EOL           (end of line record)
    01  T_NUM <8 bytes double>
    02  T_STR <u8 n> <n bytes>          // 0..7, stored as-is
    03  T_VAR <u8 1..26>                 // A..Z
    04  T_VIDX <expr tokens> T_ENDX      // A(expr); expr→int (truncate toward 0), require >=1
    FF  T_ENDX          (terminator for T_VIDX inline expression)

### 4.2 Operators & punctuation (usable in RUN/PRO)

    10  T_EQ_ASSIGN     (= assignment)
    11  T_PLUS          (+)
    12  T_MINUS         (−)
    13  T_MUL           (*)
    14  T_DIV           (/)
    15  T_POW           (∧)

    16  T_LP            (()
    17  T_RP            ())
    18  T_COMMA         (,)
    19  T_SEMI          (;)
    1A  T_COLON         (:)

    1B  T_EQ            (= for IF)
    1C  T_NE            (<>)
    1D  T_LT            (<)
    1E  T_LE            (<=)
    1F  T_GT            (>)
    20  T_GE            (>=)

### 4.3 Functions (all **numeric**; radians for trig)

  Opcode   Name    Abb.    Status
  -------- ------- ------- ---------------------------------------
  30       `SIN`   `SI.`   ✔ implemented (angle mode note below)
  31       `COS`   ---     ✔
  32       `TAN`   `TA.`   ✔
  33       `ASN`   `AS.`   ✔ (asin)
  34       `ACS`   `AC.`   ✔ (acos)
  35       `ATN`   `AT.`   ✔ (atan)
  36       `LOG`   `LO.`   ✔ (base-10)
  37       `LN`    ---     ✔ (natural)
  38       `EXP`   `EX.`   ✔
  39       `SQR`   ---     ✔ (square root; card shows "√ B")
  3A       `DMS`   `DM.`   ✔ (decimal→sexagesimal: 12.5→12°30′0)
  3B       `DEG`   ---     ✔ (sexagesimal→decimal)
  3C       `INT`   ---     ✔ (truncate toward 0)
  3D       `ABS`   `AB.`   ✔
  3E       `SGN`   `SG.`   ✔ (−1,0,1)

> Notes\
> • `DMS/DEG` here are **conversion functions** (as per card text), not
> the mode switches.\
> • Trig runs in **radians** by default; see "angle mode commands" below
> if you later want mode switches.

### 4.4 Statements

  ----------------------------------------------------------------------------------------------------------
  Opcode        Statement                  Abb. Implemented   Notes
                                                now?          
  ------------- ------------- ----------------- ------------- ----------------------------------------------
  40            `LET`                     `LE.` ✔             Optional in source; we still emit `T_LET`.
                                                              Encoding: `LET <target> = <expr>` →
                                                              `T_LET target T_EQ_ASSIGN expr`.

  41            `PRINT`                    `P.` ✔             `,` = 1 space; `;` = no space; trailing
                                                              `,`/`;` suppress newline. No `USING` yet.

  42            `INPUT`                    `I.` ✔ (basic)     Reads `"quoted"` as string (truncate to 7),
                                                              else double. Targets: `VAR`/`VIDX`.

  43            `IF`                        --- ✔             `IF <expr rel expr> THEN <line | one_stmt>`.
                                                              Rel ops use tokens above.

  44            `THEN`                     `T.` (part of IF)  Only after `IF`. For line jumps we store a u16
                                                              line after `THEN`.

  45            `GOTO`                     `G.` ✔             Jumps by **linear scan** for `u16 line`.

  46            `GOSUB`                  `GOS.` ✔             Push return PC to static stack; jump by scan.

  47            `RETURN`                  `RE.` ✔             Pops; error if empty.

  48            `FOR`                      `F.` ✔             `FOR v = start TO limit [STEP step]` pushes
                                                              static frame.

  49            `TO`                        --- (part of FOR) Token emitted inside FOR encoding.

  4A            `STEP`                   `STE.` (part of FOR) Optional in FOR.

  4B            `NEXT`                     `N.` ✔             With or without var. Matches top frame or
                                                              searches down.

  4C            `END`                      `E.` ✔             Terminate.

  4D            `STOP`                     `S.` ✔             Terminate (like END).

  4E            `REM`                       --- ✔             Discard rest of line, still close with
                                                              `T_EOL`.
  ----------------------------------------------------------------------------------------------------------

### 4.5 Mode & device-ish commands (accepted, but **not part of core VM**)

  ---------------------------------------------------------------------------
  Opcode            Command           Abb.              Our behavior
  ----------------- ----------------- ----------------- ---------------------
  50                `DEGREE`          `DEG.`            **Stub (no-op)** by
                                                        default. (Optional:
                                                        set `angle_mode=DEG`
                                                        and auto-convert trig
                                                        in/out.)

  51                `RADIAN`          `RA.`             **Stub (no-op)** (or
                                                        set
                                                        `angle_mode=RAD`).

  52                `GRAD`            ---               **Stub (no-op)** (or
                                                        set
                                                        `angle_mode=GRAD`).

  53                `CLEAR`           `CL.`             Optional: **clear all
                                                        cells** (set NUM=0,
                                                        STR=""). Safe to
                                                        implement.

  54                `BEEP`            `B.`              **Ignored** (no
                                                        hardware).

  55                `PAUSE`           `PA.`             Optional: busy-wait
                                                        using a fixed loop
                                                        (still static).
                                                        Default: **ignored**.

  56                `AREAD`           `A.`              **Unsupported**
                                                        (DEF/display
                                                        context). Tokenizer
                                                        can accept and
                                                        ignore.

  57                `USING`           `U.`              **Unsupported in
                                                        v0.5**. (Formatting
                                                        model is non-trivial;
                                                        see "Future".)
  ---------------------------------------------------------------------------

> You can compile without these opcodes at all. I keep them reserved so
> programs that include them can load without crashing; they'll no-op
> (or error) per your preference.

------------------------------------------------------------------------

## 5) Tokenization rules (updates to match the card)

-   Accept both **full names** and **abbreviations** (e.g., `SIN` or
    `SI.`).\
    Implementation tip: keyword table with two spellings per entry,
    matched case-insensitively; abbreviations end with a dot.
-   `√` in the card maps to our `SQR` token; in ASCII source, accept
    `SQR`.
-   `LOG` is **common log** (base-10); `LN` is natural.
-   `DMS`/`DEG` are **functions** (convert number), distinct from
    `DEGREE`/`RADIAN/GRAD` **commands**.
-   `LET` required **immediately after `IF`** if you choose to mirror
    that syntactic quirk; otherwise allow omitted `LET` everywhere
    (simpler).
-   `A(n)`: only `A` supports `(...)` to index cells 1..VARS_MAX. (This
    is your rule; card doesn't contradict.)

------------------------------------------------------------------------

## 6) Execution model (unchanged highlights)

-   **PC is a token pointer**. `:` continues within a line; `T_EOL`
    jumps to next record.
-   **Unstructured flow is allowed** (e.g., `NEXT` at a different line
    still matches the current FOR frame).
-   **Line lookups**: **linear scan** of `prog[]` (optionally aided by
    static `line_index[]`).
-   **Stacks** are **fixed arrays**; on overflow, print error + current
    line and halt.

------------------------------------------------------------------------

## 7) Angle mode (optional)

By default we keep **radians** and treat `DEGREE/RADIAN/GRAD` as
**no-ops**.\
If you *do* want mode:

``` c
typedef enum { ANG_RAD, ANG_DEG, ANG_GRAD } Angle;
static Angle g_ang = ANG_RAD;
```

-   On `DEGREE/RADIAN/GRAD`: set `g_ang`.
-   Before calling `sin/cos/tan`, convert **argument** from mode →
    radians.
-   For `ASN/ACS/ATN`, compute in radians, then convert **result** back
    to mode.

This stays static---no heap---and doesn't affect other parts.

------------------------------------------------------------------------

## 8) PRINT USING (future)

`PRINT USING " ### . ##"; A` implies a small formatting mini-language
(field width, decimal places, padding). For v0.5 we **reject** or
**ignore** `USING`. If you want it later, we can add a **static**
formatter: - Parse the picture string at tokenize time into a small
fixed struct (width, decimals, show sign, pad). - Store it inline after
a `T_USING` token. - At runtime, render with a custom `dtoastr` (still
no heap).

------------------------------------------------------------------------

## 9) Error messages (unchanged)

-   `Syntax error at line N`
-   `Bad line number L at line N`
-   `Type mismatch at line N`
-   `Division by zero at line N`
-   `RETURN without GOSUB` / `NEXT without FOR`
-   `Index out of range at line N` (A(n) \< 1 or \> VARS_MAX)

------------------------------------------------------------------------

## 10) Sanity tests (still valid)

-   Indexed variables & extended cells\
-   Unstructured `NEXT` at a target label/line\
-   String overwrite behavior (truncate to 7)\
-   Trig check (if you enable angle mode, verify DEG/GRAD conversions)

------------------------------------------------------------------------

## 11) Tiny header you can drop in (opcodes)

If you want to lock these now, here's the minimal enum (shortened):

``` c
typedef enum {
  T_EOL=0x00, T_NUM=0x01, T_STR=0x02, T_VAR=0x03, T_VIDX=0x04, T_ENDX=0xFF,

  T_EQ_ASSIGN=0x10, T_PLUS, T_MINUS, T_MUL, T_DIV, T_POW,
  T_LP, T_RP, T_COMMA, T_SEMI, T_COLON,
  T_EQ, T_NE, T_LT, T_LE, T_GT, T_GE,

  T_SIN=0x30, T_COS, T_TAN, T_ASN, T_ACS, T_ATN,
  T_LOG, T_LN, T_EXP, T_SQR, T_DMS, T_DEG, T_INT, T_ABS, T_SGN,

  T_LET=0x40, T_PRINT, T_INPUT, T_IF, T_THEN, T_GOTO, T_GOSUB, T_RETURN,
  T_FOR, T_TO, T_STEP, T_NEXT, T_END, T_STOP, T_REM,

  T_DEGREE=0x50, T_RADIAN, T_GRAD, T_CLEAR, T_BEEP, T_PAUSE, T_AREAD, T_USING
} Tok;
```

> If you prefer different numbers, that's fine---just keep them
> **stable** across tokenizer/VM.
