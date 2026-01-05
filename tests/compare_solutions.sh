#!/bin/bash

# Compare Two Solutions (Multi-folder version)
# Usage: ./compare_solutions.sh <sol1.cpp> <sol2.cpp> [category]

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Check arguments
if [ $# -lt 2 ]; then
    echo "Usage: $0 <solution1.cpp> <solution2.cpp> [category]"
    echo ""
    echo "Categories: samples, simple, worst, stress, all (default)"
    echo ""
    echo "Example: $0 my_solution.cpp ../src/competitive/bahnasy_sum.cpp samples"
    exit 1
fi

SOL1=$1
SOL2=$2
CATEGORY=${3:-"all"}

echo -e "${CYAN}╔═══════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║           BAHNASY TREE - SOLUTION COMPARISON              ║${NC}"
echo -e "${CYAN}╚═══════════════════════════════════════════════════════════╝${NC}"
echo ""

# Check files
if [ ! -f "$SOL1" ]; then
    echo -e "${RED}Error: Solution 1 '$SOL1' not found${NC}"
    exit 1
fi
if [ ! -f "$SOL2" ]; then
    echo -e "${RED}Error: Solution 2 '$SOL2' not found${NC}"
    exit 1
fi

# Compile both
echo -e "${BLUE}Compiling Solution 1: $SOL1...${NC}"
g++ -O3 -std=c++17 "$SOL1" -o sol1 2>&1 | head -10
if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo -e "${RED}Compilation failed!${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Solution 1 compiled${NC}\n"

echo -e "${BLUE}Compiling Solution 2: $SOL2...${NC}"
g++ -O3 -std=c++17 "$SOL2" -o sol2 2>&1 | head -10
if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo -e "${RED}Compilation failed!${NC}"
    rm -f sol1
    exit 1
fi
echo -e "${GREEN}✓ Solution 2 compiled${NC}\n"

# Define folders
declare -A FOLDERS
FOLDERS[samples]="Samples"
FOLDERS[simple]="Simple Tests"
FOLDERS[worst]="Worst Case Tests"
FOLDERS[stress]="Stress Tests"

# Determine which folders
if [ "$CATEGORY" = "all" ]; then
    TEST_FOLDERS=("${FOLDERS[@]}")
else
    if [ -z "${FOLDERS[$CATEGORY]}" ]; then
        echo -e "${RED}Invalid category: $CATEGORY${NC}"
        rm -f sol1 sol2
        exit 1
    fi
    TEST_FOLDERS=("${FOLDERS[$CATEGORY]}")
fi

# Overall results
TOTAL_TESTS=0
TOTAL_BOTH_OK=0
TOTAL_SOL1_ONLY=0
TOTAL_SOL2_ONLY=0
TOTAL_MISMATCH=0
TOTAL_BOTH_FAIL=0

OVERALL_TIME1=0
OVERALL_TIME2=0
OVERALL_MEM1=0
OVERALL_MEM2=0

# Test each folder
for FOLDER in "${TEST_FOLDERS[@]}"; do
    if [ ! -d "$FOLDER" ]; then
        continue
    fi
    
    TEST_FILES=($(ls "$FOLDER" 2>/dev/null | sort -n))
    
    if [ ${#TEST_FILES[@]} -eq 0 ]; then
        continue
    fi
    
    echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}  Comparing: $FOLDER (${#TEST_FILES[@]} tests)${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
    echo ""
    
    BOTH_OK=0
    SOL1_ONLY=0
    SOL2_ONLY=0
    MISMATCH=0
    BOTH_FAIL=0
    
    FOLDER_TIME1=0
    FOLDER_TIME2=0
    
    echo "┌──────┬──────────────┬─────────────────────────────────────────┐"
    echo "│ Test │   Status     │  Time (ms): S1 vs S2   │  Mem (MB)     │"
    echo "├──────┼──────────────┼────────────────────────┼───────────────┤"
    
    for TEST in "${TEST_FILES[@]}"; do
        TEST_FILE="$FOLDER/$TEST"
        
        if [ ! -f "$TEST_FILE" ]; then
            continue
        fi
        
        TOTAL_TESTS=$((TOTAL_TESTS + 1))
        
        # Run solution 1
        /usr/bin/time -f "%e %M" ./sol1 < "$TEST_FILE" > out1.txt 2> time1.txt
        EXIT1=$?
        read TIME1_SEC MEM1_KB < time1.txt 2>/dev/null || TIME1_SEC="0"
        TIME1_MS=$(echo "$TIME1_SEC * 1000" | bc | awk '{printf "%.0f", $0}')
        MEM1_MB=$(echo "$MEM1_KB / 1024" | bc | awk '{printf "%.1f", $0}')
        
        # Run solution 2
        /usr/bin/time -f "%e %M" ./sol2 < "$TEST_FILE" > out2.txt 2> time2.txt
        EXIT2=$?
        read TIME2_SEC MEM2_KB < time2.txt 2>/dev/null || TIME2_SEC="0"
        TIME2_MS=$(echo "$TIME2_SEC * 1000" | bc | awk '{printf "%.0f", $0}')
        MEM2_MB=$(echo "$MEM2_KB / 1024" | bc | awk '{printf "%.1f", $0}')
        
        FOLDER_TIME1=$(echo "$FOLDER_TIME1 + $TIME1_MS" | bc)
        FOLDER_TIME2=$(echo "$FOLDER_TIME2 + $TIME2_MS" | bc)
        OVERALL_TIME1=$(echo "$OVERALL_TIME1 + $TIME1_MS" | bc)
        OVERALL_TIME2=$(echo "$OVERALL_TIME2 + $TIME2_MS" | bc)
        OVERALL_MEM1=$(echo "$OVERALL_MEM1 + $MEM1_MB" | bc)
        OVERALL_MEM2=$(echo "$OVERALL_MEM2 + $MEM2_MB" | bc)
        
        # Compare
        if [ $EXIT1 -eq 0 ] && [ $EXIT2 -eq 0 ]; then
            if diff -q out1.txt out2.txt > /dev/null 2>&1; then
                BOTH_OK=$((BOTH_OK + 1))
                TOTAL_BOTH_OK=$((TOTAL_BOTH_OK + 1))
                STATUS="${GREEN}✓✓ MATCH${NC}"
            else
                MISMATCH=$((MISMATCH + 1))
                TOTAL_MISMATCH=$((TOTAL_MISMATCH + 1))
                STATUS="${RED}✗✗ DIFF${NC}"
            fi
        elif [ $EXIT1 -eq 0 ] && [ $EXIT2 -ne 0 ]; then
            SOL1_ONLY=$((SOL1_ONLY + 1))
            TOTAL_SOL1_ONLY=$((TOTAL_SOL1_ONLY + 1))
            STATUS="${YELLOW}✓✗ S1${NC}"
        elif [ $EXIT1 -ne 0 ] && [ $EXIT2 -eq 0 ]; then
            SOL2_ONLY=$((SOL2_ONLY + 1))
            TOTAL_SOL2_ONLY=$((TOTAL_SOL2_ONLY + 1))
            STATUS="${YELLOW}✗✓ S2${NC}"
        else
            BOTH_FAIL=$((BOTH_FAIL + 1))
            TOTAL_BOTH_FAIL=$((TOTAL_BOTH_FAIL + 1))
            STATUS="${RED}✗✗ FAIL${NC}"
        fi
        
        # Format time comparison
        if (( $(echo "$TIME1_MS < $TIME2_MS" | bc -l) )); then
            TIME_CMP="${GREEN}${TIME1_MS} < ${TIME2_MS}${NC}"
        elif (( $(echo "$TIME1_MS > $TIME2_MS" | bc -l) )); then
            TIME_CMP="${RED}${TIME1_MS} > ${TIME2_MS}${NC}"
        else
            TIME_CMP="${
