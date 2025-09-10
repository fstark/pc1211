| Instruction | Abbreviation | Usability in RUN and DEF modes | Programmability | Example                             | Note                                                                                                                                         |
|-------------|--------------|--------------------------------|-----------------|-------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------|
| =           |              | Yes                            | Yes             | A=10                                |                                                                                                                                              |
| +           |              | Yes                            | Yes             | A=B+C                               |                                                                                                                                              |
| −           |              | Yes                            | Yes             | A=B−C                               |                                                                                                                                              |
| *           |              | Yes                            | Yes             | A=B＊C                              |                                                                                                                                              |
| /           |              | Yes                            | Yes             | A=B/C                               |                                                                                                                                              |
| ∧           |              | Yes                            | Yes             | A=B∧C                               |                                                                                                                                              |
| ( )         |              | Yes                            | Yes             | A=(B+C)/D                           |                                                                                                                                              |
| = (IF)      |              | Yes                            | Yes             | IF A=B                              |                                                                                                                                              |
| > (IF)      |              | Yes                            | Yes             | IF A＞B                             |                                                                                                                                              |
| >= (IF)     |              | Yes                            | Yes             | IF A≧B                              |                                                                                                                                              |
| < (IF)      |              | Yes                            | Yes             | IF A＜B                             |                                                                                                                                              |
| <= (IF)     |              | Yes                            | Yes             | IF A≦B                              |                                                                                                                                              |
| <> (IF)     |              | Yes                            | Yes             | IF A<>B                             | ≠ (Not equal)                                                                                                                                |
| SIN         | SI.          | Yes                            | Yes             | A=SIN B                             |                                                                                                                                              |
| COS         |              | Yes                            | Yes             | A=COS B                             |                                                                                                                                              |
| TAN         | TA.          | Yes                            | Yes             | A=TAN B                             |                                                                                                                                              |
| ASN         | AS.          | Yes                            | Yes             | A=ASN B                             |                                                                                                                                              |
| ACS         | AC.          | Yes                            | Yes             | A=ACS B                             |                                                                                                                                              |
| ATN         | AT.          | Yes                            | Yes             | A=ATN B                             |                                                                                                                                              |
| LOG         | LO.          | Yes                            | Yes             | A=LOG B                             | Common logarithm                                                                                                                             |
| LN          |              | Yes                            | Yes             | A=LN B                              | Natural logarithm                                                                                                                            |
| EXP         | EX.          | Yes                            | Yes             | A=EXP B                             | A=e^B                                                                                                                                       |
| √           |              | Yes                            | Yes             | A=√                                 |                                                                                                                                              |
| DMS         | DM.          | Yes                            | Yes             | A=DMS B                             | Conversion to sexagesimal notation                                                                                                           |
| DEG         |              | Yes                            | Yes             | A=DEG B                             | Conversion to decimal notation                                                                                                               |
| INT         |              | Yes                            | Yes             | A=INT B                             | Obtains integer within a range does not exceed integer portion of B.                                                                         |
| ABS         | AB.          | Yes                            | Yes             | A=ABS B                             | Obtains the absolute value A = |B|.                                                                                                         |
| SGN         | SG.          | Yes                            | Yes             | A=SGN B                             | If B＞0, A=1 B=0, A=0 B＜0, A=−1                                                                                                             |
| AREAD       | A.           | No                             | Yes             | AREAD A                             | Display (shown before execution) is read into A Only when used in the first line of the executing program in the DEF mode).                  |
| BEEP        | B.           | No                             | Yes             | BEEP A                              | Sound buzzer as many times as A.                                                                                                             |
| CLEAR       | CL.          | Yes                            | Yes             | CLEAR                                | Clears all data variables.                                                                                                                   |
| DEGREE      | DEG.         | Yes                            | Yes             | DEGREE                               | Sets the angle mode to DEG (decimal notation).                                                                                               |
| END         | E.           | No                             | Yes             | END                                  | Terminates the program execution.                                                                                                            |
| FOR         | F.           | No                             | Yes             | 10 FOR A=0 TO 10 STEP 2<br>100 NEXT A | Increments by 2 from A=0 to A=10, during which time program lines up to NEXT A are repeated. Repeats the lines 10 through 100 for 6 times as A=0 advances to 2, 4 …… 10. |
| GOTO        | G.           | No                             | Yes             | 10 GOTO A*10+50<br>20 GOTO "LABEL"<br>30 GOTO A$ | Jumps to line number from expression, string literal label, or computed label from string variable. Supports expressions, string literals, and string variables as targets. |
| GOSUB       | GOS.         | No                             | Yes             | 10 GOSUB B+200<br>20 GOSUB "SUB1"<br>30 GOSUB B$ | Jumps to subroutine at line from expression, string literal label, or computed label from string variable. Same target types as GOTO but pushes return address. |
| GRAD        |              | Yes                            | Yes             | GRAD                                 | Sets the angle mode to GRAD.                                                                                                                 |
| IF          |              | No                             | Yes             | 10 IF A=B                            | Decision instruction, successive statement is executed when the IF statement is statisfied, or executes the next line when the IF statement is not statisfied. |
| INPUT       | I.           | No                             | Yes             | INPUT A                              | Data input through the keyboard                                                                                                              |
| LET         | LE.          | No                             | Yes             | LET A=10<br>LET A$="TRS-80"         | Substitute instruction (can be omitted except immediately after the IF statement).                                                            |
| NEXT        | N.           | No                             | Yes             | NEXT A                               | Used with FOR (see FOR).                                                                                                                     |
| PAUSE       | PA.          | No                             | Yes             | PAUSE A                              | Holds the display for a period of 0.85 second.                                                                                                |
| PRINT       | P.           | No                             | Yes             | PRINT A<br>PRINT A,B<br>PRINT A,B,C | Display A. Displays A and B at left and right Displays A, B and C in succession from the left.                                               |
| RADIAN      | RA.          | Yes                            | Yes             | RADIAN                               | Set the angle mode to RAD (radian).                                                                                                          |
| REM         |              | No                             | Yes             | REM "INTEREST"                       | A comment statement (is not executed)                                                                                                        |
| RETURN      | RE.          | No                             | Yes             | RETURN                               | End of subroutine. The program execution returns to execute the statement next to the GOSUB instruction                                      |
| STEP        | STE.         | No                             | Yes             |                                       | See FOR.                                                                                                                                     |
| STOP        | S.           | No                             | Yes             | STOP                                  | Suspends program execution.                                                                                                                  |
| THEN        | T.           | No.                            | Yes             | IF ... THEN 60                       | Written after the IF instruction to indicate jump line number.                                                                               |
| USING       | U.           | No                             | Yes             | PRINT USING " ### . ##"; A           | Designates the format in relation with PRINT instruction. In this example, A is designated with 3 digits of integer and 2 digits after the decimal point. |

## Labels

PC-1211 BASIC supports string labels for enhanced program organization and dynamic flow control:

### String Literal Labels
Lines can start with string literals as labels:
```basic
10 "MAIN" PRINT "Hello World"
20 "LOOP" A=A+1: IF A<10 GOTO "LOOP"  
30 "EXIT" END
```

### Label Usage with GOTO/GOSUB
- **String Literals**: `GOTO "MAIN"`, `GOSUB "SUBROUTINE"`
- **String Variables**: `A$="EXIT": GOTO A$`, `B$="SUB1": GOSUB B$`
- **Expressions**: `GOTO A*10+50` (traditional line number calculation)

### Label Rules
- Labels are **case-insensitive** (`"main"` = `"MAIN"` = `"Main"`)
- Maximum **7 characters** (PC-1211 string limit)
- Labels exceeding 7 characters are automatically truncated
- Labels and line numbers can be **mixed freely** in the same program
- Label existence is checked at **runtime**, not compile time

### Examples
```basic
10 A$="FINISH"          ; Set target label
20 PRINT "Starting..."
30 GOTO "PROCESS"       ; Jump to string literal label
40 PRINT "Never reached"

100 "PROCESS" PRINT "Processing..."
110 GOSUB A$           ; Jump to computed label
120 END

200 "FINISH" PRINT "Done!": RETURN
```
