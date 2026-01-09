# CEL-C 快速入门

## 安装

```bash
git clone <repository>
cd cel-c
mkdir build && cd build
cmake .. -DCEL_ENABLE_JSON=ON
make -j4
sudo make install
```

## 基本用法

### 1. 简单表达式求值

```c
#include "cel/cel_program.h"

int main(void) {
    cel_context_t *ctx = cel_context_create();

    // 添加变量
    cel_value_t x = cel_value_int(10);
    cel_value_t y = cel_value_int(20);
    cel_context_add_variable(ctx, "x", &x);
    cel_context_add_variable(ctx, "y", &y);

    // 求值
    cel_execute_result_t result = cel_eval_expression("x + y", ctx);

    if (result.success) {
        printf("Result: %lld\n", result.value.value.int_value);
    }

    cel_execute_result_destroy(&result);
    cel_context_destroy(ctx);
    return 0;
}
```

### 2. 编译后重复执行

```c
// 编译一次
cel_compile_result_t compile_result = cel_compile("user.age >= 18");
if (compile_result.has_errors) {
    // 处理错误
    cel_compile_result_destroy(&compile_result);
    return 1;
}

// 多次执行
for (int i = 0; i < 1000; i++) {
    cel_context_t *ctx = cel_context_create();
    // 设置变量...

    cel_execute_result_t result = cel_execute(compile_result.program, ctx);
    // 处理结果...

    cel_execute_result_destroy(&result);
    cel_context_destroy(ctx);
}

cel_compile_result_destroy(&compile_result);
```

### 3. 使用列表和映射

```c
// 创建列表
cel_list_t *items = cel_list_create(3);
cel_value_t v1 = cel_value_int(1);
cel_value_t v2 = cel_value_int(2);
cel_value_t v3 = cel_value_int(3);
cel_list_append(items, &v1);
cel_list_append(items, &v2);
cel_list_append(items, &v3);

cel_value_t items_val = cel_value_list(items);
cel_context_add_variable(ctx, "items", &items_val);

// 使用列表
cel_eval_expression("items[0] + items[1]", ctx);  // 结果: 3
cel_eval_expression("size(items)", ctx);          // 结果: 3
cel_eval_expression("2 in items", ctx);           // 结果: true

cel_value_destroy(&items_val);
```

### 4. 使用宏

```c
// all - 检查所有元素
cel_eval_expression("items.all(x, x > 0)", ctx);  // true

// exists - 检查是否存在
cel_eval_expression("items.exists(x, x == 2)", ctx);  // true

// map - 转换元素
cel_eval_expression("items.map(x, x * 2)", ctx);  // [2, 4, 6]

// filter - 过滤元素
cel_eval_expression("items.filter(x, x > 1)", ctx);  // [2, 3]
```

### 5. 字符串操作

```c
cel_value_t name = cel_value_string("hello world");
cel_context_add_variable(ctx, "name", &name);

cel_eval_expression("name.startsWith(\"hello\")", ctx);  // true
cel_eval_expression("name.endsWith(\"world\")", ctx);    // true
cel_eval_expression("name.contains(\"lo wo\")", ctx);    // true
cel_eval_expression("size(name)", ctx);                  // 11

cel_value_destroy(&name);
```

## 编译链接

```bash
gcc -o myapp myapp.c -lcel -lm
```

或使用 pkg-config:

```bash
gcc -o myapp myapp.c $(pkg-config --cflags --libs cel)
```
