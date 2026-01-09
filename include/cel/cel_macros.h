/**
 * @file cel_macros.h
 * @brief CEL 宏展开器
 *
 * 实现 CEL 宏（has, all, exists, exists_one, map, filter）的展开功能。
 * 宏在解析阶段被展开为 Comprehension 表达式。
 */

#ifndef CEL_MACROS_H
#define CEL_MACROS_H

#include "cel/cel_ast.h"
#include "cel/cel_error.h"
#include "cel/cel_memory.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 宏类型定义 ========== */

/**
 * @brief CEL 宏类型枚举
 */
typedef enum {
    CEL_MACRO_HAS,          /* has(map.field) - 字段存在性检查 */
    CEL_MACRO_ALL,          /* list.all(x, predicate) - 全称量词 */
    CEL_MACRO_EXISTS,       /* list.exists(x, predicate) - 存在量词 */
    CEL_MACRO_EXISTS_ONE,   /* list.exists_one(x, predicate) - 唯一性量词 */
    CEL_MACRO_MAP,          /* list.map(x, transform) - 列表映射 */
    CEL_MACRO_FILTER,       /* list.filter(x, predicate) - 列表过滤 */
    CEL_MACRO_UNKNOWN       /* 未知宏 */
} cel_macro_type_e;

/**
 * @brief 宏展开辅助器
 *
 * 用于在宏展开过程中生成新的 AST 节点并分配 ID
 */
typedef struct {
    arena_t *arena;        /* Arena 分配器 */
    uint64_t next_id;      /* 下一个可用的节点 ID */
} cel_macro_helper_t;

/* ========== 宏检测和展开 API ========== */

/**
 * @brief 检测函数调用是否是宏
 *
 * @param func_name 函数名称
 * @param has_target 是否有目标对象（方法调用）
 * @param arg_count 参数数量
 * @return 宏类型，如果不是宏则返回 CEL_MACRO_UNKNOWN
 */
cel_macro_type_e cel_macro_detect(const char *func_name,
                                    bool has_target,
                                    size_t arg_count);

/**
 * @brief 展开宏为 Comprehension 表达式
 *
 * @param helper 宏展开辅助器
 * @param macro_type 宏类型
 * @param target 目标表达式（方法调用的接收者），NULL 表示无目标
 * @param args 宏参数数组
 * @param arg_count 参数数量
 * @param result 输出参数：展开后的 AST 节点
 * @return CEL_OK 成功，其他值表示错误
 */
cel_error_code_e cel_macro_expand(cel_macro_helper_t *helper,
                                    cel_macro_type_e macro_type,
                                    cel_ast_node_t *target,
                                    cel_ast_node_t **args,
                                    size_t arg_count,
                                    cel_ast_node_t **result);

/* ========== 辅助器管理 API ========== */

/**
 * @brief 创建宏展开辅助器
 *
 * @param arena Arena 分配器
 * @param start_id 起始节点 ID
 * @return 宏展开辅助器指针，失败返回 NULL
 */
cel_macro_helper_t *cel_macro_helper_create(arena_t *arena, uint64_t start_id);

/**
 * @brief 销毁宏展开辅助器
 *
 * @param helper 宏展开辅助器
 */
void cel_macro_helper_destroy(cel_macro_helper_t *helper);

/**
 * @brief 创建新的 AST 节点并分配 ID
 *
 * @param helper 宏展开辅助器
 * @param type 节点类型
 * @return 新创建的 AST 节点，失败返回 NULL
 */
cel_ast_node_t *cel_macro_helper_new_node(cel_macro_helper_t *helper,
                                            cel_ast_node_type_e type);

/* ========== 具体宏展开函数 ========== */

/**
 * @brief 展开 has() 宏
 *
 * has(obj.field) => obj.field (with test=true flag)
 *
 * @param helper 宏展开辅助器
 * @param args 参数数组（预期 1 个参数）
 * @param arg_count 参数数量
 * @param result 输出参数：展开后的节点
 * @return CEL_OK 成功，其他值表示错误
 */
cel_error_code_e cel_macro_expand_has(cel_macro_helper_t *helper,
                                        cel_ast_node_t **args,
                                        size_t arg_count,
                                        cel_ast_node_t **result);

/**
 * @brief 展开 all() 宏
 *
 * list.all(x, predicate) =>
 *   Comprehension(
 *     iter_var: x,
 *     iter_range: list,
 *     accu_var: @result,
 *     accu_init: true,
 *     loop_cond: @result,
 *     loop_step: @result && predicate,
 *     result: @result
 *   )
 *
 * @param helper 宏展开辅助器
 * @param target 目标列表表达式
 * @param args 参数数组（预期 2 个参数：变量名, 谓词）
 * @param arg_count 参数数量
 * @param result 输出参数：展开后的节点
 * @return CEL_OK 成功，其他值表示错误
 */
cel_error_code_e cel_macro_expand_all(cel_macro_helper_t *helper,
                                        cel_ast_node_t *target,
                                        cel_ast_node_t **args,
                                        size_t arg_count,
                                        cel_ast_node_t **result);

/**
 * @brief 展开 exists() 宏
 *
 * list.exists(x, predicate) =>
 *   Comprehension(
 *     iter_var: x,
 *     iter_range: list,
 *     accu_var: @result,
 *     accu_init: false,
 *     loop_cond: !@result,
 *     loop_step: @result || predicate,
 *     result: @result
 *   )
 *
 * @param helper 宏展开辅助器
 * @param target 目标列表表达式
 * @param args 参数数组（预期 2 个参数：变量名, 谓词）
 * @param arg_count 参数数量
 * @param result 输出参数：展开后的节点
 * @return CEL_OK 成功，其他值表示错误
 */
cel_error_code_e cel_macro_expand_exists(cel_macro_helper_t *helper,
                                           cel_ast_node_t *target,
                                           cel_ast_node_t **args,
                                           size_t arg_count,
                                           cel_ast_node_t **result);

/**
 * @brief 展开 exists_one() / existsOne() 宏
 *
 * list.exists_one(x, predicate) =>
 *   Comprehension(
 *     iter_var: x,
 *     iter_range: list,
 *     accu_var: @result,
 *     accu_init: 0,
 *     loop_cond: true,
 *     loop_step: predicate ? (@result + 1) : @result,
 *     result: @result == 1
 *   )
 *
 * @param helper 宏展开辅助器
 * @param target 目标列表表达式
 * @param args 参数数组（预期 2 个参数：变量名, 谓词）
 * @param arg_count 参数数量
 * @param result 输出参数：展开后的节点
 * @return CEL_OK 成功，其他值表示错误
 */
cel_error_code_e cel_macro_expand_exists_one(cel_macro_helper_t *helper,
                                               cel_ast_node_t *target,
                                               cel_ast_node_t **args,
                                               size_t arg_count,
                                               cel_ast_node_t **result);

/**
 * @brief 展开 map() 宏
 *
 * 两种形式：
 * 1. list.map(x, transform) - 映射所有元素
 * 2. list.map(x, filter, transform) - 先过滤再映射
 *
 * list.map(x, transform) =>
 *   Comprehension(
 *     iter_var: x,
 *     iter_range: list,
 *     accu_var: @result,
 *     accu_init: [],
 *     loop_cond: true,
 *     loop_step: @result + [transform],
 *     result: @result
 *   )
 *
 * @param helper 宏展开辅助器
 * @param target 目标列表表达式
 * @param args 参数数组（2 或 3 个参数）
 * @param arg_count 参数数量
 * @param result 输出参数：展开后的节点
 * @return CEL_OK 成功，其他值表示错误
 */
cel_error_code_e cel_macro_expand_map(cel_macro_helper_t *helper,
                                        cel_ast_node_t *target,
                                        cel_ast_node_t **args,
                                        size_t arg_count,
                                        cel_ast_node_t **result);

/**
 * @brief 展开 filter() 宏
 *
 * list.filter(x, predicate) =>
 *   Comprehension(
 *     iter_var: x,
 *     iter_range: list,
 *     accu_var: @result,
 *     accu_init: [],
 *     loop_cond: true,
 *     loop_step: predicate ? (@result + [x]) : @result,
 *     result: @result
 *   )
 *
 * @param helper 宏展开辅助器
 * @param target 目标列表表达式
 * @param args 参数数组（预期 2 个参数：变量名, 谓词）
 * @param arg_count 参数数量
 * @param result 输出参数：展开后的节点
 * @return CEL_OK 成功，其他值表示错误
 */
cel_error_code_e cel_macro_expand_filter(cel_macro_helper_t *helper,
                                           cel_ast_node_t *target,
                                           cel_ast_node_t **args,
                                           size_t arg_count,
                                           cel_ast_node_t **result);

/* ========== 辅助函数 ========== */

/**
 * @brief 从标识符节点提取变量名
 *
 * @param node AST 节点（必须是 CEL_AST_IDENT 类型）
 * @param var_name 输出参数：变量名字符串
 * @return CEL_OK 成功，其他值表示错误
 */
cel_error_code_e cel_macro_extract_ident(cel_ast_node_t *node, const char **var_name);

/**
 * @brief 创建累加器标识符节点
 *
 * 创建引用累加器变量 "@result" 的标识符节点
 *
 * @param helper 宏展开辅助器
 * @return 标识符节点，失败返回 NULL
 */
cel_ast_node_t *cel_macro_create_accu_ident(cel_macro_helper_t *helper);

/**
 * @brief 创建布尔字面量节点
 *
 * @param helper 宏展开辅助器
 * @param value 布尔值
 * @return 字面量节点，失败返回 NULL
 */
cel_ast_node_t *cel_macro_create_bool_literal(cel_macro_helper_t *helper, bool value);

/**
 * @brief 创建整数字面量节点
 *
 * @param helper 宏展开辅助器
 * @param value 整数值
 * @return 字面量节点，失败返回 NULL
 */
cel_ast_node_t *cel_macro_create_int_literal(cel_macro_helper_t *helper, int64_t value);

/**
 * @brief 创建空列表字面量节点
 *
 * @param helper 宏展开辅助器
 * @return 列表节点，失败返回 NULL
 */
cel_ast_node_t *cel_macro_create_empty_list(cel_macro_helper_t *helper);

#ifdef __cplusplus
}
#endif

#endif /* CEL_MACROS_H */
