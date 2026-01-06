# CEL-C 语言设计文档 (第 3 部分: 算法、API 与实现)

---

## 4. 关键算法设计

### 4.1 解析流程设计

#### 4.1.1 推荐工具链

**词法分析器: re2c**

**优势:**
- 生成高效的 C 代码
- 无运行时依赖
- 速度快于 Flex
- 支持Unicode
-生成的代码可直接集成

**语法分析器: Lemon**

**优势:**
- SQLite 使用,久经考验
- 线程安全(无全局状态)
- LALR(1) 解析器
- 错误恢复机制
- 生成的代码简洁

**备选方案: 手写递归下降**

如果希望完全控制解析过程,可手写递归下降解析器。

#### 4.1.2 词法分析器示例 (re2c)

```c
// lexer.re

#include "tokens.h"

/*!re2c
    re2c:define:YYCTYPE = "unsigned char";
    re2c:yyfill:enable = 0;

    // 空白字符
    WS      = [ \t\r\n]+;

    // 标识符和关键字
    ID      = [a-zA-Z_][a-zA-Z0-9_]*;

    // 整数字面量
    INT     = [0-9]+ | "0x"[0-9a-fA-F]+;
    UINT    = INT "u";

    // 浮点数字面量
    FLOAT   = [0-9]+ "." [0-9]* | [0-9]* "." [0-9]+;
    EXP     = [eE][+-]?[0-9]+;
    DOUBLE  = (FLOAT | [0-9]+) EXP?;

    // 字符串字面量
    STRING  = ["] ([^"\\] | "\\" [^\n])* ["];

    // 字节字面量
    BYTES   = "b" STRING;
*/

token_t* lex_next_token(lexer_state_t* state) {
    const unsigned char* YYCURSOR = (const unsigned char*)state->cursor;
    const unsigned char* YYMARKER;

loop:
    /*!re2c
        WS      { goto loop; }
        "//"    { skip_line_comment(state); goto loop; }

        // 关键字
        "true"  { return make_token(TOKEN_TRUE, state); }
        "false" { return make_token(TOKEN_FALSE, state); }
        "null"  { return make_token(TOKEN_NULL, state); }

        // 运算符
        "+"     { return make_token(TOKEN_PLUS, state); }
        "-"     { return make_token(TOKEN_MINUS, state); }
        "*"     { return make_token(TOKEN_STAR, state); }
        "/"     { return make_token(TOKEN_SLASH, state); }
        "%"     { return make_token(TOKEN_PERCENT, state); }
        "=="    { return make_token(TOKEN_EQUAL_EQUAL, state); }
        "!="    { return make_token(TOKEN_BANG_EQUAL, state); }
        "<"     { return make_token(TOKEN_LESS, state); }
        "<="    { return make_token(TOKEN_LESS_EQUAL, state); }
        ">"     { return make_token(TOKEN_GREATER, state); }
        ">="    { return make_token(TOKEN_GREATER_EQUAL, state); }
        "&&"    { return make_token(TOKEN_AND_AND, state); }
        "||"    { return make_token(TOKEN_OR_OR, state); }
        "!"     { return make_token(TOKEN_BANG, state); }
        "?"     { return make_token(TOKEN_QUESTION, state); }
        ":"     { return make_token(TOKEN_COLON, state); }
        "."     { return make_token(TOKEN_DOT, state); }
        ".?"    { return make_token(TOKEN_DOT_QUESTION, state); }
        "["     { return make_token(TOKEN_LBRACKET, state); }
        "]"     { return make_token(TOKEN_RBRACKET, state); }
        "[?"    { return make_token(TOKEN_LBRACKET_QUESTION, state); }
        "("     { return make_token(TOKEN_LPAREN, state); }
        ")"     { return make_token(TOKEN_RPAREN, state); }
        "{"     { return make_token(TOKEN_LBRACE, state); }
        "}"     { return make_token(TOKEN_RBRACE, state); }
        ","     { return make_token(TOKEN_COMMA, state); }

        // 字面量
        INT     { return make_int_token(state); }
        UINT    { return make_uint_token(state); }
        DOUBLE  { return make_double_token(state); }
        STRING  { return make_string_token(state); }
        BYTES   { return make_bytes_token(state); }

        // 标识符
        ID      { return make_id_token(state); }

        // 文件结束
        "\x00"  { return make_token(TOKEN_EOF, state); }

        // 错误
        *       { return make_error_token("Unexpected character", state); }
    */
}
```

#### 4.1.3 语法分析器示例 (Lemon)

```c
// parser.y

%include {
#include "ast.h"
#include "tokens.h"
}

%token_type {token_t*}
%default_type {ast_node_t*}

%type expr {ast_node_t*}
%type primary {ast_node_t*}

// 运算符优先级
%left OR_OR.
%left AND_AND.
%left EQUAL_EQUAL BANG_EQUAL.
%left LESS LESS_EQUAL GREATER GREATER_EQUAL.
%left PLUS MINUS.
%left STAR SLASH PERCENT.
%right BANG UMINUS.
%left DOT LBRACKET LPAREN.

// 起始符号
%start_symbol program

program ::= expr(E). {
    parser_set_result(parser, E);
}

// 表达式
expr(R) ::= primary(P). {
    R = P;
}

expr(R) ::= expr(L) PLUS expr(R). {
    R = ast_create_call("+", NULL, (ast_node_t*[]){L, R}, 2);
}

expr(R) ::= expr(L) MINUS expr(R). {
    R = ast_create_call("-", NULL, (ast_node_t*[]){L, R}, 2);
}

expr(R) ::= expr(L) STAR expr(R). {
    R = ast_create_call("*", NULL, (ast_node_t*[]){L, R}, 2);
}

expr(R) ::= expr(L) SLASH expr(R). {
    R = ast_create_call("/", NULL, (ast_node_t*[]){L, R}, 2);
}

expr(R) ::= expr(COND) QUESTION expr(TRUE) COLON expr(FALSE). {
    R = ast_create_conditional(COND, TRUE, FALSE);
}

expr(R) ::= expr(OBJ) DOT ID(FIELD). {
    R = ast_create_select(OBJ, FIELD->text, false);
}

expr(R) ::= expr(OBJ) LBRACKET expr(IDX) RBRACKET. {
    R = ast_create_index(OBJ, IDX, false);
}

expr(R) ::= expr(FUNC) LPAREN arg_list(ARGS) RPAREN. {
    R = ast_create_call(FUNC->name, NULL, ARGS->args, ARGS->count);
}

// 基本表达式
primary(R) ::= INT(I). {
    R = ast_create_literal(cel_value_create_int(I->int_val));
}

primary(R) ::= UINT(U). {
    R = ast_create_literal(cel_value_create_uint(U->uint_val));
}

primary(R) ::= DOUBLE(D). {
    R = ast_create_literal(cel_value_create_double(D->double_val));
}

primary(R) ::= STRING(S). {
    R = ast_create_literal(cel_value_create_string(S->text, S->length));
}

primary(R) ::= TRUE. {
    R = ast_create_literal(cel_value_create_bool(true));
}

primary(R) ::= FALSE. {
    R = ast_create_literal(cel_value_create_bool(false));
}

primary(R) ::= NULL. {
    R = ast_create_literal(cel_value_create_null());
}

primary(R) ::= ID(I). {
    R = ast_create_ident(I->text);
}

primary(R) ::= LPAREN expr(E) RPAREN. {
    R = E;
}

// 列表字面量
primary(R) ::= LBRACKET elem_list(ELEMS) RBRACKET. {
    R = ast_create_list(ELEMS->nodes, ELEMS->optional_flags, ELEMS->count);
}

// Map 字面量
primary(R) ::= LBRACE entry_list(ENTRIES) RBRACE. {
    R = ast_create_map(ENTRIES->keys, ENTRIES->values, ENTRIES->optional_flags, ENTRIES->count);
}
```

#### 4.1.4 宏展开算法

```c
// cel_macros.c

/**
 * @brief has(obj.field) 宏展开
 * 展开为: "field" in obj
 */
ast_node_t* expand_has_macro(ast_node_t* select_node) {
    if (select_node->type != AST_SELECT) {
        return NULL; // 不是 select 节点
    }

    // 创建 "field" 字面量
    cel_value_t* field_str = cel_value_create_string(
        select_node->data.select.field,
        strlen(select_node->data.select.field)
    );
    ast_node_t* field_lit = ast_create_literal(field_str);

    // 创建 in 调用: @in(field, obj)
    ast_node_t* args[] = {field_lit, select_node->data.select.operand};
    ast_node_t* in_call = ast_create_call("@in", NULL, args, 2);

    return in_call;
}

/**
 * @brief list.all(x, predicate) 宏展开
 * 展开为 Comprehension:
 *   iter_range = list
 *   iter_var = "x"
 *   accu_var = "__accu"
 *   accu_init = true
 *   loop_cond = accu && iter_range.has_next()
 *   loop_step = accu && predicate(x)
 *   result = accu
 */
ast_node_t* expand_all_macro(ast_node_t* list, const char* var, ast_node_t* predicate) {
    ast_node_t* comp = ast_create_comprehension();

    comp->data.comprehension.iter_range = list;
    comp->data.comprehension.iter_var = strdup(var);
    comp->data.comprehension.accu_var = strdup("__result__");

    // accu_init = true
    comp->data.comprehension.accu_init = ast_create_literal(cel_value_create_bool(true));

    // loop_cond = __result__
    comp->data.comprehension.loop_cond = ast_create_ident("__result__");

    // loop_step = __result__ && predicate
    ast_node_t* accu_ident = ast_create_ident("__result__");
    ast_node_t* step_args[] = {accu_ident, predicate};
    comp->data.comprehension.loop_step = ast_create_call("_&&_", NULL, step_args, 2);

    // result = __result__
    comp->data.comprehension.result = ast_create_ident("__result__");

    return comp;
}

/**
 * @brief list.map(x, transform) 宏展开
 */
ast_node_t* expand_map_macro(ast_node_t* list, const char* var, ast_node_t* transform) {
    ast_node_t* comp = ast_create_comprehension();

    comp->data.comprehension.iter_range = list;
    comp->data.comprehension.iter_var = strdup(var);
    comp->data.comprehension.accu_var = strdup("__result__");

    // accu_init = []
    comp->data.comprehension.accu_init = ast_create_list(NULL, NULL, 0);

    // loop_cond = true (总是继续)
    comp->data.comprehension.loop_cond = ast_create_literal(cel_value_create_bool(true));

    // loop_step = __result__ + [transform]
    ast_node_t* accu = ast_create_ident("__result__");
    ast_node_t* elems[] = {transform};
    ast_node_t* list_lit = ast_create_list(elems, NULL, 1);
    ast_node_t* append_args[] = {accu, list_lit};
    comp->data.comprehension.loop_step = ast_create_call("_+_", NULL, append_args, 2);

    // result = __result__
    comp->data.comprehension.result = ast_create_ident("__result__");

    return comp;
}
```

### 4.2 求值算法设计

#### 4.2.1 核心求值函数

```c
// cel_eval.c

/**
 * @brief 求值 AST 节点
 */
cel_result_t cel_eval_node(ast_node_t* node, cel_context_t* ctx) {
    // 递归深度检查
    if (ctx->current_depth >= ctx->max_recursion_depth) {
        return cel_error_result(CEL_ERR_MAX_RECURSION,
                                "Maximum recursion depth exceeded");
    }
    ctx->current_depth++;

    cel_result_t result;

    switch (node->type) {
        case AST_LITERAL:
            result = cel_eval_literal(node, ctx);
            break;
        case AST_IDENT:
            result = cel_eval_ident(node, ctx);
            break;
        case AST_CALL:
            result = cel_eval_call(node, ctx);
            break;
        case AST_SELECT:
            result = cel_eval_select(node, ctx);
            break;
        case AST_INDEX:
            result = cel_eval_index(node, ctx);
            break;
        case AST_CONDITIONAL:
            result = cel_eval_conditional(node, ctx);
            break;
        case AST_LIST:
            result = cel_eval_list(node, ctx);
            break;
        case AST_MAP:
            result = cel_eval_map(node, ctx);
            break;
        case AST_COMPREHENSION:
            result = cel_eval_comprehension(node, ctx);
            break;
        default:
            result = cel_error_result(CEL_ERR_EVAL, "Unknown AST node type");
            break;
    }

    ctx->current_depth--;
    return result;
}

/**
 * @brief 求值字面量
 */
static cel_result_t cel_eval_literal(ast_node_t* node, cel_context_t* ctx) {
    cel_value_t* value = node->data.literal.value;
    cel_value_retain(value);
    return cel_ok_result(value);
}

/**
 * @brief 求值变量引用
 */
static cel_result_t cel_eval_ident(ast_node_t* node, cel_context_t* ctx) {
    const char* name = node->data.ident.name;
    cel_value_t* value = cel_context_get_variable(ctx, name);

    if (!value) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Undefined variable: %s", name);
        return cel_error_result(CEL_ERR_UNDEFINED_VAR, msg);
    }

    cel_value_retain(value);
    return cel_ok_result(value);
}

/**
 * @brief 求值函数调用 (包括运算符)
 */
static cel_result_t cel_eval_call(ast_node_t* node, cel_context_t* ctx) {
    const char* func_name = node->data.call.func_name;

    // 短路求值优化
    if (strcmp(func_name, "_&&_") == 0) {
        return cel_eval_logical_and(node, ctx);
    } else if (strcmp(func_name, "_||_") == 0) {
        return cel_eval_logical_or(node, ctx);
    }

    // 求值所有参数
    size_t arg_count = node->data.call.arg_count;
    cel_value_t** args = malloc(arg_count * sizeof(cel_value_t*));
    if (!args) {
        return cel_error_result(CEL_ERR_NOMEM, "Out of memory");
    }

    for (size_t i = 0; i < arg_count; i++) {
        cel_result_t arg_result = cel_eval_node(node->data.call.args[i], ctx);
        if (!arg_result.success) {
            // 释放已求值的参数
            for (size_t j = 0; j < i; j++) {
                cel_value_release(args[j]);
            }
            free(args);
            return arg_result;
        }
        args[i] = arg_result.data.value;
    }

    // 查找函数
    cel_function_t* func = cel_context_get_function(ctx, func_name);
    if (!func) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Undefined function: %s", func_name);
        for (size_t i = 0; i < arg_count; i++) {
            cel_value_release(args[i]);
        }
        free(args);
        return cel_error_result(CEL_ERR_UNDEFINED_FUNC, msg);
    }

    // 调用函数
    cel_func_context_t func_ctx = {
        .context = ctx,
        .func_name = func_name,
        .call_site = node
    };

    cel_result_t result = func->func(&func_ctx, args, arg_count);

    // 释放参数
    for (size_t i = 0; i < arg_count; i++) {
        cel_value_release(args[i]);
    }
    free(args);

    return result;
}

/**
 * @brief 短路求值: &&
 */
static cel_result_t cel_eval_logical_and(ast_node_t* node, cel_context_t* ctx) {
    // 求值左侧
    cel_result_t left_result = cel_eval_node(node->data.call.args[0], ctx);
    if (!left_result.success) {
        return left_result;
    }

    cel_value_t* left = left_result.data.value;

    // 检查左侧类型
    if (left->type != CEL_TYPE_BOOL) {
        cel_value_release(left);
        return cel_error_result(CEL_ERR_TYPE, "Logical AND requires boolean operands");
    }

    // 短路: 如果左侧为 false,不求值右侧
    if (!left->data.bool_val) {
        return cel_ok_result(left); // 直接返回 false
    }

    cel_value_release(left);

    // 求值右侧
    cel_result_t right_result = cel_eval_node(node->data.call.args[1], ctx);
    if (!right_result.success) {
        return right_result;
    }

    cel_value_t* right = right_result.data.value;

    if (right->type != CEL_TYPE_BOOL) {
        cel_value_release(right);
        return cel_error_result(CEL_ERR_TYPE, "Logical AND requires boolean operands");
    }

    return cel_ok_result(right);
}

/**
 * @brief 短路求值: ||
 */
static cel_result_t cel_eval_logical_or(ast_node_t* node, cel_context_t* ctx) {
    // 求值左侧
    cel_result_t left_result = cel_eval_node(node->data.call.args[0], ctx);
    if (!left_result.success) {
        return left_result;
    }

    cel_value_t* left = left_result.data.value;

    if (left->type != CEL_TYPE_BOOL) {
        cel_value_release(left);
        return cel_error_result(CEL_ERR_TYPE, "Logical OR requires boolean operands");
    }

    // 短路: 如果左侧为 true,不求值右侧
    if (left->data.bool_val) {
        return cel_ok_result(left); // 直接返回 true
    }

    cel_value_release(left);

    // 求值右侧
    return cel_eval_node(node->data.call.args[1], ctx);
}

/**
 * @brief 求值三元条件表达式
 */
static cel_result_t cel_eval_conditional(ast_node_t* node, cel_context_t* ctx) {
    // 求值条件
    cel_result_t cond_result = cel_eval_node(node->data.conditional.condition, ctx);
    if (!cond_result.success) {
        return cond_result;
    }

    cel_value_t* cond = cond_result.data.value;

    if (cond->type != CEL_TYPE_BOOL) {
        cel_value_release(cond);
        return cel_error_result(CEL_ERR_TYPE, "Condition must be boolean");
    }

    bool is_true = cond->data.bool_val;
    cel_value_release(cond);

    // 懒惰求值: 只求值被选中的分支
    if (is_true) {
        return cel_eval_node(node->data.conditional.true_expr, ctx);
    } else {
        return cel_eval_node(node->data.conditional.false_expr, ctx);
    }
}

/**
 * @brief 求值推导式 (Comprehension)
 */
static cel_result_t cel_eval_comprehension(ast_node_t* node, cel_context_t* ctx) {
    // 求值迭代范围
    cel_result_t range_result = cel_eval_node(node->data.comprehension.iter_range, ctx);
    if (!range_result.success) {
        return range_result;
    }

    cel_value_t* range = range_result.data.value;

    // 检查是否是可迭代类型
    if (range->type != CEL_TYPE_LIST && range->type != CEL_TYPE_MAP) {
        cel_value_release(range);
        return cel_error_result(CEL_ERR_TYPE, "Comprehension requires list or map");
    }

    // 创建子上下文
    cel_context_t* child_ctx = cel_context_create_child(ctx);

    // 求值累加器初始值
    cel_result_t accu_init_result = cel_eval_node(node->data.comprehension.accu_init, ctx);
    if (!accu_init_result.success) {
        cel_context_destroy(child_ctx);
        cel_value_release(range);
        return accu_init_result;
    }

    cel_value_t* accu = accu_init_result.data.value;

    // 迭代列表
    if (range->type == CEL_TYPE_LIST) {
        cel_list_t* list = range->data.list_val;
        for (size_t i = 0; i < list->length; i++) {
            // 绑定循环变量
            cel_context_add_variable(child_ctx, node->data.comprehension.iter_var, list->items[i]);

            // 绑定累加器
            cel_context_add_variable(child_ctx, node->data.comprehension.accu_var, accu);

            // 检查循环条件
            cel_result_t cond_result = cel_eval_node(node->data.comprehension.loop_cond, child_ctx);
            if (!cond_result.success) {
                cel_value_release(accu);
                cel_context_destroy(child_ctx);
                cel_value_release(range);
                return cond_result;
            }

            cel_value_t* cond = cond_result.data.value;
            bool should_continue = (cond->type == CEL_TYPE_BOOL && cond->data.bool_val);
            cel_value_release(cond);

            if (!should_continue) {
                break; // 提前退出循环
            }

            // 求值循环步骤
            cel_result_t step_result = cel_eval_node(node->data.comprehension.loop_step, child_ctx);
            if (!step_result.success) {
                cel_value_release(accu);
                cel_context_destroy(child_ctx);
                cel_value_release(range);
                return step_result;
            }

            // 更新累加器
            cel_value_release(accu);
            accu = step_result.data.value;
        }
    }

    // 求值最终结果
    cel_context_add_variable(child_ctx, node->data.comprehension.accu_var, accu);
    cel_result_t result = cel_eval_node(node->data.comprehension.result, child_ctx);

    cel_value_release(accu);
    cel_context_destroy(child_ctx);
    cel_value_release(range);

    return result;
}
```

### 4.3 内存管理策略

#### 4.3.1 引用计数的循环引用问题

**问题**: 如果 Map 中包含自身引用,引用计数永远不会归零。

**解决方案:**

1. **文档声明**: 在文档中说明不支持循环引用
2. **可选的循环检测**: 在 Debug 模式下进行循环检测
3. **弱引用**: 提供 weak reference 机制(复杂)

**Debug 模式循环检测:**

```c
#ifdef CEL_DEBUG
static bool detect_cycle(cel_value_t* value, cel_value_t** visited, size_t* count) {
    // 检查是否已访问
    for (size_t i = 0; i < *count; i++) {
        if (visited[i] == value) {
            return true; // 发现循环
        }
    }

    visited[(*count)++] = value;

    // 递归检查子值
    if (value->type == CEL_TYPE_LIST) {
        cel_list_t* list = value->data.list_val;
        for (size_t i = 0; i < list->length; i++) {
            if (detect_cycle(list->items[i], visited, count)) {
                return true;
            }
        }
    } else if (value->type == CEL_TYPE_MAP) {
        // 遍历 Map 值
        // ...
    }

    return false;
}
#endif
```

#### 4.3.2 Arena 分配器实现

```c
// cel_memory.c

#define ARENA_BLOCK_SIZE (64 * 1024) // 64KB per block

struct arena {
    uint8_t* buffer;
    size_t offset;
    size_t capacity;
    struct arena* next;
};

arena_t* arena_create(size_t capacity) {
    if (capacity == 0) {
        capacity = ARENA_BLOCK_SIZE;
    }

    arena_t* arena = malloc(sizeof(arena_t));
    if (!arena) return NULL;

    arena->buffer = malloc(capacity);
    if (!arena->buffer) {
        free(arena);
        return NULL;
    }

    arena->offset = 0;
    arena->capacity = capacity;
    arena->next = NULL;

    return arena;
}

void* arena_alloc(arena_t* arena, size_t size) {
    // 对齐到 8 字节
    size = (size + 7) & ~7;

    // 当前 block 是否有足够空间
    if (arena->offset + size <= arena->capacity) {
        void* ptr = arena->buffer + arena->offset;
        arena->offset += size;
        return ptr;
    }

    // 创建新 block
    arena_t* new_block = arena_create(size > ARENA_BLOCK_SIZE ? size : ARENA_BLOCK_SIZE);
    if (!new_block) return NULL;

    new_block->next = arena->next;
    arena->next = new_block;

    return arena_alloc(new_block, size);
}

void arena_destroy(arena_t* arena) {
    while (arena) {
        arena_t* next = arena->next;
        free(arena->buffer);
        free(arena);
        arena = next;
    }
}

void arena_reset(arena_t* arena) {
    // 重置所有 block 的 offset,但保留内存
    arena_t* current = arena;
    while (current) {
        current->offset = 0;
        current = current->next;
    }
}
```

### 4.4 并发安全设计

#### 4.4.1 不可变值共享

**原则**: 值一旦创建,不可修改

```c
// 创建值时设置为不可变
cel_value_t* cel_value_create_int(int64_t value) {
    cel_value_t* val = malloc(sizeof(cel_value_t));
    if (!val) return NULL;

    val->type = CEL_TYPE_INT64;
    val->ref_count = 1;
    val->data.int_val = value;

    return val;
}

// List 和 Map 创建后不可修改
// 追加操作返回新的 List/Map
cel_list_t* cel_list_append_immutable(cel_list_t* list, cel_value_t* value) {
    cel_list_t* new_list = cel_list_clone(list);
    cel_list_append(new_list, value);
    return new_list;
}
```

#### 4.4.2 线程安全的引用计数

```c
// 使用 atomic 操作 (C11 stdatomic.h)
#include <stdatomic.h>

typedef struct cel_value {
    cel_value_type_e type;
    atomic_int ref_count; // 原子引用计数
    // ...
} cel_value_t;

void cel_value_retain(cel_value_t* value) {
    if (value) {
        atomic_fetch_add(&value->ref_count, 1);
    }
}

void cel_value_release(cel_value_t* value) {
    if (value) {
        int old_count = atomic_fetch_sub(&value->ref_count, 1);
        if (old_count == 1) {
            cel_value_destroy(value);
        }
    }
}
```

---

## 5. API 设计

### 5.1 公开 API

```c
// cel.h (主头文件)

#ifndef CEL_H
#define CEL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// 前向声明
typedef struct cel_program cel_program_t;
typedef struct cel_context cel_context_t;
typedef struct cel_value cel_value_t;
typedef struct cel_error cel_error_t;

/**
 * @brief 编译 CEL 表达式
 * @param source CEL 源代码字符串
 * @param program 输出: 编译后的程序对象
 * @param error 输出: 错误信息 (如果编译失败)
 * @return 成功返回 0, 失败返回错误码
 *
 * @threadsafe 线程安全
 *
 * @example
 *   cel_program_t* program;
 *   cel_error_t* error;
 *   if (cel_compile("x + y", &program, &error) != 0) {
 *       fprintf(stderr, "Compile error: %s\n", cel_error_message(error));
 *       cel_error_destroy(error);
 *       return;
 *   }
 */
int cel_compile(const char* source, cel_program_t** program, cel_error_t** error);

/**
 * @brief 执行 CEL 程序
 * @param program 编译后的程序
 * @param context 执行上下文
 * @param result 输出: 执行结果值
 * @param error 输出: 错误信息 (如果执行失败)
 * @return 成功返回 0, 失败返回错误码
 *
 * @threadsafe_with_different_contexts
 *            不同 context 可并发执行
 *
 * @example
 *   cel_value_t* result;
 *   cel_error_t* error;
 *   if (cel_execute(program, context, &result, &error) != 0) {
 *       fprintf(stderr, "Execution error: %s\n", cel_error_message(error));
 *       cel_error_destroy(error);
 *       return;
 *   }
 *   // 使用 result...
 *   cel_value_release(result);
 */
int cel_execute(cel_program_t* program, cel_context_t* context,
                cel_value_t** result, cel_error_t** error);

/**
 * @brief 销毁程序对象
 */
void cel_program_destroy(cel_program_t* program);

/**
 * @brief 创建默认上下文 (包含所有内置函数)
 * @return 上下文对象
 *
 * @threadsafe 线程安全
 */
cel_context_t* cel_context_create(void);

/**
 * @brief 创建空上下文 (不含内置函数)
 */
cel_context_t* cel_context_create_empty(void);

/**
 * @brief 销毁上下文
 */
void cel_context_destroy(cel_context_t* context);

/**
 * @brief 添加变量到上下文
 * @param context 上下文
 * @param name 变量名
 * @param value 变量值 (context 会增加引用计数)
 * @return 成功返回 0
 */
int cel_context_add_variable(cel_context_t* context,
                              const char* name,
                              cel_value_t* value);

/**
 * @brief 添加函数到上下文
 * @param context 上下文
 * @param name 函数名
 * @param func 函数指针
 * @param min_args 最少参数数
 * @param max_args 最多参数数 (SIZE_MAX = 可变参数)
 * @return 成功返回 0
 */
typedef int (*cel_function_fn)(void* ctx, cel_value_t** args, size_t count,
                                cel_value_t** result, cel_error_t** error);

int cel_context_add_function(cel_context_t* context,
                              const char* name,
                              cel_function_fn func,
                              size_t min_args,
                              size_t max_args);

/**
 * @brief 创建各种类型的值
 */
cel_value_t* cel_value_create_null(void);
cel_value_t* cel_value_create_bool(bool value);
cel_value_t* cel_value_create_int(int64_t value);
cel_value_t* cel_value_create_uint(uint64_t value);
cel_value_t* cel_value_create_double(double value);
cel_value_t* cel_value_create_string(const char* str, size_t length);

/**
 * @brief 引用计数操作
 */
void cel_value_retain(cel_value_t* value);
void cel_value_release(cel_value_t* value);

/**
 * @brief 获取值的类型
 */
const char* cel_value_type_name(cel_value_t* value);

/**
 * @brief 将��转换为字符串 (需要调用者释放)
 */
char* cel_value_to_string(cel_value_t* value);

/**
 * @brief 错误处理
 */
const char* cel_error_message(cel_error_t* error);
int cel_error_code(cel_error_t* error);
void cel_error_destroy(cel_error_t* error);

#endif // CEL_H
```

### 5.2 使用示例

#### 5.2.1 基本使用

```c
#include <stdio.h>
#include <cel.h>

int main() {
    // 编译表达式
    cel_program_t* program;
    cel_error_t* error;
    if (cel_compile("2 + 3 * 4", &program, &error) != 0) {
        fprintf(stderr, "Compile error: %s\n", cel_error_message(error));
        cel_error_destroy(error);
        return 1;
    }

    // 创建上下文
    cel_context_t* context = cel_context_create();

    // 执行
    cel_value_t* result;
    if (cel_execute(program, context, &result, &error) != 0) {
        fprintf(stderr, "Execution error: %s\n", cel_error_message(error));
        cel_error_destroy(error);
        cel_context_destroy(context);
        cel_program_destroy(program);
        return 1;
    }

    // 输出结果
    char* result_str = cel_value_to_string(result);
    printf("Result: %s\n", result_str); // "14"
    free(result_str);

    // 清理
    cel_value_release(result);
    cel_context_destroy(context);
    cel_program_destroy(program);

    return 0;
}
```

#### 5.2.2 使用变量

```c
// 创建上下文并添加变量
cel_context_t* context = cel_context_create();

cel_value_t* x = cel_value_create_int(10);
cel_value_t* y = cel_value_create_int(20);

cel_context_add_variable(context, "x", x);
cel_context_add_variable(context, "y", y);

cel_value_release(x); // context 已增加引用计数
cel_value_release(y);

// 编译并执行
cel_program_t* program;
cel_error_t* error;
cel_compile("x + y", &program, &error);

cel_value_t* result;
cel_execute(program, context, &result, &error);

// result 为 30
```

#### 5.2.3 自定义函数

```c
// 自定义函数实现
int my_add_func(void* ctx, cel_value_t** args, size_t count,
                cel_value_t** result, cel_error_t** error) {
    if (count != 2) {
        *error = cel_error_create(CEL_ERR_INVALID_ARG_COUNT,
                                  "add() requires 2 arguments");
        return -1;
    }

    if (args[0]->type != CEL_TYPE_INT64 || args[1]->type != CEL_TYPE_INT64) {
        *error = cel_error_create(CEL_ERR_TYPE,
                                  "add() requires integer arguments");
        return -1;
    }

    int64_t a = args[0]->data.int_val;
    int64_t b = args[1]->data.int_val;

    *result = cel_value_create_int(a + b);
    return 0;
}

// 注册函数
cel_context_t* context = cel_context_create();
cel_context_add_function(context, "add", my_add_func, 2, 2);

// 使用
cel_program_t* program;
cel_error_t* error;
cel_compile("add(5, 10)", &program, &error);

cel_value_t* result;
cel_execute(program, context, &result, &error);
// result 为 15
```

---

## 6. 实现建议

### 6.1 开发阶段划分

**第 1 阶段: MVP (最小可行产品) - 2-3 个月**

- 核心数据结构: cel_value, cel_ast
- 简单解析器 (手写递归下降或 Lemon)
- 基础求值器
- 支持的特性:
  - 基础类型: Int, Bool, String, Null
  - 算术和比较运算符
  - 变量引用
  - 简单函数调用
  - 基础内置函数: size, contains, string

**第 2 阶段: 核心功能 - 2-3 个月**

- 完整的解析器 (re2c + Lemon)
- 完整的值类型 (List, Map, Float, Bytes)
- 所有运算符实现
- 上下文系统和函数注册
- 宏系统 (has, all, exists, map, filter)
- 完整的内置函数库

**第 3 阶段: 高级特性 - 1-2 个月**

- Optional 类型
- Opaque 类型扩展
- 变量解析器
- 时间类型 (Timestamp, Duration)
- 正则表达式支持
- 错误恢复和详细错误报告

**第 4 阶段: 优化与测试 - 1-2 个月**

- 性能优化 (内存池, Copy-on-Write, 字符串 interning)
- 完整的单元测试
- 集成测试
- 模糊测试
- 内存泄漏检测
- 性能基准测试

**第 5 阶段: 文档与发布 - 1 个月**

- API 文档
- 用户指南
- 示例代码
- 发布 1.0 版本

### 6.2 测试策略

#### 6.2.1 单元测试

使用 Unity 测试框架:

```c
// test_value.c
#include "unity.h"
#include "cel_value.h"

void setUp(void) {
    // 每个测试前调用
}

void tearDown(void) {
    // 每个测试后调用
}

void test_create_int_value(void) {
    cel_value_t* val = cel_value_create_int(42);
    TEST_ASSERT_NOT_NULL(val);
    TEST_ASSERT_EQUAL(CEL_TYPE_INT64, val->type);
    TEST_ASSERT_EQUAL_INT64(42, val->data.int_val);
    cel_value_release(val);
}

void test_string_concat(void) {
    cel_value_t* a = cel_value_create_string("hello", 5);
    cel_value_t* b = cel_value_create_string(" world", 6);

    cel_value_t* result = cel_string_concat(a, b);

    char* str = cel_value_to_string(result);
    TEST_ASSERT_EQUAL_STRING("hello world", str);
    free(str);

    cel_value_release(a);
    cel_value_release(b);
    cel_value_release(result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_create_int_value);
    RUN_TEST(test_string_concat);
    return UNITY_END();
}
```

#### 6.2.2 集成测试

```c
// test_integration.c
void test_full_expression(void) {
    cel_program_t* program;
    cel_error_t* error;

    int ret = cel_compile("(1 + 2) * 3 == 9", &program, &error);
    TEST_ASSERT_EQUAL(0, ret);

    cel_context_t* context = cel_context_create();

    cel_value_t* result;
    ret = cel_execute(program, context, &result, &error);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(CEL_TYPE_BOOL, result->type);
    TEST_ASSERT_TRUE(result->data.bool_val);

    cel_value_release(result);
    cel_context_destroy(context);
    cel_program_destroy(program);
}
```

#### 6.2.3 模糊测试

```c
// fuzz_parser.c
#include <stdint.h>
#include <stddef.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // 将输入作为 CEL 表达式
    char* source = malloc(size + 1);
    memcpy(source, data, size);
    source[size] = '\0';

    cel_program_t* program;
    cel_error_t* error;

    // 尝试编译 (不应崩溃)
    cel_compile(source, &program, &error);

    if (program) {
        cel_program_destroy(program);
    } else {
        cel_error_destroy(error);
    }

    free(source);
    return 0;
}
```

### 6.3 性能基准测试

```c
// bench_eval.c
#include <time.h>

void benchmark_expression(const char* expr, size_t iterations) {
    cel_program_t* program;
    cel_error_t* error;
    cel_compile(expr, &program, &error);

    cel_context_t* context = cel_context_create();

    clock_t start = clock();

    for (size_t i = 0; i < iterations; i++) {
        cel_value_t* result;
        cel_execute(program, context, &result, &error);
        cel_value_release(result);
    }

    clock_t end = clock();

    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    double ops_per_sec = iterations / time_spent;

    printf("%s: %.2f ops/sec\n", expr, ops_per_sec);

    cel_context_destroy(context);
    cel_program_destroy(program);
}

int main() {
    benchmark_expression("1 + 2", 1000000);
    benchmark_expression("[1, 2, 3].map(x, x * 2)", 100000);
    return 0;
}
```

### 6.4 与 Rust 版本的兼容性验证

**测试用例共享:**

1. 从 cel-rust 导出测试用例为 JSON 格式
2. C 实现读取相同的测试用例
3. 比较输出结果

```json
[
  {
    "name": "simple_add",
    "expression": "1 + 2",
    "expected": 3
  },
  {
    "name": "string_concat",
    "expression": "'hello' + ' world'",
    "expected": "hello world"
  }
]
```

```c
// test_compatibility.c
void run_test_suite(const char* json_file) {
    cJSON* root = cJSON_Parse(load_file(json_file));
    cJSON* test_case = NULL;

    cJSON_ArrayForEach(test_case, root) {
        const char* name = cJSON_GetObjectItem(test_case, "name")->valuestring;
        const char* expr = cJSON_GetObjectItem(test_case, "expression")->valuestring;
        cJSON* expected = cJSON_GetObjectItem(test_case, "expected");

        // 执行测试
        cel_program_t* program;
        cel_error_t* error;
        cel_compile(expr, &program, &error);

        cel_context_t* context = cel_context_create();
        cel_value_t* result;
        cel_execute(program, context, &result, &error);

        // 比较结果
        bool passed = compare_result(result, expected);
        printf("%s: %s\n", name, passed ? "PASS" : "FAIL");

        cel_value_release(result);
        cel_context_destroy(context);
        cel_program_destroy(program);
    }

    cJSON_Delete(root);
}
```

### 6.5 代码风格规范

**遵循 Linux Kernel Coding Style:**

- 缩进: 8 空格 (或 1 tab)
- 行长度: 80 字符 (建议), 100 字符 (最大)
- 括号风格: K&R 风格
- 命名: snake_case

**示例:**

```c
// 好的风格
int cel_value_compare(const cel_value_t *a, const cel_value_t *b)
{
        if (a->type != b->type) {
                return a->type - b->type;
        }

        switch (a->type) {
        case CEL_TYPE_INT64:
                if (a->data.int_val < b->data.int_val)
                        return -1;
                if (a->data.int_val > b->data.int_val)
                        return 1;
                return 0;
        default:
                return 0;
        }
}
```

---

## 7. 总结

本设计文档为 CEL 的 C 语言实现提供了完整的技术方案:

1. **设计概述**: 目标、差异、标准、库选型
2. **架构设计**: 模块划分、依赖关系、流程图
3. **数据结构**: cel_value, cel_ast, cel_context, cel_functions, cel_error
4. **算法设计**: 解析流程、求值算法、内存管理、并发安全
5. **API 设计**: 清晰的公开 API 和使用示例
6. **实现建议**: 开发阶段、测试策略、性能测试、兼容性验证

该设计充分考虑了 C 语言的特点,采用了成熟的第三方库,设计了清晰的模块边界和内存管理策略。通过这份设计文档,开发者可以直接开始 CEL-C 的实现工作。
