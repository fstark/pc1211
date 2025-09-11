# PC-1211 BASIC Test Harness

This directory contains a comprehensive test harness for the PC-1211 BASIC interpreter, designed to ensure code quality and prevent regressions during refactoring.

## Quick Start

```bash
# Run all tests
./run_tests.sh

# Save current outputs as reference baseline
./run_tests.sh save

# Compare current run with saved reference
./run_tests.sh compare
```

## Test Organization

### Test Files (72 total)
- **Phase Tests**: `test_phase*.bas` - Test core functionality by implementation phase
- **Feature Tests**: `test_*.bas` - Test specific features (math, strings, labels, etc.)
- **Error Tests**: `test_*error*.bas` - Test error conditions and edge cases  
- **Debug Tests**: `debug_*.bas` - Development/debugging test cases
- **Edge Cases**: Various boundary condition and regression tests

### Expected Outcomes
The test harness automatically classifies tests based on naming patterns:

**Expected to Pass (57 tests)**:
- All `test_phase*.bas` files
- Most `test_*.bas` files  
- All `minimal_*.bas` and basic functionality tests

**Expected to Fail/Timeout (15 tests)**:
- `*error*.bas` - Tests that should produce errors
- `*domain*.bas` - Math domain error tests
- `*input*.bas` - INPUT tests (timeout waiting for user input)
- `debug_*.bas` - Debug tests (often test edge cases)
- `test_computed_debug.bas` - Infinite loop test

## Test Harness Features

### Automatic Classification
- Detects tests that should pass vs. fail based on naming
- Handles timeouts appropriately (INPUT tests, infinite loops)
- Reports unexpected failures separately from expected ones

### Reference Baseline System  
- Save test outputs as JSON reference: `test_references.json`
- Compare future runs against baseline to detect regressions
- Track changes in exit codes and output content
- MD5 hashing for efficient output comparison

### Regression Detection
```bash
# Make code changes...
./run_tests.sh compare
```
Output shows:
- New tests added
- Tests removed  
- Changed exit codes
- Changed outputs
- "No changes detected" if everything matches

### Performance Monitoring
- Execution time tracking for each test
- 10-second timeout for hanging tests
- Summary statistics

## Usage Examples

### During Development
```bash
# Quick test after making changes
./run_tests.sh quick

# Full test suite
./run_tests.sh run

# Check for regressions
./run_tests.sh compare
```

### Before Major Refactoring
```bash
# Save current state as baseline
./run_tests.sh save

# After refactoring...
./run_tests.sh compare
```

### Continuous Integration
```bash
# In CI pipeline
python3 test_harness.py --compare
if [ $? -ne 0 ]; then
    echo "Tests changed - review required"
    exit 1
fi
```

## Test Results Interpretation

### Success Indicators
- **Total tests: 72**
- **Passed: 62** (all non-error tests working)  
- **Failed: 10** (expected errors/timeouts)
- **Unexpected failures: 0** (the key metric)

### What to Watch For
- **Unexpected failures** - Always investigate these
- **Changed test outputs** - May indicate bugs or behavior changes
- **New timeouts** - Could indicate infinite loops introduced
- **Performance regressions** - Execution time increases

## Reference Implementation Goals

This test harness is designed to support the goal of creating a reference implementation for assembly development on real PC-1211 hardware:

### Quality Assurance
- Ensures all PC-1211 BASIC features work correctly
- Prevents regressions during code refactoring  
- Validates edge cases and error conditions

### Refactoring Support  
- Enables confident large-scale code restructuring
- Immediate feedback on functionality preservation
- Baseline comparison prevents subtle bugs

### Documentation
- Comprehensive test coverage serves as behavior documentation
- Examples of expected PC-1211 BASIC behavior
- Edge case handling reference

## Files

- `test_harness.py` - Main Python test harness
- `run_tests.sh` - Shell wrapper script  
- `test_references.json` - Saved reference baseline (auto-generated)
- `tests/*.bas` - 72 test programs
- `docs/Test_Harness.md` - This documentation

## Technical Details

### Implementation
- **Language**: Python 3 with minimal dependencies
- **Test Discovery**: Automatic `.bas` file detection
- **Execution**: Subprocess calls to `src/pc1211 --run`
- **Output Capture**: Both stdout and stderr captured
- **Timeout Handling**: 10-second limit with graceful handling
- **Comparison**: MD5 hashing for efficient output comparison

### Error Handling
- Graceful timeout handling for INPUT tests
- Distinguishes expected vs unexpected failures
- Detailed error reporting with context
- Non-zero exit codes for CI integration

This test harness provides a solid foundation for maintaining code quality while refactoring the PC-1211 BASIC interpreter, supporting the goal of creating a reliable reference implementation.
