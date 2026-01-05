# Test Cases for Bahnasy Tree

This directory contains comprehensive test cases for validating correctness and performance.

## Test Structure

### **Samples (01-05)**
Small hand-crafted test cases for debugging and understanding.

### **Simple Tests (06-12)**
Progressive difficulty tests with random operations:
- **06**: 10 elements, 20 queries
- **07**: 100 elements, 200 queries  
- **08**: 1k elements, 2k queries
- **09**: 5k elements, 10k queries
- **10**: 10k elements, 20k queries
- **11**: 50k elements, 50k queries
- **12**: 100k elements, 100k queries

### **Worst Case Tests (13-42)**
Adversarial patterns designed to stress-test specific scenarios:
- Insert at position 1 repeatedly
- Delete from beginning
- Sequential operations
- Pathological split patterns

### **Stress Tests (43-93)**
Advanced test patterns:
- **Fibonacci**: Insert operations following Fibonacci sequence
- **Cascade**: Cascading insert/delete patterns
- **Spiral**: Spiral insertion pattern
- **Prime Killer**: Operations at prime positions
- **Pathological**: Edge cases for tree structure
- **Memory Stress**: Maximum memory usage tests
- **Adversarial**: Designed to maximize operation count

## Running Tests

### Using Your Solution

```bash
g++ -O3 -std=c++17 ../src/competitive/bahnasy_sum.cpp -o solution

# Run single test
./solution < inputs/01.txt

# Run all tests (if you have outputs)
for i in {01..93}; do
    ./solution < inputs/$i.txt > my_output.txt
    diff my_output.txt outputs/$i.txt || echo "Test $i failed"
done
