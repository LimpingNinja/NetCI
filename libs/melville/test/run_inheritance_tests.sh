#!/bin/bash
# Run inheritance tests and capture output

echo "Starting NetCI with inheritance tests..."
echo "========================================"
echo ""

# Run netci in noisy mode and capture output
timeout 5 ./netci -noisy 2>&1 | grep -E "(Test [0-9]:|PASS|FAIL|INHERITANCE TEST SUITE|TEST SUITE COMPLETE)" || true

echo ""
echo "========================================"
echo "Test run complete!"
