# CEL-C API Reference

CEL-C 是 Common Expression Language (CEL) 的 C 语言实现。

## 快速开始

```c
#include "cel/cel_program.h"
#include "cel/cel_context.h"

int main(void) {
    // 创建上下文并添加变量
    cel_context_t *ctx = cel_context_create();
    cel_value_t age = cel_value_int(25);
    cel_context_add_variable(ctx, "age", &age);

    // 编译并执行表达式
    cel_execute_result_t result = cel_eval_expression("age >= 18", ctx);

    if (result.success && result.value.type == CEL_TYPE_BOOL) {
        printf("Is adult: %s\n", result.value.value.bool_value ? "yes" : "no");
    }

    // 清理
    cel_execute_result_destroy(&result);
    cel_context_destroy(ctx);
    return 0;
}
```

## 核心 API

### 编译和执行

#### cel_compile
```c
cel_compile_result_t cel_compile(const char *source);
```
编译 CEL 表达式为可执行程序。

#### cel_execute
```c
cel_execute_result_t cel_execute(const cel_program_t *program, cel_context_t *ctx);
```
执行编译后的程序。

#### cel_eval_expression
```c
cel_execute_result_t cel_eval_expression(const char *source, cel_context_t *ctx);
```
一步完成编译和执行（便捷 API）。

### 上下文管理

#### cel_context_create
```c
cel_context_t *cel_context_create(void);
```
创建新的执行上下文。

#### cel_context_add_variable
```c
bool cel_context_add_variable(cel_context_t *ctx, const char *name, const cel_value_t *value);
```
向上下文添加变量。

#### cel_context_destroy
```c
void cel_context_destroy(cel_context_t *ctx);
```
销毁上下文。

### 值类型

#### 基础类型创建
```c
cel_value_t cel_value_null(void);
cel_value_t cel_value_bool(bool value);
cel_value_t cel_value_int(int64_t value);
cel_value_t cel_value_uint(uint64_t value);
cel_value_t cel_value_double(double value);
cel_value_t cel_value_string(const char *value);
```

#### 容器类型
```c
cel_list_t *cel_list_create(size_t capacity);
void cel_list_append(cel_list_t *list, const cel_value_t *value);
cel_value_t *cel_list_get(const cel_list_t *list, size_t index);
size_t cel_list_size(const cel_list_t *list);

cel_map_t *cel_map_create(size_t capacity);
void cel_map_put(cel_map_t *map, const cel_value_t *key, const cel_value_t *value);
cel_value_t *cel_map_get(const cel_map_t *map, const cel_value_t *key);
```

#### 值销毁
```c
void cel_value_destroy(cel_value_t *value);
```

## 支持的表达式

### 运算符
- 算术: `+`, `-`, `*`, `/`, `%`
- 比较: `==`, `!=`, `<`, `<=`, `>`, `>=`
- 逻辑: `&&`, `||`, `!`
- 成员: `in`
- 条件: `? :`

### 内置函数
- `size(list/map/string)` - 获取大小
- `contains(string, substring)` - 字符串包含
- `startsWith(string, prefix)` - 字符串前缀
- `endsWith(string, suffix)` - 字符串后缀
- `matches(string, regex)` - 正则匹配 (需要 PCRE2)
- `int(value)`, `uint(value)`, `double(value)`, `string(value)` - 类型转换

### 宏
- `has(field)` - 字段存在检查
- `all(list, var, predicate)` - 全部满足
- `exists(list, var, predicate)` - 存在满足
- `exists_one(list, var, predicate)` - 恰好一个满足
- `map(list, var, transform)` - 映射转换
- `filter(list, var, predicate)` - 过滤

## 可选特性

### 时间类型 (CEL_ENABLE_CHRONO)
```c
cel_value_t cel_value_timestamp(int64_t seconds, int32_t nanos);
cel_value_t cel_value_duration(int64_t seconds, int32_t nanos);
```

### JSON 转换 (CEL_ENABLE_JSON)
```c
char *cel_value_to_json(const cel_value_t *value);
cel_value_t cel_value_from_json(const char *json);
```

### 正则表达式 (CEL_ENABLE_REGEX)
需要 PCRE2 库支持 `matches()` 函数。

## 构建选项

```bash
cmake .. \
    -DCEL_ENABLE_JSON=ON \
    -DCEL_ENABLE_REGEX=ON \
    -DCEL_ENABLE_CHRONO=ON \
    -DCEL_BUILD_TESTS=ON \
    -DCEL_BUILD_BENCH=ON
```

## 错误处理

```c
cel_compile_result_t result = cel_compile("invalid expression");
if (result.has_errors) {
    for (cel_parse_error_t *err = result.errors; err; err = err->next) {
        printf("Error at line %d: %s\n", err->line, err->message);
    }
}
cel_compile_result_destroy(&result);
```
