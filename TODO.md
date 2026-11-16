# PC-1211 BASIC — TODO

## Cannot implement (need hardware/modes we don't have)

- **AREAD** — only works as first statement of a DEF-mode program
- **CONT** — requires interactive command loop to resume after STOP
- **DEBUG** — single-step execution, interactive ENTER after each line
- **MEM** — reports remaining program steps / flexible memories
- **Reserve programs** — requires RESERVE mode and labeled-key dispatch
- **Cassette I/O** — CSAVE/CLOAD, no tape hardware
- **24-char LCD layout** — real display right-justification, field splitting, cursor

## Not yet implemented

- **USING** formatting — parse `"###.##"` format strings, apply to PRINT/PAUSE output
- **INPUT prompt strings** — `INPUT "msg", A` and `INPUT "msg"; A`
- **INPUT multiple variables** — `INPUT A, B, C` (currently single var only)
- **A$ = B$** string copy — `execute_svar_assign` only accepts literals, not string variables
- **Functions without parens** — `SIN 30`, `SIN A` (VM requires `(` after every function)
- **Compound functions** — `LN ABS A` = `LN(ABS(A))`, right-to-left chaining
- **Implicit multiplication** — `2A` = `2*A`, neither tokenizer nor evaluator handles it
- **π constant** — no token, not recognized
- **RANDOM** — seed RNG, no token exists
- **INPUT skip** — ENTER without data should skip rest of line
- **STOP ≠ END** — STOP should save position for CONT (currently identical to END)
- **RUN with argument** — `RUN 50`, `RUN "label"` (interactive RUN takes no args)

## Implemented but incomplete

- **String variable assignment** — `A$="HELLO"` works, `A$=B$` gives syntax error
- **LET with strings** — `LET A$="X"` works, `LET A$=B$` fails (same root cause)
- **PRINT comma vs semicolon** — both emit a space; comma should do 2-field 12-char layout
- **USING** — tokenizer recognizes it, `execute_using()` is a no-op
- **FOR TO/STEP** — accepts any float; manual says integer part only, < 3 digits
- **RETURN** — doesn't enforce being last statement on line
- **GOTO** — doesn't enforce being last statement on line (works by accident since PC jumps)
