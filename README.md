# CEL-C

C11 implementation of Common Expression Language (CEL).

## Project Status

🚧 **Under Development** - Phase 1 (Infrastructure) in progress

## Features (Planned)

- ✅ Full CEL specification compliance
- ✅ All standard operators and functions
- ✅ Macro support (has, all, exists, map, filter)
- ✅ Timestamp and duration support
- ✅ Optional regex support (PCRE2)
- ✅ Thread-safe reference counting
- ✅ Arena allocator for fast AST allocation

## Building

### Prerequisites

- **CMake** 3.15 or higher
- **C11-compatible compiler** (GCC 7+, Clang 6+, MSVC 2019+)
- **Git** (for third-party dependencies)
- **PCRE2** (optional, for regex support)
  - Ubuntu/Debian: `sudo apt-get install libpcre2-dev`
  - macOS: `brew install pcre2`

### Quick Start

```bash
# Clone the repository
git clone <repository-url> cel-c
cd cel-c

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
make -j$(nproc)

# Run tests
make test
# or
ctest --output-on-failure
```

### Build Options

Configure the build with CMake options:

| Option | Default | Description |
|--------|---------|-------------|
| `CEL_ENABLE_CHRONO` | ON | Enable timestamp and duration support |
| `CEL_ENABLE_REGEX` | ON | Enable regex support (requires PCRE2) |
| `CEL_ENABLE_JSON` | OFF | Enable JSON conversion support |
| `CEL_THREAD_SAFE` | ON | Enable thread-safe reference counting |
| `CEL_BUILD_TESTS` | ON | Build unit tests |
| `CEL_BUILD_BENCH` | OFF | Build benchmarks |
| `CEL_BUILD_EXAMPLES` | ON | Build examples |
| `CEL_USE_ASAN` | OFF | Enable AddressSanitizer |

Example:

```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCEL_USE_ASAN=ON \
      -DCEL_ENABLE_JSON=ON \
      ..
```

### Build Types

- **Debug**: `-g3 -O0`, includes debugging symbols
- **Release**: `-O3`, optimized for performance

```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build
cmake -DCMAKE_BUILD_TYPE=Release ..
```

## Usage

```c
#include <cel/cel.h>
#include <stdio.h>

int main() {
    // TODO: Add usage example after Phase 2 is complete

    // Example (coming soon):
    // cel_context_t* ctx = cel_context_create();
    // cel_program_t* prog = cel_compile("1 + 2", ctx);
    // cel_result_t result = cel_execute(prog, ctx);
    // printf("Result: %s\n", cel_value_to_string(result.data.value));

    return 0;
}
```

## Testing

### Run all tests
```bash
cd build
make test
```

### Run specific test
```bash
./tests/test_error
./tests/test_memory
```

### Memory leak detection (Valgrind)
```bash
# Ubuntu/Linux
valgrind --leak-check=full ./tests/test_error

# Or use the convenience script
cd build
make memcheck
```

### Test coverage (Debug build only)
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make coverage
```

## Project Structure

```
cel-c/
├── include/cel/          # Public header files
│   ├── cel.h            # Main API
│   ├── cel_error.h      # Error handling
│   ├── cel_memory.h     # Memory management
│   ├── cel_value.h      # Value types
│   ├── cel_context.h    # Execution context
│   └── ...
├── src/                 # Implementation files
│   ├── cel_error.c
│   ├── cel_memory.c
│   ├── parser/          # Lexer and parser
│   └── eval/            # Evaluator
├── tests/               # Unit tests
├── examples/            # Usage examples
├── bench/               # Benchmarks
├── third_party/         # Third-party libraries (auto-downloaded)
└── docs/                # Documentation
```

## Development Phases

- ✅ **Phase 1**: Infrastructure (build system, error handling, memory management, testing) - *IN PROGRESS*
- ⏳ **Phase 2**: Core data structures (value types, AST)
- ⏳ **Phase 3**: Parser (lexer, parser, macros)
- ⏳ **Phase 4**: Execution engine (evaluator, operators, functions)
- ⏳ **Phase 5**: Advanced features (optimization, additional testing)

See `specs/TASK-BREAKDOWN.md` for detailed task breakdown.

## Contributing

Contributions are welcome! Please read the development guide in `specs/`.

## License

Apache 2.0 License - see LICENSE file for details

## References

- [CEL Specification](https://github.com/google/cel-spec)
- [cel-rust](https://github.com/clarkmcc/cel-rust) - Original Rust implementation
- Design documents in `specs/` directory

## Acknowledgments

This project is a C port of cel-rust, based on the Common Expression Language specification by Google.

Third-party libraries:
- [uthash](https://troydhanson.github.io/uthash/) - Hash table implementation
- [SDS](https://github.com/antirez/sds) - Simple Dynamic Strings
- [Unity](https://github.com/ThrowTheSwitch/Unity) - Unit testing framework
- [PCRE2](https://www.pcre.org/) - Regular expression library (optional)
- [cJSON](https://github.com/DaveGamble/cJSON) - JSON parser (optional)
