This is a reference implementation of a BASIC interpreter for the Casio PC-1211 pocket computer, written in C.
The goal of this implementation is to serve as a model for writing 8 bits assembly code.
* Do not allocate memory
* Do not use run-time test to test for impossible situations, use 'assert'
* Use simple, direct code
* Favor code that cannot fail/return NULL
* Try not to repeat code, use helper functions
* Avoid special cases (like empty arrays, NULL pointers, etc)
* Use clear and consistent naming conventions
* Use comments to explain non-obvious code

There is a test harness, that you can execute from the top-directory, using ./run_tests.sh
* Never change the output of a test, even when fixing a bug. You human will do it.
* Never add a test that fails, even when fixing a bug. You human will do it.
* Never remove a test, even when fixing a bug. You human will do it.
* The test harness will help you to refactor the code, by checking that the behavior is unchanged. Run it often.

Stylistic remarks
* Use consistent indentation (4 spaces)
* Favor < over > when comparing, unless comparing with a constant (ie: use ``if (a<b)``, but also `` if (a>0)``)

Buffer and Memory Management
* Always account for terminators in buffer lengths - Empty data structures still consume space for their termination markers
* Terminators are real data, not conceptual - They have actual bytes in memory with defined structure
* Validate buffer math carefully - length fields typically include overhead, not just payload

Error Handling Philosophy
* Impossible situations use assert(), possible failures return error codes
* Validation functions should assert() for corruption, not return bool for programmer errors
* Fail fast on corruption - Use assert() rather than defensive programming for impossible states
* Reserved values (0xFF) for debugging - Use consistent sentinel values to detect uninitialized memory

API Design Principles
* Functions should have single, clear semantics - Avoid overloading meaning (e.g., "last line" means terminator, not final content line)
* Prefer direct checks over computed properties - get_len(ptr) == 0 is clearer than position arithmetic
* Consistent naming - If something is called "line", it should behave like a line in all contexts

Code Structure
* Minimize special cases - Design data structures so empty/normal cases follow same code paths
* Simple iteration patterns - Prefer for(init; !terminator_check; advance) over complex while loops with multiple exit conditions
* Explicit over implicit - Make buffer layouts and data structure overhead visible in the code

Debugging Support
* Include debug breadcrumbs - Add sentinel values, debug prints, and validation that can be easily enabled
* Make corruption visible - Structure code so buffer overruns and invalid states are immediately obvious
* Use assertions liberally - Assert on all invariants, especially in low-level buffer manipulations


After a successful change, ask me if I want you to commit. In such case, make clean, git status, git add, git commit with an appropriate message and git push
