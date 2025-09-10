# Phase 6: Math Functions Specification

## Function Specifications

### Trigonometric Functions (Radians)
- **SIN(x)** - Sine function, standard libc `sin(x)`
- **COS(x)** - Cosine function, standard libc `cos(x)`  
- **TAN(x)** - Tangent function, standard libc `tan(x)`

### Inverse Trigonometric Functions
- **ASN(x)** - Arcsine, standard libc `asin(x)`, domain [-1, 1]
- **ACS(x)** - Arccosine, standard libc `acos(x)`, domain [-1, 1] 
- **ATN(x)** - Arctangent, standard libc `atan(x)`, all real numbers

### Logarithmic and Exponential Functions
- **LOG(x)** - Base 10 logarithm, standard libc `log10(x)`, domain x > 0
- **LN(x)** - Natural logarithm, standard libc `log(x)`, domain x > 0
- **EXP(x)** - Exponential function, standard libc `exp(x)`, all real numbers

### Other Math Functions
- **SQR(x)** - Square root, standard libc `sqrt(x)`, domain x ≥ 0
- **ABS(x)** - Absolute value, standard libc `fabs(x)`, all real numbers
- **INT(x)** - Floor function, standard libc `floor(x)`
  - Examples: `INT(3.7) = 3`, `INT(-3.7) = -4`, `INT(0.0) = 0`
- **SGN(x)** - Sign function
  - Returns: -1 if x < 0, 0 if x = 0, +1 if x > 0

### Angle Conversion Functions
- **DMS(x)** - Decimal degrees to Degrees.Minutes.Seconds format
  - Input: decimal degrees (e.g., 15.4125)
  - Output: DD.MMSS format (e.g., 15.2445)
  - Format: integer part = degrees, digits 1-2 = minutes (00-59), digits 3-4 = seconds (00-59), digits 5+ = fractional seconds
  - Conversion: 0.4125° × 60 = 24.75' = 24' + 0.75' = 24' + 45" = 15° 24' 45" = 15.2445

- **DEG(x)** - Degrees.Minutes.Seconds format to decimal degrees
  - Input: DD.MMSS format (e.g., 15.2445)
  - Output: decimal degrees (e.g., 15.4125)
  - Reverse conversion of DMS

## Domain Error Handling

All functions use standard libc behavior for domain errors:
- **Domain violations** generate `ERR_MATH_DOMAIN` (error code 2)
- **Overflow/underflow** may generate `ERR_MATH_OVERFLOW` (error code 3)

Examples of domain errors:
- `SQR(-1)` - square root of negative number
- `LOG(0)`, `LOG(-1)` - logarithm of non-positive number  
- `LN(0)`, `LN(-1)` - natural log of non-positive number
- `ASN(2)`, `ASN(-2)` - arcsine outside [-1, 1] range
- `ACS(2)`, `ACS(-2)` - arccosine outside [-1, 1] range

## Implementation Notes

- All functions work in **radians** for trigonometric operations
- Use standard C math library (`math.h`) functions where possible
- Check for `isfinite()` results to detect overflow conditions
- Use `errno` or return value checks for domain error detection
- Functions are called within expressions: `PRINT SIN(3.14159/2)`

## Test Cases

### Basic Function Tests
```basic
10 PRINT "SIN(0)=", SIN(0)                    REM Should be 0
20 PRINT "COS(0)=", COS(0)                    REM Should be 1  
30 PRINT "INT(-3.7)=", INT(-3.7)              REM Should be -4
40 PRINT "SGN(-5)=", SGN(-5)                  REM Should be -1
50 PRINT "SGN(0)=", SGN(0)                    REM Should be 0
60 PRINT "SGN(5)=", SGN(5)                    REM Should be 1
```

### DMS/DEG Conversion Tests
```basic
70 PRINT "DMS(15.4125)=", DMS(15.4125)        REM Should be 15.2445
80 PRINT "DEG(15.2445)=", DEG(15.2445)        REM Should be 15.4125
90 PRINT "Round trip:", DEG(DMS(30.75))       REM Should be 30.75
```

### Domain Error Tests
```basic  
100 PRINT SQR(-1)    REM Should generate ERR_MATH_DOMAIN
110 PRINT LOG(0)     REM Should generate ERR_MATH_DOMAIN
120 PRINT ASN(2)     REM Should generate ERR_MATH_DOMAIN
```
