# Phase 7: Mode & Device Statements Specification

## Angle Mode System

### Mode Commands
- **DEGREE** - Set angle mode to degrees
- **RADIAN** - Set angle mode to radians (default)
- **GRAD** - Set angle mode to gradians

### Angle Conversions
- **90 degrees = π/2 radians = 100 gradians**
- **360 degrees = 2π radians = 400 gradians**

### Conversion Formulas
```c
// To radians for internal calculation:
if (mode == DEGREE) angle_rad = angle * (M_PI / 180.0);
if (mode == GRAD)   angle_rad = angle * (M_PI / 200.0);
if (mode == RADIAN) angle_rad = angle;

// From radians for result:
if (mode == DEGREE) result = result_rad * (180.0 / M_PI);
if (mode == GRAD)   result = result_rad * (200.0 / M_PI);
if (mode == RADIAN) result = result_rad;
```

### Affected Functions
**The 6 trigonometric functions are affected:**
- **SIN(x), COS(x), TAN(x)** - input angle converted from current mode to radians
- **ASN(x), ACS(x), ATN(x)** - output angle converted from radians to current mode

**NOT affected:**
- **DMS(x), DEG(x)** - always work in degrees regardless of angle mode
- **EXP, LOG, LN, SQR, ABS, INT, SGN** - non-trigonometric functions

### Test Examples
```basic
DEGREE
PRINT COS(90)     REM Should print 0 (90° = π/2 rad)
PRINT SIN(90)     REM Should print 1
GRAD  
PRINT COS(100)    REM Should print 0 (100 grad = π/2 rad)
RADIAN
PRINT COS(1.5708) REM Should print 0 (π/2 rad)
```

## Memory Management

### CLEAR Statement
- **Syntax**: `CLEAR` (no parameters)
- **Behavior**: 
  - Set all numeric variables A-Z to 0
  - Set all indexed variables A(1) through A(VARS_MAX) to 0
  - Clear all string variables A$-Z$ (they become numeric 0)
  - Does NOT change angle mode (DEGREE/RADIAN/GRAD)
  - Does NOT affect program code

### Memory Reset Example
```basic
A = 5
B$ = "HELLO"
A(10) = 99
DEGREE
CLEAR
REM Now: A=0, B=0 (no longer string), A(10)=0
REM But angle mode is still DEGREE
```

## Device Commands

### BEEP Statement
- **Syntax**: `BEEP` (no parameters)
- **Behavior**: Emit ASCII bell character (^G, ASCII 7)
- **Implementation**: `putchar('\a')` or `printf("\a")`

### PAUSE Statement  
- **Syntax**: `PAUSE [expression_list]` (same as PRINT)
- **Behavior**: 
  - Print expressions exactly like PRINT statement
  - Wait 100ms after displaying
  - Supports same format: `PAUSE "Value:", A, "Result:", B`
- **Implementation**: Execute PRINT logic, then `usleep(100000)` or equivalent

### AREAD Statement ✅ IMPLEMENTED  
- **Syntax**: `AREAD variable` (statement, not expression)
- **Variable types**: 
  - `AREAD A` - numeric variable
  - `AREAD B$` - string variable  
  - `AREAD A(5)` - indexed numeric variable
  - `AREAD B$(3)` - indexed string variable
- **Command line options**:
  - `--aread-value N` - set numeric value
  - `--aread-string S` - set string value
- **Behavior**: 
  - Reads previous "screen value" into variable
  - Type conversion: string→numeric, numeric→string as needed
  - Consumed on first use (subsequent AREAD gets 0/"")
  - PRINT/PAUSE statements clear AREAD
- **Implementation**: Global variables with type flag for string/numeric

### USING Statement
- **Syntax**: `USING` (no-op for now)
- **Behavior**: No operation, just ignore
- **Future**: Format output specifications

## Implementation Architecture

### Global State
Add to vm.h:
```c
typedef enum {
    ANGLE_RADIAN = 0,  // Default
    ANGLE_DEGREE = 1,
    ANGLE_GRAD = 2
} AngleMode;

extern AngleMode g_angle_mode;
extern char g_aread_string[8];   // AREAD string value (up to 7 chars + null)
extern double g_aread_value;     // AREAD numeric value  
extern bool g_aread_is_string;   // Whether AREAD value is a string
```

### Trigonometric Function Wrapper
```c
double convert_angle_to_radians(double angle) {
    switch (g_angle_mode) {
        case ANGLE_DEGREE: return angle * (M_PI / 180.0);
        case ANGLE_GRAD:   return angle * (M_PI / 200.0);
        case ANGLE_RADIAN: return angle;
    }
}

double convert_angle_from_radians(double radians) {
    switch (g_angle_mode) {
        case ANGLE_DEGREE: return radians * (180.0 / M_PI);
        case ANGLE_GRAD:   return radians * (200.0 / M_PI);
        case ANGLE_RADIAN: return radians;
    }
}
```

### Statement Handlers
Add to vm.c switch statement:
```c
case T_DEGREE: g_angle_mode = ANGLE_DEGREE; break;
case T_RADIAN: g_angle_mode = ANGLE_RADIAN; break;
case T_GRAD:   g_angle_mode = ANGLE_GRAD; break;
case T_CLEAR:  /* Clear all variables */ break;
case T_BEEP:   putchar('\a'); fflush(stdout); break;
case T_PAUSE:  /* Like PRINT + 100ms delay */ break;
case T_USING:  /* No-op */ break;
```

## Test Cases

### Angle Mode Tests
```basic
10 REM Test angle modes
20 RADIAN
30 PRINT "RADIAN: COS(", 3.14159/2, ")=", COS(3.14159/2)
40 DEGREE  
50 PRINT "DEGREE: COS(90)=", COS(90)
60 GRAD
70 PRINT "GRAD: COS(100)=", COS(100)
```

### CLEAR Test
```basic
10 A = 42
20 B$ = "TEST"
30 A(5) = 99
40 PRINT "Before CLEAR: A=", A, " B$=", B$, " A(5)=", A(5)
50 CLEAR
60 PRINT "After CLEAR: A=", A, " B=", B, " A(5)=", A(5)
```

### Device Commands Test ✅ IMPLEMENTED
```basic
10 AREAD X           REM Read command-line value first!
20 PRINT "Initial:", X
30 BEEP
40 PAUSE "Waiting...", 3, " seconds" 
50 AREAD Y           REM This gets 0 (cleared by PAUSE)
60 PRINT "After PAUSE:", Y
70 USING             REM This is ignored
```

## Command Line Enhancement ✅ IMPLEMENTED
Support for both numeric and string AREAD input:
```bash
# Numeric input
./pc1211 program.bas --run --aread-value 42.5

# String input  
./pc1211 program.bas --run --aread-string "HELLO"
```

## Implementation Status: ✅ COMPLETE
This specification has been fully implemented with all features working:
- ✅ Angle mode system (DEGREE/RADIAN/GRAD)
- ✅ CLEAR statement (clears all variables)
- ✅ AREAD statement (supports both string and numeric input)  
- ✅ Device commands (BEEP, PAUSE, USING)
- ✅ Command-line options for AREAD values
