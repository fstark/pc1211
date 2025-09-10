# PC-1211 BASIC Label System Documentation

## Overview

The PC-1211 BASIC interpreter supports a comprehensive label system that enhances program organization and enables dynamic flow control through string-based labels.

## Features

### 1. String Literal Labels
Lines can start with string literals as labels:

```basic
10 "MAIN" PRINT "Hello World"
20 "LOOP" A=A+1
30 "EXIT" END
```

### 2. GOTO/GOSUB with Labels
Jump to labels using GOTO and GOSUB:

```basic
10 GOTO "MAIN"          ; Jump to string literal label
20 GOSUB "SUBROUTINE"   ; Call subroutine with string literal label
```

### 3. Computed Labels (String Variables)
Use string variables as dynamic label targets:

```basic
10 TARGET$="FINISH"
20 PRINT "Jumping to:", TARGET$
30 GOTO TARGET$         ; Jump to label stored in variable
40 "FINISH" PRINT "Arrived!"
```

### 4. Expression Support
Traditional expression-based GOTO/GOSUB still works:

```basic
10 A=50
20 GOTO A*2             ; Jumps to line 100
30 GOSUB B+200          ; Calls subroutine at calculated line
```

### 5. Mixed Usage
Programs can freely mix line numbers and labels:

```basic
10 GOTO 100             ; Traditional line number
20 GOTO "PROCESS"       ; String literal label
30 A$="END": GOTO A$    ; Computed label
100 "PROCESS" PRINT "Processing..."
200 "END" STOP
```

## Label Rules

### Syntax
- Labels must be **string literals** at the **start of a line**
- Format: `"LABELNAME" statement`
- Labels are **case-insensitive** (`"main"` = `"MAIN"` = `"Main"`)

### Limitations
- Maximum **7 characters** (PC-1211 string limit)
- Labels longer than 7 characters are automatically **truncated**
- Labels cannot contain special characters beyond PC-1211 string support

### Behavior
- **Runtime resolution**: Labels are mapped to line numbers when GOTO/GOSUB executes
- **Linear search**: Label lookup uses O(n) search through program
- **Dynamic updates**: Label table rebuilt when program is loaded/modified

## Error Handling

### Error 2: Bad Line Number
Occurs when:
- GOTO/GOSUB targets a non-existent label
- Computed label resolves to undefined label name

```basic
10 GOTO "MISSING"       ; Error: label doesn't exist
20 A$="NONE": GOTO A$   ; Error: computed label not found
```

### Error 5: Type Mismatch  
Occurs when:
- String variable used in computed GOTO/GOSUB contains non-string data
- Variable not properly initialized as string

```basic
10 A=123                ; A is numeric
20 GOTO A               ; OK: numeric expression
30 GOTO A$              ; Error: A$ is not a string variable
```

## Implementation Details

### Label Table
- **Capacity**: 100 labels maximum per program
- **Structure**: Label name → line number mapping
- **Memory**: Automatic cleanup on program load
- **Thread Safety**: Single-threaded access assumed

### Token Support
- **T_STR**: String literal labels and GOTO/GOSUB targets
- **T_SVAR**: String variable references for computed labels
- **Token Parsing**: Handles mixed expression/label syntax

### Performance
- **Label Definition**: O(1) insertion during program parsing
- **Label Lookup**: O(n) linear search through label table
- **Memory Overhead**: ~24 bytes per label (name + line number)

## Usage Examples

### Basic Label Program
```basic
10 "START" PRINT "Program beginning"
20 GOTO "MAIN"
30 PRINT "This won't print"
100 "MAIN" PRINT "Main routine"
110 GOSUB "HELPER"
120 GOTO "END"
200 "HELPER" PRINT "Helper routine": RETURN
300 "END" PRINT "Program finished"
```

### Dynamic Label Selection
```basic
10 INPUT "Enter mode (1-3):", MODE
20 IF MODE=1 THEN TARGET$="MODE1"
30 IF MODE=2 THEN TARGET$="MODE2"  
40 IF MODE=3 THEN TARGET$="MODE3"
50 GOTO TARGET$
100 "MODE1" PRINT "Mode 1 selected": GOTO "EXIT"
200 "MODE2" PRINT "Mode 2 selected": GOTO "EXIT"
300 "MODE3" PRINT "Mode 3 selected": GOTO "EXIT"
400 "EXIT" END
```

### Subroutine Table
```basic
10 DIM SUB$(3): SUB$(1)="INIT": SUB$(2)="PROC": SUB$(3)="CLEAN"
20 FOR I=1 TO 3
30   PRINT "Calling subroutine:", SUB$(I)
40   GOSUB SUB$(I) 
50 NEXT I
60 END
100 "INIT" PRINT "Initializing...": RETURN
200 "PROC" PRINT "Processing...": RETURN
300 "CLEAN" PRINT "Cleaning up...": RETURN
```

## Testing

All label functionality is verified through comprehensive test suites:

- **test_labels.bas**: Basic label and GOTO/GOSUB functionality
- **test_computed_goto.bas**: String variable label targets  
- **test_computed_gosub.bas**: String variable subroutine calls
- **test_comprehensive_goto.bas**: Mixed usage scenarios
- Error condition testing for undefined labels and type mismatches

## Compatibility

This label system is a **PC-1211 enhancement** that extends the original specification while maintaining full backward compatibility with traditional line number-based GOTO/GOSUB statements.
