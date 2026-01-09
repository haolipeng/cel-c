# CEL-C 构建指南

## 系统要求

- C11 兼容编译器 (GCC 7+, Clang 5+)
- CMake 3.14+
- 可选: PCRE2 (正则表达式支持)
- 可选: Valgrind (内存检测)

## 快速构建

```bash
git clone <repository>
cd cel-c
mkdir build && cd build
cmake ..
make -j4
```

## 构建选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| CEL_BUILD_TESTS | ON | 构建测试 |
| CEL_BUILD_EXAMPLES | OFF | 构建示例 |
| CEL_BUILD_BENCH | OFF | 构建基准测试 |
| CEL_BUILD_FUZZ | OFF | 构建模糊测试 |
| CEL_ENABLE_JSON | OFF | 启用 JSON 转换 |
| CEL_ENABLE_REGEX | OFF | 启用正则表达式 |
| CEL_ENABLE_CHRONO | ON | 启用时间类型 |
| CEL_USE_ASAN | OFF | 启用 AddressSanitizer |
| CEL_THREAD_SAFE | OFF | 启用线程安全 |

## 完整构建示例

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCEL_ENABLE_JSON=ON \
    -DCEL_ENABLE_REGEX=ON \
    -DCEL_BUILD_TESTS=ON \
    -DCEL_BUILD_BENCH=ON

make -j$(nproc)
```

## 运行测试

```bash
# 运行所有测试
ctest --output-on-failure

# 运行特定测试
./tests/test_parser
./tests/test_functions

# 内存检测
valgrind --leak-check=full ./tests/test_memory
```

## 运行基准测试

```bash
cmake .. -DCEL_BUILD_BENCH=ON
make bench_cel
./bench/bench_cel
```

## 安装

```bash
sudo make install
```

默认安装到:
- 头文件: `/usr/local/include/cel/`
- 库文件: `/usr/local/lib/libcel.so`

## 依赖项

### 必需
- SDS (Simple Dynamic Strings) - 已包含在 third_party/
- Unity (测试框架) - 已包含在 third_party/

### 可选
- PCRE2: `apt install libpcre2-dev`
- cJSON: 已包含在 third_party/ (用于 JSON 支持)

## 交叉编译

```bash
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release
```

## 调试构建

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCEL_USE_ASAN=ON

make -j4
```
