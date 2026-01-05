#!/usr/bin/env python3
"""
Bahnasy Tree Benchmark Tool (Multi-folder version)
Usage: python3 benchmark.py <solution.cpp> [--category all] [--output results.csv]
"""

import subprocess
import sys
import time
import os
import argparse
from pathlib import Path

class Colors:
    BLUE = '\033[0;34m'
    GREEN = '\033[0;32m'
    RED = '\033[0;31m'
    YELLOW = '\033[1;33m'
    CYAN = '\033[0;36m'
    NC = '\033[0m'

FOLDERS = {
    'samples': 'Samples',
    'simple': 'Simple Tests',
    'worst': 'Worst Case Tests',
    'stress': 'Stress Tests'
}

def compile_solution(cpp_file):
    """Compile C++ solution"""
    print(f"{Colors.BLUE}Compiling {cpp_file}...{Colors.NC}")
    
    result = subprocess.run(
        ['g++', '-O3', '-std=c++17', cpp_file, '-o', 'benchmark_sol'],
        capture_output=True,
        text=True
    )
    
    if result.returncode != 0:
        print(f"{Colors.RED}Compilation failed:{Colors.NC}")
        print(result.stderr)
        return False
    
    print(f"{Colors.GREEN}✓ Compilation successful{Colors.NC}\n")
    return True

def run_test(test_file, timeout=10):
    """Run solution on test file"""
    if not Path(test_file).exists():
        return None
    
    start = time.time()
    
    try:
        with open(test_file, 'r') as f:
            result = subprocess.run(
                ['./benchmark_sol'],
                stdin=f,
                capture_output=True,
                timeout=timeout,
                text=True
            )
        
        elapsed = (time.time() - start) * 1000  # ms
        
        # Get memory (Linux)
        mem_kb = 0
        if sys.platform == 'linux':
            try:
                mem_result = subprocess.run(
                    ['/usr/bin/time', '-v', './benchmark_sol'],
                    stdin=open(test_file, 'r'),
                    capture_output=True,
                    text=True
                )
                for line in mem_result.stderr.split('\n'):
                    if 'Maximum resident set size' in line:
                        mem_kb = int(line.split(':')[1].strip())
                        break
            except:
                pass
        
        return {
            'time_ms': elapsed,
            'memory_mb': mem_kb / 1024,
            'exit_code': result.returncode,
            'output': result.stdout
        }
    
    except subprocess.TimeoutExpired:
        return {
            'time_ms': timeout * 1000,
            'memory_mb': 0,
            'exit_code': -1,
            'output': 'TIMEOUT'
        }

def main():
    parser = argparse.ArgumentParser(description='Benchmark Bahnasy Tree')
    parser.add_argument('solution', help='C++ solution file')
    parser.add_argument('--category', default='all', 
                       choices=['samples', 'simple', 'worst', 'stress', 'all'],
                       help='Test category to run')
    parser.add_argument('--output', default='benchmark_results.csv', help='Output CSV')
    parser.add_argument('--timeout', type=int, default=10, help='Timeout per test (sec)')
    
    args = parser.parse_args()
    
    # Compile
    if not compile_solution(args.solution):
        return 1
    
    # Determine folders to test
    if args.category == 'all':
        test_folders = list(FOLDERS.values())
    else:
        test_folders = [FOLDERS[args.category]]
    
    # Run benchmarks
    all_results = []
    total_passed = 0
    total_failed = 0
    
    print(f"{Colors.CYAN}Running benchmarks...{Colors.NC}\n")
    
    for folder in test_folders:
        folder_path = Path(folder)
        
        if not folder_path.exists():
            print(f"{Colors.YELLOW}Warning: {folder} not found{Colors.NC}")
            continue
        
        # Get test files
        test_files = sorted([f for f in folder_path.iterdir() if f.is_file()],
                          key=lambda x: int(x.name) if x.name.isdigit() else 0)
        
        if not test_files:
            continue
        
        print(f"{Colors.CYAN}{'='*60}{Colors.NC}")
        print(f"{Colors.CYAN}  {folder} ({len(test_files)} tests){Colors.NC}")
        print(f"{Colors.CYAN}{'='*60}{Colors.NC}\n")
        
        print("┌──────┬─────────┬──────────┬──────────┐")
        print("│ Test │ Result  │ Time(ms) │ Mem(MB)  │")
        print("├──────┼─────────┼──────────┼──────────┤")
        
        for test_file in test_files:
            result = run_test(str(test_file), args.timeout)
            
            if result is None:
                continue
            
            test_name = test_file.name
            
            if result['exit_code'] == 0:
                status = f"{Colors.GREEN}✓ PASS{Colors.NC}"
                total_passed += 1
                status_text = 'PASS'
            else:
                status = f"{Colors.RED}✗ FAIL{Colors.NC}"
                total_failed += 1
                status_text = 'FAIL'
            
            print(f"│ {test_name:4s} │ {status}  │ {result['time_ms']:8.1f} │ {result['memory_mb']:8.1f} │")
            
            all_results.append({
                'category': folder,
                'test': test_name,
                'time_ms': result['time_ms'],
                'memory_mb': result['memory_mb'],
                'status': status_text
            })
        
        print("└──────┴─────────┴──────────┴──────────┘\n")
    
    # Overall summary
    total = total_passed + total_failed
    total_time = sum(r['time_ms'] for r in all_results)
    avg_time = total_time / total if total > 0 else 0
    max_mem = max((r['memory_mb'] for r in all_results), default=0)
    
    print(f"{Colors.CYAN}╔{'='*58}╗{Colors.NC}")
    print(f"{Colors.CYAN}║{'OVERALL SUMMARY':^58}║{Colors.NC}")
    print(f"{Colors.CYAN}╚{'='*58}╝{Colors.NC}")
    print(f"Total Tests:   {total}")
    print(f"{Colors.GREEN}Passed:        {total_passed}{Colors.NC}")
    if total_failed > 0:
        print(f"{Colors.RED}Failed:        {total_failed}{Colors.NC}")
    else:
        print(f"Failed:        {total_failed}")
    print(f"Total Time:    {total_time:.1f} ms")
    print(f"Average Time:  {avg_time:.1f} ms")
    print(f"Max Memory:    {max_mem:.1f} MB")
    print(f"{Colors.CYAN}╚{'='*58}╝{Colors.NC}")
    
    # Save CSV
    with open(args.output, 'w') as f:
        f.write("category,test,time_ms,memory_mb,status\n")
        for r in all_results:
            f.write(f"{r['category']},{r['test']},{r['time_ms']:.1f},{r['memory_mb']:.1f},{r['status']}\n")
    
    print(f"\n{Colors.GREEN}Results saved to {args.output}{Colors.NC}")
    
    # Cleanup
    if Path('benchmark_sol').exists():
        os.remove('benchmark_sol')
    
    return 0 if total_failed == 0 else 1

if __name__ == '__main__':
    sys.exit(main())
