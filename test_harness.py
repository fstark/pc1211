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
        self.is_timeout = (exit_code == -1 and "timed out" in stderr.lower())
        self.expected_to_timeout = self.name.endswith('_timeout.bas')
    
    def _determine_expected_outcome(self) -> bool:
        """Determine if this test is expected to fail based on explicit naming"""
        # All test files must end with either _pass.bas, _fail.bas, or _timeout.bas
        if self.name.endswith('_pass.bas'):
            return False  # Expected to pass (exit code 0)
        elif self.name.endswith('_fail.bas'):
            return True   # Expected to fail (non-zero exit code)
        elif self.name.endswith('_timeout.bas'):
            return False  # Expected to timeout (handled separately)
        else:
            # This should never happen - test harness should refuse to run
            raise ValueError(f"Test file {self.name} must end with '_pass.bas', '_fail.bas', or '_timeout.bas'")
    
    def is_success(self) -> bool:
        """Test succeeded if exit code matches expectation AND output format is correct"""
        
        # Expected timeouts are successful if they actually timed out
        if self.expected_to_timeout:
            return self.is_timeout
        
        # Unexpected timeouts are always failures
        if self.is_timeout:
            self.fail_reason = "Test timed out"
            return False
        
        # First check for proper output format
        output_lines = self.stdout.strip().split('\n')
        has_pass_fail_marker = False
        test_passed_by_output = False
        fail_reason = None
        
        for line in output_lines:
            if line.startswith('PASS:'):
                has_pass_fail_marker = True
                test_passed_by_output = True
                break
            elif line.startswith('FAIL:'):
                has_pass_fail_marker = True
                test_passed_by_output = False
                fail_reason = line[5:].strip()  # Extract reason after "FAIL:"
                break
        
        # Store the fail reason for reporting
        if fail_reason:
            self.fail_reason = fail_reason
        else:
            self.fail_reason = None
        
        # If exit code is not 0, test failed regardless of output
        if self.exit_code != 0:
            if not hasattr(self, 'fail_reason'):
                self.fail_reason = f"Exit code {self.exit_code}"
            return self.expected_to_fail  # Success if we expected it to fail
        
        # Exit code is 0, check output format
        if not has_pass_fail_marker:
            self.fail_reason = "Malformed test - no PASS: or FAIL: marker found"
            return False  # Always fail malformed tests
        
        # Test has proper format, check if result matches expectation
        if self.expected_to_fail:
            # Expected to fail - success if output shows FAIL:
            return not test_passed_by_output
        else:
            # Expected to pass - success if output shows PASS:
            return test_passed_by_output
    
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
        
        # Use the test file path as-is since it's already relative to the workspace
        print(f"Running {test_file}...", end=" ", flush=True)
        
        start_time = time.time()
        try:
            result = subprocess.run(
                [str(self.pc1211_path), str(test_file), "--run"],
                capture_output=True,
                text=True,
                timeout=0.5  # 500ms timeout for faster testing
            )
            execution_time = time.time() - start_time
            
            test_result = TestResult(
                name=str(test_file),
                exit_code=result.returncode,
                stdout=result.stdout,
                stderr=result.stderr,
                execution_time=execution_time
            )
            
            # Display the result
            if test_result.is_timeout:
                if test_result.expected_to_timeout:
                    print(f"TIMEOUT (expected) ({execution_time:.3f}s)")
                else:
                    print(f"TIMEOUT ({execution_time:.3f}s)")
            elif test_result.is_success():
                status = "PASS"
                expected = "(expected)" if test_result.expected_to_fail else ""
                print(f"{status} {expected} ({execution_time:.3f}s)")
            else:
                # Test failed - show the reason
                status = "FAIL"
                expected = "(expected)" if test_result.expected_to_fail else ""
                reason = getattr(test_result, 'fail_reason', 'Unknown failure')
                print(f"{status} {expected} ({execution_time:.3f}s)")
                if reason and not test_result.expected_to_fail:
                    # Only show reason for unexpected failures
                    print(f"    Reason: {reason}")
            
            return test_result
            
        except subprocess.TimeoutExpired:
            execution_time = time.time() - start_time
            print(f"TIMEOUT ({execution_time:.3f}s)")
            timeout_result = TestResult(
                name=test_file.name,
                exit_code=-1,
                stdout="",
                stderr="Test timed out after 0.5 seconds",
                execution_time=execution_time
            )
            timeout_result.fail_reason = "Test timed out"
            return timeout_result
        except Exception as e:
            execution_time = time.time() - start_time
            print(f"ERROR: {e}")
            error_result = TestResult(
                name=test_file.name,
                exit_code=-2,
                stdout="",
                stderr=f"Test harness error: {e}",
                execution_time=execution_time
            )
            error_result.fail_reason = f"Test harness error: {e}"
            return error_result
    
    def run_all_tests(self) -> None:
        """Run all tests and collect results"""
        test_files = self.find_test_files()
        
        # Validate all test files have proper naming
        invalid_files = []
        for test_file in test_files:
            if not (test_file.name.endswith('_pass.bas') or test_file.name.endswith('_fail.bas') or test_file.name.endswith('_timeout.bas')):
                invalid_files.append(test_file.name)
        
        if invalid_files:
            print("ERROR: The following test files do not end with '_pass.bas', '_fail.bas', or '_timeout.bas':")
            for filename in invalid_files:
                print(f"  - {filename}")
            print("\nAll test files must be explicitly named to indicate expected outcome.")
            print("Please rename them with either '_pass.bas', '_fail.bas', or '_timeout.bas' suffix.")
            sys.exit(1)
        
        if not self.pc1211_path.exists():
            print(f"Error: PC-1211 interpreter not found at {self.pc1211_path}")
            print("Please build it first: gcc -Wall -Wextra -O2 -o src/pc1211 src/*.c")
            sys.exit(1)
        
        print(f"Found {len(test_files)} test files")
        print("=" * 60)
        
        self.results = []
        for test_file in test_files:
            try:
                result = self.run_single_test(test_file)
                self.results.append(result)
            except Exception as e:
                print(f"CRITICAL ERROR running test {test_file}: {e}")
                print(f"This test will be missing from results - this is the bug!")
                # Create a dummy failed result so the test isn't silently dropped
                dummy_result = TestResult(
                    name=str(test_file),
                    exit_code=-999,
                    stdout="",
                    stderr=f"Critical test harness error: {e}",
                    execution_time=0.0
                )
                self.results.append(dummy_result)
        
        self.print_summary()
    
    def print_summary(self) -> None:
        """Print test run summary"""
        total_tests = len(self.results)
        
        # Categorize results properly
        expected_passes = []    # Should pass and did pass
        expected_failures = []  # Should fail and did fail  
        unexpected_failures = [] # Should pass but failed
        unexpected_passes = []   # Should fail but passed
        expected_timeouts = []  # Should timeout and did timeout
        unexpected_timeouts = [] # Should not timeout but did timeout
        
        for r in self.results:
            if r.is_timeout:
                if r.expected_to_timeout:
                    expected_timeouts.append(r)
                else:
                    unexpected_timeouts.append(r)
            elif r.expected_to_timeout:
                # Test was expected to timeout but didn't
                if r.is_success():
                    unexpected_passes.append(r)  # Didn't timeout when it should have
                else:
                    unexpected_failures.append(r)  # Failed for other reasons
            elif r.expected_to_fail:
                if r.is_success():
                    expected_failures.append(r)
                else:
                    unexpected_passes.append(r)
            else:
                if r.is_success():
                    expected_passes.append(r)
                else:
                    unexpected_failures.append(r)
        
        print("=" * 60)
        print("TEST SUMMARY")
        print("=" * 60)
        print(f"Total tests:             {total_tests}")
        print(f"Expected passes:         {len(expected_passes)}")
        print(f"Expected failures:       {len(expected_failures)}")
        print(f"Expected timeouts:       {len(expected_timeouts)}")
        print(f"Unexpected failures:     {len(unexpected_failures)}")
        print(f"Unexpected passes:       {len(unexpected_passes)}")
        print(f"Unexpected timeouts:     {len(unexpected_timeouts)}")
        print()
        
        if len(unexpected_timeouts) > 0:
            print("UNEXPECTED TIMEOUTS (tests that took too long):")
            for r in unexpected_timeouts:
                print(f"  - {r.name}")
            print()
        
        if len(unexpected_failures) > 0:
            print("UNEXPECTED FAILURES (should have passed):")
            for result in unexpected_failures:
                print(f"  - {result.name} (exit code: {result.exit_code})")
            print()
            
        if len(unexpected_passes) > 0:
            print("UNEXPECTED PASSES (should have failed):")
            for result in unexpected_passes:
                print(f"  - {result.name} (exit code: {result.exit_code})")
            print()
    
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
    
    # Exit with error code if there were any unexpected timeouts or unexpected failures
    unexpected_timeouts = [r for r in harness.results if r.is_timeout and not r.expected_to_timeout]
    unexpected_failures = [r for r in harness.results if not r.expected_to_fail and not r.expected_to_timeout and not r.is_success()]
    
    if unexpected_timeouts or unexpected_failures:
        sys.exit(1)  # Exit with error code for unexpected timeouts or unexpected failures

if __name__ == "__main__":
    main()
