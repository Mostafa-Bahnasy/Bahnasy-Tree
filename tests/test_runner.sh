#!/bin/bash

# Bahnasy Tree Test Runner (Multi-folder version)
# Usage: ./test_runner.sh <solution.cpp> [category]
# Categories: samples, simple, worst, stress, all

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Check arguments
if [ $# -lt 1 ]; then
    echo "Usage: $0 <solution.cpp> [category]"
    echo ""
    echo "Categories:"
    echo "  samples  - Run only Samples/ tests"
    echo "  simple   - Run only Simple Tests/"
    echo "  worst    - Run only Worst Case Tests/"
    echo "  stress   - Run only Stress Tests/"
    echo "  all      - Run all tests (default)"
    echo ""
    echo "Examples:"
    echo "  $0 ../src/competitive/bahnasy_sum.cpp samples"
    echo "  $0 my_solution.cpp all"
    exit 1
fi

SOLUTION=$1
CATEGORY=${2:-"all"}

# Check if solution exists
if [ ! -f "$SOLUTION" ]; then
    echo -e "${RED}Error: Solution file '$SOLUTION' not found${NC}"
    exit 1
fi

# Compile solution
echo -e "${BLUE}Compiling $SOLUTION...${NC}"
g++ -O3 -std=c++17 -DLOCAL "$SOLUTION" -o solution 2>&1 | head -20
if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo -e "${RED}Compilation failed!${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Compilation successful!${NC}\n"

# Define test folders
declare -A FOLDERS
FOLDERS[samples]="Samples"
FOLDERS[simple]="Simple Tests"
FOLDERS[worst]="Worst Case Tests"
FOLDERS[stress]="Stress Tests"

# Determine which folders to test
if [ "$CATEGORY" = "all" ]; then
    TEST_FOLDERS=("${FOLDERS[@]}")
else
    if [ -z "${FOLDERS[$CATEGORY]}" ]; then
        echo -e "${RED}Invalid category: $CATEGORY${NC}"
        echo "Valid: samples, simple, worst, stress, all"
        rm -f solution
        exit 1
    fi
    TEST_FOLDERS=("${FOLDERS[$CATEGORY]}")
fi

# Results tracking
TOTAL_PASSED=0
TOTAL_FAILED=0
TOTAL_TESTS=0
TOTAL_TIME=0
MAX_MEMORY=0

# Run tests for each folder
for FOLDER in "${TEST_FOLDERS[@]}"; do
    if [ ! -d "$FOLDER" ]; then
        echo -e "${YELLOW}Warning: Folder '$FOLDER' not found, skipping...${NC}"
        continue
    fi
    
    # Get test files in this folder
    TEST_FILES=($(ls "$FOLDER" 2>/dev/null | sort -n))
    
    if [ ${#TEST_FILES[@]} -eq 0 ]; then
        continue
    fi
    
    echo -e "${CYAN}═══════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}  Testing: $FOLDER (${#TEST_FILES[@]} tests)${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════${NC}"
    echo ""
    
    PASSED=0
    FAILED=0
    FOLDER_TIME=0
    
    echo "┌──────┬─────────┬──────────┬──────────┬─────────────────┐"
    echo "│ Test │ Result  │ Time(ms) │ Mem(MB)  │ Status          │"
    echo "├──────┼─────────┼──────────┼──────────┼─────────────────┤"
    
    for TEST in "${TEST_FILES[@]}"; do
        TEST_FILE="$FOLDER/$TEST"
        
        if [ ! -f "$TEST_FILE" ]; then
            continue
        fi
        
        TOTAL_TESTS=$((TOTAL_TESTS + 1))
        
        # Run with time and memory measurement
        /usr/bin/time -f "%e %M" ./solution < "$TEST_FILE" > output.txt 2> time.txt
        EXIT_CODE=$?
        
        # Parse time and memory
        if [ -f time.txt ]; then
            read TIME_SEC MEM_KB < time.txt
            TIME_MS=$(echo "$TIME_SEC * 1000" | bc | awk '{printf "%.0f", $0}')
            MEM_MB=$(echo "$MEM_KB / 1024" | bc | awk '{printf "%.1f", $0}')
            FOLDER_TIME=$(echo "$FOLDER_TIME + $TIME_MS" | bc)
            TOTAL_TIME=$(echo "$TOTAL_TIME + $TIME_MS" | bc)
            
            # Update max memory
            if (( $(echo "$MEM_MB > $MAX_MEMORY" | bc -l) )); then
                MAX_MEMORY=$MEM_MB
            fi
        else
            TIME_MS="N/A"
            MEM_MB="N/A"
        fi
        
        # Check result
        if [ $EXIT_CODE -eq 0 ]; then
            PASSED=$((PASSED + 1))
            TOTAL_PASSED=$((TOTAL_PASSED + 1))
            RESULT="${GREEN}✓ PASS${NC}"
            STATUS="OK"
        else
            FAILED=$((FAILED + 1))
            TOTAL_FAILED=$((TOTAL_FAILED + 1))
            RESULT="${RED}✗ FAIL${NC}"
            STATUS="Runtime Error"
        fi
        
        printf "│ %-4s │ %-7b │ %8s │ %8s │ %-15s │\n" "$TEST" "$RESULT" "$TIME_MS" "$MEM_MB" "$STATUS"
    done
    
    echo "└──────┴─────────┴──────────┴──────────┴─────────────────┘"
    
    # Folder summary
    echo ""
    echo -e "Folder Results: ${GREEN}$PASSED passed${NC}, ${RED}$FAILED failed${NC}, Total time: ${FOLDER_TIME} ms"
    echo ""
done

# Overall Summary
echo ""
echo -e "${BLUE}╔═══════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║                   OVERALL SUMMARY                     ║${NC}"
echo -e "${BLUE}╚═══════════════════════════════════════════════════════╝${NC}"
echo -e "Total Tests:     $TOTAL_TESTS"
echo -e "${GREEN}Passed:          $TOTAL_PASSED${NC}"
if [ $TOTAL_FAILED -gt 0 ]; then
    echo -e "${RED}Failed:          $TOTAL_FAILED${NC}"
else
    echo -e "Failed:          $TOTAL_FAILED"
fi
echo -e "Success Rate:    $(echo "scale=2; $TOTAL_PASSED * 100 / $TOTAL_TESTS" | bc)%"
echo ""
echo -e "Total Time:      ${TOTAL_TIME} ms"
echo -e "Average Time:    $(echo "scale=2; $TOTAL_TIME / $TOTAL_TESTS" | bc) ms"
echo -e "Max Memory:      ${MAX_MEMORY} MB"
echo -e "${BLUE}╚═══════════════════════════════════════════════════════╝${NC}"

# Cleanup
rm -f solution output.txt time.txt

# Exit 
