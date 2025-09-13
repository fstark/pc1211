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
