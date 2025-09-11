#!/bin/bash
#
# PC-1211 BASIC Test Runner
# Simple wrapper around the Python test harness
#

set -e

# Build the interpreter if it doesn't exist or source files are newer
needs_build=0
if [ ! -f "src/pc1211" ]; then
    needs_build=1
else
    for src_file in src/*.c src/*.h; do
        if [ "$src_file" -nt "src/pc1211" ]; then
            needs_build=1
            break
        fi
    done
fi

if [ $needs_build -eq 1 ]; then
    echo "Building PC-1211 interpreter..."
    gcc -Wall -Wextra -O2 -o src/pc1211 src/*.c
    echo "Build complete."
    echo
fi

# Run tests based on command line arguments
case "${1:-run}" in
    "run")
        echo "Running all tests..."
        python3 test_harness.py
        ;;
    "save")
        echo "Running tests and saving as reference..."
        python3 test_harness.py --save-reference
        ;;
    "compare")
        echo "Running tests and comparing with reference..."
        python3 test_harness.py --compare
        ;;
    "quick")
        echo "Running quick subset of tests..."
        # Run a subset of key tests for quick validation
        python3 test_harness.py | grep -E "(PASS|FAIL|Total tests|Passed|Failed)"
        ;;
    *)
        echo "Usage: $0 [run|save|compare|quick]"
        echo "  run     - Run all tests (default)"
        echo "  save    - Run tests and save as reference baseline"
        echo "  compare - Run tests and compare with saved reference"
        echo "  quick   - Run tests with minimal output"
        exit 1
        ;;
esac
