10 A$(10)="HELLO"
20 PRINT A$(10)
30 A$(A+1)="WORLD"
35 REM This changed the variable A to "WORLD"
36 REM So A+1 will fail in the next line
40 PRINT A$(A+1)
