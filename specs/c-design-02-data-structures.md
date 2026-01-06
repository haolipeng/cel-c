# CEL-C 语言设计文档 (第 2 部分: 核心数据结构)

---

## 3. 核心数据结构设计

### 3.1 值表示 (cel_value)

#### 3.1.1 值类型定义

```c
// cel_value.h

#ifndef CEL_VALUE_H
#define CEL_VALUE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief CEL 值类型枚举
 */
typedef enum {
    CEL_TYPE_NULL = 0,
    CEL_TYPE_BOOL,
    CEL_TYPE_INT64,
    CEL_TYPE_UINT64,
    CEL_TYPE_DOUBLE,
    CEL_TYPE_STRING,
    CEL_TYPE_BYTES,
    CEL_TYPE_LIST,
    CEL_TYPE_MAP,
    CEL_TYPE_TIMESTAMP,      // 需要 chrono 特性
    CEL_TYPE_DURATION,       // 需要 chrono 特性
    CEL_TYPE_OPTIONAL,
    CEL_TYPE_OPAQUE          // 自定义类型
} cel_value_type_e;

/**
 * @brief 字符串结构 (基于 SDS 简化版)
 */
typedef struct {
    char* data;              // UTF-8 字符串数据 (null 结尾)
    size_t length;           // 字符串长度(不含 null)
    size_t capacity;         // 分配的容量
} cel_string_t;

/**
 * @brief 字节数组结构
 */
typedef struct {
    uint8_t* data;           // 原始字节
    size_t length;           // 字节数量
    size_t capacity;         // 分配的容量
} cel_bytes_t;

/**
 * @brief 列表结构
 */
typedef struct {
    struct cel_value** items; // 值指针数组
    size_t length;            // 元素数量
    size_t capacity;          // 分配的容量
} cel_list_t;

/**
 * @brief Map 键结构 (支持多种类型的键)
 */
typedef struct cel_map_key {
    cel_value_type_e type;    // 键类型
    union {
        int64_t int_key;
        uint64_t uint_key;
        bool bool_key;
        cel_string_t* string_key; // 引用,不拥有
    } data;
    struct cel_map_key* next; // uthash 链表指针
} cel_map_key_t;

/**
 * @brief Map 结构 (使用 uthash)
 */
typedef struct {
    cel_map_key_t* entries;   // uthash 哈希表
    size_t size;              // 键值对数量
} cel_map_t;

/**
 * @brief 时间戳结构 (RFC3339)
 */
typedef struct {
    int64_t seconds;          // 自 1970-01-01 的秒数
    int32_t nanoseconds;      // 纳秒部分 (0-999999999)
    int16_t offset_minutes;   // UTC 偏移(分钟)
} cel_timestamp_t;

/**
 * @brief 时长结构
 */
typedef struct {
    int64_t seconds;          // 秒数 (可为负)
    int32_t nanoseconds;      // 纳秒部分 (0-999999999)
} cel_duration_t;

/**
 * @brief Optional 结构
 */
typedef struct {
    bool has_value;           // 是否有值
    struct cel_value* value;  // 包含的值(may be NULL)
} cel_optional_t;

/**
 * @brief 不透明类型虚表
 */
typedef struct {
    const char* type_name;
    void (*destroy)(void* data);
    bool (*equals)(const void* a, const void* b);
    char* (*to_string)(const void* data);
    void* (*to_json)(const void* data); // 返回 cJSON* (可选)
} cel_opaque_vtable_t;

/**
 * @brief 不透明类型结构
 */
typedef struct {
    void* data;
    const cel_opaque_vtable_t* vtable;
} cel_opaque_t;

/**
 * @brief CEL 值结构 (核心)
 */
typedef struct cel_value {
    cel_value_type_e type;    // 值类型标签
    int ref_count;            // 引用计数 (原子操作或加锁)

    union {
        bool bool_val;
        int64_t int_val;
        uint64_t uint_val;
        double double_val;
        cel_string_t* string_val;
        cel_bytes_t* bytes_val;
        cel_list_t* list_val;
        cel_map_t* map_val;
        cel_timestamp_t* timestamp_val;
        cel_duration_t* duration_val;
        cel_optional_t* optional_val;
        cel_opaque_t* opaque_val;
    } data;
} cel_value_t;

/**
 * @brief 创建值的 API
 */
cel_value_t* cel_value_create_null(void);
cel_value_t* cel_value_create_bool(bool value);
cel_value_t* cel_value_create_int(int64_t value);
cel_value_t* cel_value_create_uint(uint64_t value);
cel_value_t* cel_value_create_double(double value);
cel_value_t* cel_value_create_string(const char* str, size_t len);
cel_value_t* cel_value_create_string_ref(cel_string_t* str); // 获取所有权
cel_value_t* cel_value_create_bytes(const uint8_t* data, size_t len);
cel_value_t* cel_value_create_list(size_t capacity);
cel_value_t* cel_value_create_map(void);
cel_value_t* cel_value_create_timestamp(int64_t sec, int32_t nsec, int16_t offset);
cel_value_t* cel_value_create_duration(int64_t sec, int32_t nsec);
cel_value_t* cel_value_create_optional(cel_value_t* value); // 转移所有权
cel_value_t* cel_value_create_optional_none(void);
cel_value_t* cel_value_create_opaque(void* data, const cel_opaque_vtable_t* vtable);

/**
 * @brief 引用计数 API
 */
void cel_value_retain(cel_value_t* value);    // 增加引用计数
void cel_value_release(cel_value_t* value);   // 减少引用计数,可能销毁

/**
 * @brief 值操作 API
 */
bool cel_value_equals(const cel_value_t* a, const cel_value_t* b);
int cel_value_compare(const cel_value_t* a, const cel_value_t* b); // <0, 0, >0
char* cel_value_to_string(const cel_value_t* value);

/**
 * @brief 容器操作 API
 */
// List 操作
cel_error_code_e cel_list_append(cel_list_t* list, cel_value_t* value);
cel_value_t* cel_list_get(const cel_list_t* list, size_t index);
size_t cel_list_size(const cel_list_t* list);

// Map 操作
cel_error_code_e cel_map_insert(cel_map_t* map, cel_value_t* key, cel_value_t* value);
cel_value_t* cel_map_get(const cel_map_t* map, const cel_value_t* key);
bool cel_map_contains(const cel_map_t* map, const cel_value_t* key);
size_t cel_map_size(const cel_map_t* map);

#endif // CEL_VALUE_H
```

#### 3.1.2 设计要点

**引用计数实现:**

```c
// cel_value.c

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

// 多线程环境使用互斥锁保护引用计数
#ifdef CEL_THREAD_SAFE
static pthread_mutex_t ref_count_mutex = PTHREAD_MUTEX_INITIALIZER;
#define REF_COUNT_LOCK() pthread_mutex_lock(&ref_count_mutex)
#define REF_COUNT_UNLOCK() pthread_mutex_unlock(&ref_count_mutex)
#else
#define REF_COUNT_LOCK()
#define REF_COUNT_UNLOCK()
#endif

void cel_value_retain(cel_value_t* value) {
    if (value == NULL) return;

    REF_COUNT_LOCK();
    value->ref_count++;
    REF_COUNT_UNLOCK();
}

void cel_value_release(cel_value_t* value) {
    if (value == NULL) return;

    REF_COUNT_LOCK();
    value->ref_count--;
    int count = value->ref_count;
    REF_COUNT_UNLOCK();

    if (count == 0) {
        cel_value_destroy(value); // 内部函数:销毁值
    }
}

static void cel_value_destroy(cel_value_t* value) {
    switch (value->type) {
        case CEL_TYPE_STRING:
            cel_string_destroy(value->data.string_val);
            break;
        case CEL_TYPE_BYTES:
            cel_bytes_destroy(value->data.bytes_val);
            break;
        case CEL_TYPE_LIST:
            cel_list_destroy(value->data.list_val);
            break;
        case CEL_TYPE_MAP:
            cel_map_destroy(value->data.map_val);
            break;
        case CEL_TYPE_OPTIONAL:
            if (value->data.optional_val->value) {
                cel_value_release(value->data.optional_val->value);
            }
            free(value->data.optional_val);
            break;
        case CEL_TYPE_OPAQUE:
            if (value->data.opaque_val->vtable->destroy) {
                value->data.opaque_val->vtable->destroy(
                    value->data.opaque_val->data
                );
            }
            free(value->data.opaque_val);
            break;
        // ... 其他类型
        default:
            break;
    }
    free(value);
}
```

**Copy-on-Write 优化:**

```c
// 列表追加时使用 COW
cel_error_code_e cel_list_append(cel_list_t* list, cel_value_t* value) {
    // 如果列表被共享(ref_count > 1),先复制
    if (list->ref_count > 1) {
        cel_list_t* new_list = cel_list_clone(list);
        // 更新引用...
    }

    // 扩容检查
    if (list->length >= list->capacity) {
        size_t new_cap = list->capacity * 2;
        cel_value_t** new_items = realloc(
            list->items,
            new_cap * sizeof(cel_value_t*)
        );
        if (!new_items) return CEL_ERR_NOMEM;
        list->items = new_items;
        list->capacity = new_cap;
    }

    list->items[list->length++] = value;
    cel_value_retain(value); // 增加引用
    return CEL_OK;
}
```

**字符串 Interning (可选优化):**

```c
// 字符串池减少重复字符串内存
typedef struct {
    cel_string_t* strings; // 哈希表存储
} cel_string_pool_t;

cel_string_t* cel_string_intern(cel_string_pool_t* pool, const char* str) {
    // 查找是否已存在
    cel_string_t* existing = string_pool_find(pool, str);
    if (existing) {
        return existing; // 返回已有字符串
    }

    // 创建新字符串并加入池
    cel_string_t* new_str = cel_string_create(str);
    string_pool_insert(pool, new_str);
    return new_str;
}
```

### 3.2 AST 表示 (cel_ast)

#### 3.2.1 AST 节点定义

```c
// cel_ast.h

#ifndef CEL_AST_H
#define CEL_AST_H

#include "cel_value.h"

/**
 * @brief AST 节点类型
 */
typedef enum {
    AST_LITERAL,          // 字面量
    AST_IDENT,            // 变量引用
    AST_CALL,             // 函数调用 (包括运算符)
    AST_SELECT,           // 字段选择 (obj.field)
    AST_INDEX,            // 索引访问 (obj[key])
    AST_CONDITIONAL,      // 三元条件 (cond ? a : b)
    AST_LIST,             // 列表字面量
    AST_MAP,              // Map 字面量
    AST_STRUCT,           // 结构体字面量
    AST_COMPREHENSION     // 推导式 (宏展开后)
} ast_node_type_e;

/**
 * @brief AST 节点通用结构
 */
typedef struct ast_node {
    ast_node_type_e type;      // 节点类型
    uint64_t id;               // 唯一 ID (用于源位置追踪)

    union {
        // AST_LITERAL
        struct {
            cel_value_t* value;
        } literal;

        // AST_IDENT
        struct {
            char* name;
        } ident;

        // AST_CALL
        struct {
            char* func_name;
            struct ast_node* target; // NULL 表示函数调用,非 NULL 表示方法调用
            struct ast_node** args;
            size_t arg_count;
        } call;

        // AST_SELECT
        struct {
            struct ast_node* operand;
            char* field;
            bool optional;           // 是否是 .? 可选选择
        } select;

        // AST_INDEX
        struct {
            struct ast_node* operand;
            struct ast_node* index;
            bool optional;           // 是否是 [?] 可选索引
        } index;

        // AST_CONDITIONAL
        struct {
            struct ast_node* condition;
            struct ast_node* true_expr;
            struct ast_node* false_expr;
        } conditional;

        // AST_LIST
        struct {
            struct ast_node** elements;
            bool* optional_flags;    // 每个元素是否可选
            size_t element_count;
        } list;

        // AST_MAP
        struct {
            struct ast_node** keys;
            struct ast_node** values;
            bool* optional_flags;    // 每个键值对是否可选
            size_t entry_count;
        } map;

        // AST_STRUCT
        struct {
            char* type_name;         // 结构体类型名
            char** field_names;
            struct ast_node** field_values;
            bool* optional_flags;
            size_t field_count;
        } struct_node;

        // AST_COMPREHENSION
        struct {
            struct ast_node* iter_range;    // 迭代的集合
            char* iter_var;                 // 循环变量名
            char* accu_var;                 // 累加器变量名
            struct ast_node* accu_init;     // 累加器初始值
            struct ast_node* loop_cond;     // 循环条件
            struct ast_node* loop_step;     // 循环步骤
            struct ast_node* result;        // 最终结果表达式
        } comprehension;

    } data;
} ast_node_t;

/**
 * @brief AST 创建 API
 */
ast_node_t* ast_create_literal(cel_value_t* value);
ast_node_t* ast_create_ident(const char* name);
ast_node_t* ast_create_call(const char* func_name, ast_node_t* target,
                             ast_node_t** args, size_t arg_count);
ast_node_t* ast_create_select(ast_node_t* operand, const char* field, bool optional);
ast_node_t* ast_create_index(ast_node_t* operand, ast_node_t* index, bool optional);
ast_node_t* ast_create_conditional(ast_node_t* cond, ast_node_t* true_expr,
                                    ast_node_t* false_expr);
ast_node_t* ast_create_list(ast_node_t** elements, bool* optional_flags,
                             size_t count);
ast_node_t* ast_create_map(ast_node_t** keys, ast_node_t** values,
                            bool* optional_flags, size_t count);
ast_node_t* ast_create_comprehension(/* 参数省略 */);

/**
 * @brief AST 销毁 API
 */
void ast_node_destroy(ast_node_t* node);

/**
 * @brief AST 遍历 API (访问者模式)
 */
typedef void (*ast_visitor_fn)(ast_node_t* node, void* user_data);
void ast_traverse(ast_node_t* root, ast_visitor_fn visitor, void* user_data);

#endif // CEL_AST_H
```

#### 3.2.2 AST 内存管理

**Arena 分配器优化:**

```c
// cel_memory.h

/**
 * @brief Arena 分配器 (内存池)
 */
typedef struct {
    uint8_t* buffer;
    size_t offset;
    size_t capacity;
    struct arena* next;  // 链表
} arena_t;

arena_t* arena_create(size_t capacity);
void* arena_alloc(arena_t* arena, size_t size);
void arena_destroy(arena_t* arena);

/**
 * @brief AST 上下文 (管理所有 AST 节点)
 */
typedef struct {
    arena_t* arena;           // AST 节点分配在 arena 中
    ast_node_t** nodes;       // 节点数组(用于快速释放)
    size_t node_count;
} ast_context_t;

ast_context_t* ast_context_create(void);
ast_node_t* ast_context_alloc_node(ast_context_t* ctx);
void ast_context_destroy(ast_context_t* ctx); // 一次性释放所有节点
```

**使用示例:**

```c
// 解析器中使用 arena
ast_context_t* ctx = ast_context_create();

// 创建 AST 节点 (从 arena 分配)
ast_node_t* node1 = ast_context_alloc_node(ctx);
node1->type = AST_LITERAL;
// ...

// 解析完成后一次性释放
ast_context_destroy(ctx); // 释放所有 AST 节点
```

### 3.3 执行上下文 (cel_context)

#### 3.3.1 上下文结构定义

```c
// cel_context.h

#ifndef CEL_CONTEXT_H
#define CEL_CONTEXT_H

#include "cel_value.h"
#include "cel_error.h"
#include "uthash.h"

/**
 * @brief 函数指针类型
 */
typedef cel_result_t (*cel_function_fn)(
    struct cel_func_context* ctx,
    cel_value_t** args,
    size_t arg_count
);

/**
 * @brief 函数元数据
 */
typedef struct {
    char* name;                  // 函数名
    cel_function_fn func;        // 函数指针
    size_t min_args;             // 最少参数数量
    size_t max_args;             // 最多参数数量 (SIZE_MAX = 可变参数)
    cel_value_type_e* arg_types; // 参数类型(可选,NULL = 不检查)
    cel_value_type_e return_type;// 返回类型
    void* user_data;             // 用户数据
} cel_function_t;

/**
 * @brief 函数注册表项 (uthash)
 */
typedef struct {
    char* name;                  // 键 (函数名)
    cel_function_t* func;        // 值 (函数元数据)
    UT_hash_handle hh;           // uthash 句柄
} cel_function_entry_t;

/**
 * @brief 变量表项 (uthash)
 */
typedef struct {
    char* name;                  // 键 (变量名)
    cel_value_t* value;          // 值
    UT_hash_handle hh;
} cel_variable_entry_t;

/**
 * @brief 变量解析器接口
 */
typedef cel_value_t* (*cel_var_resolver_fn)(const char* name, void* user_data);

/**
 * @brief 执行上下文结构
 */
typedef struct cel_context {
    struct cel_context* parent;  // 父上下文 (作用域链)

    // 变量表
    cel_variable_entry_t* variables; // uthash 哈希表

    // 函数注册表
    cel_function_entry_t* functions; // uthash 哈希表

    // 自定义变量解析器
    cel_var_resolver_fn resolver;
    void* resolver_user_data;

    // 配置
    size_t max_recursion_depth;  // 最大递归深度
    size_t current_depth;        // 当前递归深度
} cel_context_t;

/**
 * @brief 函数执行上下文 (传递给函数实现)
 */
typedef struct cel_func_context {
    cel_context_t* context;      // 执行上下文
    const char* func_name;       // 当前函数名
    ast_node_t* call_site;       // 调用点 AST 节点 (用于错误报告)
} cel_func_context_t;

/**
 * @brief 上下文 API
 */
cel_context_t* cel_context_create(void);
cel_context_t* cel_context_create_empty(void); // 无内置函数
cel_context_t* cel_context_create_child(cel_context_t* parent);
void cel_context_destroy(cel_context_t* ctx);

/**
 * @brief 变量操作
 */
cel_error_code_e cel_context_add_variable(cel_context_t* ctx,
                                           const char* name,
                                           cel_value_t* value);
cel_value_t* cel_context_get_variable(cel_context_t* ctx, const char* name);
bool cel_context_has_variable(cel_context_t* ctx, const char* name);

/**
 * @brief 函数操作
 */
cel_error_code_e cel_context_add_function(cel_context_t* ctx,
                                           const char* name,
                                           cel_function_fn func,
                                           size_t min_args,
                                           size_t max_args);
cel_function_t* cel_context_get_function(cel_context_t* ctx, const char* name);
bool cel_context_has_function(cel_context_t* ctx, const char* name);

/**
 * @brief 变量解析器
 */
void cel_context_set_resolver(cel_context_t* ctx,
                               cel_var_resolver_fn resolver,
                               void* user_data);

#endif // CEL_CONTEXT_H
```

#### 3.3.2 上下文实现要点

**变量查找 (支持作用域链):**

```c
// cel_context.c

cel_value_t* cel_context_get_variable(cel_context_t* ctx, const char* name) {
    cel_context_t* current = ctx;

    while (current) {
        // 在当前上下文查找
        cel_variable_entry_t* entry = NULL;
        HASH_FIND_STR(current->variables, name, entry);

        if (entry) {
            return entry->value; // 找到
        }

        // 尝试自定义解析器
        if (current->resolver) {
            cel_value_t* resolved = current->resolver(
                name,
                current->resolver_user_data
            );
            if (resolved) {
                return resolved;
            }
        }

        // 向父上下文查找
        current = current->parent;
    }

    return NULL; // 未找到
}
```

**函数注册 (带默认函数):**

```c
cel_context_t* cel_context_create(void) {
    cel_context_t* ctx = calloc(1, sizeof(cel_context_t));
    if (!ctx) return NULL;

    ctx->parent = NULL;
    ctx->variables = NULL;
    ctx->functions = NULL;
    ctx->max_recursion_depth = 96;
    ctx->current_depth = 0;

    // 注册内置函数
    cel_register_builtin_functions(ctx);

    return ctx;
}

void cel_register_builtin_functions(cel_context_t* ctx) {
    // 集合操作
    cel_context_add_function(ctx, "size", cel_func_size, 1, 1);
    cel_context_add_function(ctx, "contains", cel_func_contains, 2, 2);

    // 字符串操作
    cel_context_add_function(ctx, "startsWith", cel_func_starts_with, 2, 2);
    cel_context_add_function(ctx, "endsWith", cel_func_ends_with, 2, 2);
    cel_context_add_function(ctx, "matches", cel_func_matches, 2, 2);

    // 类型转换
    cel_context_add_function(ctx, "int", cel_func_int, 1, 1);
    cel_context_add_function(ctx, "uint", cel_func_uint, 1, 1);
    cel_context_add_function(ctx, "double", cel_func_double, 1, 1);
    cel_context_add_function(ctx, "string", cel_func_string, 1, 1);
    cel_context_add_function(ctx, "bytes", cel_func_bytes, 1, 1);

    // 聚合函数
    cel_context_add_function(ctx, "max", cel_func_max, 1, SIZE_MAX);
    cel_context_add_function(ctx, "min", cel_func_min, 1, SIZE_MAX);

    // Optional 操作
    cel_context_add_function(ctx, "optional.none", cel_func_optional_none, 0, 0);
    cel_context_add_function(ctx, "optional.of", cel_func_optional_of, 1, 1);
    cel_context_add_function(ctx, "hasValue", cel_func_has_value, 1, 1);
    cel_context_add_function(ctx, "value", cel_func_value, 1, 1);
    cel_context_add_function(ctx, "or", cel_func_or, 2, 2);
    cel_context_add_function(ctx, "orValue", cel_func_or_value, 2, 2);

    // 时间函数 (如果启用 chrono 特性)
#ifdef CEL_ENABLE_CHRONO
    cel_context_add_function(ctx, "timestamp", cel_func_timestamp, 1, 1);
    cel_context_add_function(ctx, "duration", cel_func_duration, 1, 1);
    cel_context_add_function(ctx, "getFullYear", cel_func_get_full_year, 1, 1);
    // ... 更多时间函数
#endif
}
```

### 3.4 函数接口 (cel_functions)

#### 3.4.1 函数实现模式

```c
// cel_functions.c

/**
 * @brief size() 函数实现
 */
cel_result_t cel_func_size(cel_func_context_t* ctx,
                            cel_value_t** args,
                            size_t arg_count) {
    // 参数检查
    if (arg_count != 1) {
        return cel_error_result(CEL_ERR_INVALID_ARG_COUNT,
                                "size() requires 1 argument");
    }

    cel_value_t* arg = args[0];
    int64_t size = 0;

    switch (arg->type) {
        case CEL_TYPE_STRING:
            size = arg->data.string_val->length;
            break;
        case CEL_TYPE_BYTES:
            size = arg->data.bytes_val->length;
            break;
        case CEL_TYPE_LIST:
            size = arg->data.list_val->length;
            break;
        case CEL_TYPE_MAP:
            size = arg->data.map_val->size;
            break;
        default:
            return cel_error_result(CEL_ERR_TYPE,
                                    "size() requires string, bytes, list or map");
    }

    return cel_ok_result(cel_value_create_int(size));
}

/**
 * @brief startsWith() 函数实现
 */
cel_result_t cel_func_starts_with(cel_func_context_t* ctx,
                                   cel_value_t** args,
                                   size_t arg_count) {
    if (arg_count != 2) {
        return cel_error_result(CEL_ERR_INVALID_ARG_COUNT,
                                "startsWith() requires 2 arguments");
    }

    cel_value_t* str = args[0];
    cel_value_t* prefix = args[1];

    if (str->type != CEL_TYPE_STRING || prefix->type != CEL_TYPE_STRING) {
        return cel_error_result(CEL_ERR_TYPE,
                                "startsWith() requires two strings");
    }

    cel_string_t* s = str->data.string_val;
    cel_string_t* p = prefix->data.string_val;

    // 比较
    bool result = false;
    if (s->length >= p->length) {
        result = (memcmp(s->data, p->data, p->length) == 0);
    }

    return cel_ok_result(cel_value_create_bool(result));
}

/**
 * @brief max() 函数实现 (可变参数)
 */
cel_result_t cel_func_max(cel_func_context_t* ctx,
                           cel_value_t** args,
                           size_t arg_count) {
    if (arg_count == 0) {
        return cel_error_result(CEL_ERR_INVALID_ARG_COUNT,
                                "max() requires at least 1 argument");
    }

    cel_value_t* max_val = args[0];

    for (size_t i = 1; i < arg_count; i++) {
        if (cel_value_compare(args[i], max_val) > 0) {
            max_val = args[i];
        }
    }

    cel_value_retain(max_val); // 增加引用计数
    return cel_ok_result(max_val);
}
```

### 3.5 错误处理 (cel_error)

#### 3.5.1 错误结构定义

```c
// cel_error.h

#ifndef CEL_ERROR_H
#define CEL_ERROR_H

#include <stddef.h>

/**
 * @brief 错误码枚举
 */
typedef enum {
    CEL_OK = 0,                     // 成功
    CEL_ERR_PARSE,                  // 解析错误
    CEL_ERR_EVAL,                   // 求值错误
    CEL_ERR_TYPE,                   // 类型错误
    CEL_ERR_OVERFLOW,               // 整数溢出
    CEL_ERR_DIV_ZERO,               // 除零错误
    CEL_ERR_INDEX_OUT_OF_BOUNDS,    // 索引越界
    CEL_ERR_NO_SUCH_KEY,            // Map 键不存在
    CEL_ERR_UNDEFINED_VAR,          // 未定义变量
    CEL_ERR_UNDEFINED_FUNC,         // 未定义函数
    CEL_ERR_INVALID_ARG_COUNT,      // 参数数量错误
    CEL_ERR_NO_SUCH_OVERLOAD,       // 函数重载不匹配
    CEL_ERR_FUNC_ERROR,             // 函数执行错误
    CEL_ERR_NOMEM,                  // 内存不足
    CEL_ERR_MAX_RECURSION           // 超过最大递归深度
} cel_error_code_e;

/**
 * @brief 错误结构
 */
typedef struct {
    cel_error_code_e code;          // 错误码
    char* message;                  // 错误消息
    uint64_t source_position;       // 源代码位置
    char* source_snippet;           // 源代码片段 (可选)
} cel_error_t;

/**
 * @brief 结果类型 (类似 Rust 的 Result<T, E>)
 */
typedef struct {
    bool success;                   // 是否成功
    union {
        cel_value_t* value;         // 成功时的值
        cel_error_t* error;         // 失败时的错误
    } data;
} cel_result_t;

/**
 * @brief 错误 API
 */
cel_error_t* cel_error_create(cel_error_code_e code, const char* message);
void cel_error_destroy(cel_error_t* error);
const char* cel_error_code_string(cel_error_code_e code);

/**
 * @brief 结果 API
 */
cel_result_t cel_ok_result(cel_value_t* value);
cel_result_t cel_error_result(cel_error_code_e code, const char* message);
void cel_result_destroy(cel_result_t* result);

/**
 * @brief 宏: 错误传播
 */
#define CEL_TRY(expr) \
    do { \
        cel_result_t __result = (expr); \
        if (!__result.success) { \
            return __result; \
        } \
    } while (0)

#define CEL_UNWRAP(result, var) \
    do { \
        if (!(result).success) { \
            return (result); \
        } \
        var = (result).data.value; \
    } while (0)

#endif // CEL_ERROR_H
```

#### 3.5.2 错误使用示例

```c
// 在求值器中使用
cel_result_t cel_eval_binary_op(ast_node_t* node, cel_context_t* ctx) {
    // 求值左操作数
    cel_result_t left_result = cel_eval_node(node->data.call.args[0], ctx);
    CEL_TRY(left_result); // 如果失败,直接返回错误
    cel_value_t* left = left_result.data.value;

    // 求值右操作数
    cel_result_t right_result = cel_eval_node(node->data.call.args[1], ctx);
    if (!right_result.success) {
        cel_value_release(left);
        return right_result; // 传播错误
    }
    cel_value_t* right = right_result.data.value;

    // 执行运算
    cel_result_t result = cel_exec_add(left, right);

    cel_value_release(left);
    cel_value_release(right);

    return result;
}
```

---

## 总结

本部分详细设计了 CEL-C 的核心数据结构:

1. **cel_value**: 值类型系统,使用 tagged union + 引用计数
2. **cel_ast**: AST 节点表示,使用 Arena 分配器优化内存
3. **cel_context**: 执行上下文,使用 uthash 管理变量和函数
4. **cel_functions**: 函数接口设计,统一的函数指针类型
5. **cel_error**: 错误处理,Result-like 模式

下一部分将介绍解析器设计、求值算法、API 设计和实现建议。
