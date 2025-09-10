## String Comparison Details

### The spec mentions "only =/<> allowed for strings" but doesn't specify the exact comparison rules

Strings are ASCII only, uppercase, max length 7 chars

### Need clarification on string-to-number coercion behavior in mixed comparisons

There is no coertion; mixed comparisons are a type 1 error.

### Error Recovery Strategy

Just stop on any error, display the line number of the interpreter and the error number.

### No details on how to handle malformed token streams

That should never happen, so you can assert false in that case.

### INPUT behavior for invalid input (non-numeric when expecting number)

Great question. The input is parsed as a BASIC expression and evluated. Any error aborts the input.

### PRINT spacing rules could be more precise (especially for mixed numeric/string output)

We don't care about spacing for now.

### No specification for number formatting precision in output

We don't care about this for now (because number implementation will change in a latter iteration)

### Math domain errors (e.g., SQR(-1), LOG(0), ASN(2))

Type 1 errors.

### Overflow/underflow for very large/small numbers

Type 1 error for overflow, round to 0 for underflow.

### Behavior when FOR step is 0

Type 1 error

### The conversion functions DMS and DEG need more precise specifications

Will be provided later. You can implement a stub that asserts false.

### Valid range for line numbers (1-9999? 1-32767?)

1-999

### Behavior when jumping to non-existent line numbers

Type 2 error

### How to handle duplicate line numbers

Latest one entered wins

### Maximum program size validation

Will be handled later. There should be a fixed area for program storage (in the real machine, it is shared with the variables). Size of a real machine is 1424 bytes of program storage shared with 178 variables (8 bytes each). As we may be different in implementation, it would probably be safe to round up program space to say 2048 bytes.

### Line ordering requirements (must be ascending?)

Lines are in any order, but need to be moved in memory when entered (yes, it is o(n^2), but that's what the real machine did).

### Memory initialization - Should variables start as NUM=0 or undefined?

Variables start as NUM=0, STR="".

### REM statement - Should the rest of the line after REM be completely ignored or stored as a comment token?

The remark is stored, as it will be needed later for the "LIST" command (not in initial scope).
It may be a good idea to implement LIST early to debug the internal representation.

### No Makefile or build configuration

Create a simple Makefile. Executable name can be pc1211.

### No actual test runner implementation (just the plan mentions t_runner.c)

Implement what you need/want.
