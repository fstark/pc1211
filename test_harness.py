#!/usr/bin/env python3
"""
PC-1211 BASIC Test Harness

This test harness runs all .bas files in the tests/ directory and creates
reference outputs for regression testing. It automatically classifies tests
as expected to pass or fail based on naming patterns and exit codes.
"""

import os
import subprocess
import sys
import json
import hashlib
from pathlib import Path
from typing import Dict, List, Tuple, Optional

class TestResult:
    def __init__(self, name: str, exit_code: int, stdout: str, stderr: str, execution_time: float):
        self.name = name
        self.exit_code = exit_code
        self.stdout = stdout
        self.stderr = stderr
        self.execution_time = execution_time
        self.expected_to_fail = self._determine_expected_outcome()
    
    def _determine_expected_outcome(self) -> bool:
        """Determine if this test is expected to fail based on naming patterns"""
        fail_patterns = [
            'error', 'fail', 'domain', 'overflow', 'invalid', 'bad',
            'test_error', 'test_domain', 'debug_'  # debug files often test edge cases
        ]
        
        # INPUT tests will timeout waiting for user input - this is expected
        input_patterns = ['input', 'computed_debug']  # computed_debug has infinite loop
        
        is_expected_fail = any(pattern in self.name.lower() for pattern in fail_patterns)
        is_input_test = any(pattern in self.name.lower() for pattern in input_patterns)
        
        return is_expected_fail or is_input_test
    
    def is_success(self) -> bool:
        """Test succeeded if exit code matches expectation"""
        if self.expected_to_fail:
            # For input tests or infinite loops, timeout (-1) is success
            # For error tests, non-zero exit code is success
            return self.exit_code != 0 or self.exit_code == -1
        else:
            return self.exit_code == 0   # Should pass
    
    def to_dict(self) -> Dict:
        return {
            'name': self.name,
            'exit_code': self.exit_code,
            'stdout': self.stdout,
            'stderr': self.stderr,
            'execution_time': self.execution_time,
            'expected_to_fail': self.expected_to_fail,
            'success': self.is_success(),
            'output_hash': hashlib.md5(self.stdout.encode()).hexdigest()
        }

class TestHarness:
    def __init__(self, pc1211_path: str, tests_dir: str, reference_file: str = "test_references.json"):
        self.pc1211_path = Path(pc1211_path)
        self.tests_dir = Path(tests_dir)
        self.reference_file = Path(reference_file)
        self.results: List[TestResult] = []
    
    def find_test_files(self) -> List[Path]:
        """Find all .bas test files"""
        return sorted(self.tests_dir.glob("*.bas"))
    
    def run_single_test(self, test_file: Path) -> TestResult:
        """Run a single test file and capture results"""
        import time
        
        print(f"Running {test_file.name}...", end=" ", flush=True)
        
        start_time = time.time()
        try:
            result = subprocess.run(
                [str(self.pc1211_path), str(test_file), "--run"],
                capture_output=True,
                text=True,
                timeout=10  # 10 second timeout
            )
            execution_time = time.time() - start_time
            
            test_result = TestResult(
                name=test_file.name,
                exit_code=result.returncode,
                stdout=result.stdout,
                stderr=result.stderr,
                execution_time=execution_time
            )
            
            status = "PASS" if test_result.is_success() else "FAIL"
            expected = "(expected)" if test_result.expected_to_fail else ""
            print(f"{status} {expected} ({execution_time:.3f}s)")
            
            return test_result
            
        except subprocess.TimeoutExpired:
            execution_time = time.time() - start_time
            print(f"TIMEOUT ({execution_time:.3f}s)")
            return TestResult(
                name=test_file.name,
                exit_code=-1,
                stdout="",
                stderr="Test timed out after 10 seconds",
                execution_time=execution_time
            )
        except Exception as e:
            execution_time = time.time() - start_time
            print(f"ERROR: {e}")
            return TestResult(
                name=test_file.name,
                exit_code=-2,
                stdout="",
                stderr=f"Test harness error: {e}",
                execution_time=execution_time
            )
    
    def run_all_tests(self) -> None:
        """Run all tests and collect results"""
        test_files = self.find_test_files()
        
        if not self.pc1211_path.exists():
            print(f"Error: PC-1211 interpreter not found at {self.pc1211_path}")
            print("Please build it first: gcc -Wall -Wextra -O2 -o src/pc1211 src/*.c")
            sys.exit(1)
        
        print(f"Found {len(test_files)} test files")
        print("=" * 60)
        
        self.results = []
        for test_file in test_files:
            result = self.run_single_test(test_file)
            self.results.append(result)
        
        self.print_summary()
    
    def print_summary(self) -> None:
        """Print test run summary"""
        total_tests = len(self.results)
        passed_tests = sum(1 for r in self.results if r.is_success())
        failed_tests = total_tests - passed_tests
        
        expected_failures = sum(1 for r in self.results if r.expected_to_fail)
        unexpected_failures = sum(1 for r in self.results if not r.is_success() and not r.expected_to_fail)
        
        print("=" * 60)
        print("TEST SUMMARY")
        print("=" * 60)
        print(f"Total tests:           {total_tests}")
        print(f"Passed:                {passed_tests}")
        print(f"Failed:                {failed_tests}")
        print(f"Expected failures:     {expected_failures}")
        print(f"Unexpected failures:   {unexpected_failures}")
        print()
        
        if unexpected_failures > 0:
            print("UNEXPECTED FAILURES:")
            for result in self.results:
                if not result.is_success() and not result.expected_to_fail:
                    print(f"  - {result.name} (exit code: {result.exit_code})")
            print()
        
        # Show some example outputs
        print("SAMPLE OUTPUTS:")
        for i, result in enumerate(self.results[:3]):
            print(f"\n--- {result.name} ---")
            print(f"Exit code: {result.exit_code}")
            if result.stdout:
                print("Output:")
                print(result.stdout[:200] + ("..." if len(result.stdout) > 200 else ""))
    
    def save_reference(self) -> None:
        """Save current test results as reference for future comparison"""
        reference_data = {
            'version': '1.0',
            'pc1211_version': 'v0.5',
            'total_tests': len(self.results),
            'results': [result.to_dict() for result in self.results]
        }
        
        with open(self.reference_file, 'w') as f:
            json.dump(reference_data, f, indent=2)
        
        print(f"Reference saved to {self.reference_file}")
    
    def load_reference(self) -> Optional[Dict]:
        """Load reference test results"""
        if not self.reference_file.exists():
            return None
        
        with open(self.reference_file, 'r') as f:
            return json.load(f)
    
    def compare_with_reference(self) -> None:
        """Compare current results with saved reference"""
        reference = self.load_reference()
        if not reference:
            print("No reference file found. Run with --save-reference first.")
            return
        
        print("COMPARING WITH REFERENCE")
        print("=" * 60)
        
        current_by_name = {r.name: r for r in self.results}
        reference_by_name = {r['name']: r for r in reference['results']}
        
        changes = []
        for name, current in current_by_name.items():
            if name not in reference_by_name:
                changes.append(f"NEW: {name}")
                continue
            
            ref = reference_by_name[name]
            
            # Compare exit codes
            if current.exit_code != ref['exit_code']:
                changes.append(f"EXIT CODE CHANGED: {name} ({ref['exit_code']} -> {current.exit_code})")
            
            # Compare output hashes
            current_hash = hashlib.md5(current.stdout.encode()).hexdigest()
            if current_hash != ref['output_hash']:
                changes.append(f"OUTPUT CHANGED: {name}")
        
        # Check for removed tests
        for name in reference_by_name:
            if name not in current_by_name:
                changes.append(f"REMOVED: {name}")
        
        if changes:
            print(f"Found {len(changes)} changes:")
            for change in changes:
                print(f"  - {change}")
        else:
            print("No changes detected - all tests match reference!")

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="PC-1211 BASIC Test Harness")
    parser.add_argument("--pc1211", default="src/pc1211", help="Path to pc1211 interpreter")
    parser.add_argument("--tests", default="tests", help="Path to tests directory")
    parser.add_argument("--reference", default="test_references.json", help="Reference file path")
    parser.add_argument("--save-reference", action="store_true", help="Save results as new reference")
    parser.add_argument("--compare", action="store_true", help="Compare with existing reference")
    
    args = parser.parse_args()
    
    harness = TestHarness(args.pc1211, args.tests, args.reference)
    
    # Always run the tests
    harness.run_all_tests()
    
    if args.save_reference:
        harness.save_reference()
    
    if args.compare:
        harness.compare_with_reference()

if __name__ == "__main__":
    main()
