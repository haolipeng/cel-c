#!/bin/bash

set -e

BUILD_DIR="${1:-build}"
BUILD_TYPE="${2:-Debug}"

echo "Building tests in $BUILD_DIR ($BUILD_TYPE)..."

# 创建构建目录
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 配置
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -DCEL_BUILD_TESTS=ON \
      -DCEL_USE_ASAN=ON \
      ..

# 编译
make -j$(nproc)

# 运行测试
echo ""
echo "Running tests..."
ctest --output-on-failure

echo ""
echo "All tests passed!"
