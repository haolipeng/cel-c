#!/bin/bash

set -e

BUILD_DIR="${1:-build}"

echo "Running Valgrind memory checks..."

cd "$BUILD_DIR"

# 为每个测试运行 Valgrind
for test_exe in tests/test_*; do
    if [ -x "$test_exe" ]; then
        echo ""
        echo "Checking $test_exe..."
        valgrind --leak-check=full \
                 --show-leak-kinds=all \
                 --track-origins=yes \
                 --error-exitcode=1 \
                 "$test_exe"
    fi
done

echo ""
echo "All memory checks passed!"
