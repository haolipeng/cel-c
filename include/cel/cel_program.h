/**
 * @file cel_program.h
 * @brief CEL 程序对象 API
 *
 * 提供编译和执行 CEL 表达式的高层 API。
 */

#ifndef CEL_PROGRAM_H
#define CEL_PROGRAM_H

#include "cel/cel_ast.h"
#include "cel/cel_context.h"
#include "cel/cel_error.h"
#include "cel/cel_parser.h"
#include "cel/cel_value.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 程序对象 ========== */

/**
 * @brief CEL 编译后的程序对象
 *
 * 包含解析后的 AST，可以多次执行。
 */
typedef struct cel_program {
	cel_ast_node_t *ast;           /* 解析后的 AST */
	char *source;                  /* 源代码副本 (用于错误报告) */
	size_t source_length;          /* 源代码长度 */
} cel_program_t;

/**
 * @brief 编译选项
 */
typedef struct {
	size_t max_recursion_depth;    /* 最大解析递归深度 (默认 100) */
	bool enable_macros;            /* 是否启用宏 (默认 true) */
} cel_compile_options_t;

/**
 * @brief 执行选项
 */
typedef struct {
	size_t max_eval_recursion;     /* 最大求值递归深度 (默认 100) */
	size_t timeout_ms;             /* 超时时间 (毫秒, 0 = 无限) */
} cel_execute_options_t;

/**
 * @brief 编译结果
 */
typedef struct {
	cel_program_t *program;        /* 编译成功的程序 */
	cel_parse_error_t *errors;     /* 错误列表 */
	size_t error_count;            /* 错误数量 */
	bool has_errors;               /* 是否有错误 */
} cel_compile_result_t;

/**
 * @brief 执行结果
 */
typedef struct {
	cel_value_t value;             /* 执行结果值 */
	cel_error_t *error;            /* 错误信息 */
	bool success;                  /* 是否成功 */
} cel_execute_result_t;

/* ========== 编译 API ========== */

/**
 * @brief 编译 CEL 表达式
 *
 * 将 CEL 源代码编译为可执行的程序对象。
 *
 * @param source 源代码字符串
 * @return 编译结果
 *
 * @example
 *   cel_compile_result_t result = cel_compile("user.age >= 18");
 *   if (result.has_errors) {
 *       // 处理编译错误
 *   }
 *   // 使用 result.program 执行表达式
 *   cel_compile_result_destroy(&result);
 */
cel_compile_result_t cel_compile(const char *source);

/**
 * @brief 编译 CEL 表达式 (带选项)
 *
 * @param source 源代码字符串
 * @param options 编译选项
 * @return 编译结果
 */
cel_compile_result_t cel_compile_with_options(const char *source,
					       const cel_compile_options_t *options);

/**
 * @brief 销毁编译结果
 *
 * 释放程序和所有错误信息。
 *
 * @param result 编译结果
 */
void cel_compile_result_destroy(cel_compile_result_t *result);

/* ========== 程序管理 ========== */

/**
 * @brief 销毁程序对象
 *
 * @param program 程序对象
 */
void cel_program_destroy(cel_program_t *program);

/**
 * @brief 获取程序源代码
 *
 * @param program 程序对象
 * @return 源代码字符串 (不需要释放)
 */
const char *cel_program_get_source(const cel_program_t *program);

/* ========== 执行 API ========== */

/**
 * @brief 执行程序
 *
 * 在给定上下文中执行编译后的程序。
 *
 * @param program 程序对象
 * @param ctx 执行上下文
 * @return 执行结果
 *
 * @example
 *   cel_execute_result_t result = cel_execute(program, ctx);
 *   if (result.success) {
 *       // 使用 result.value
 *   } else {
 *       // 处理执行错误
 *   }
 *   cel_execute_result_destroy(&result);
 */
cel_execute_result_t cel_execute(const cel_program_t *program, cel_context_t *ctx);

/**
 * @brief 执行程序 (带选项)
 *
 * @param program 程序对象
 * @param ctx 执行上下文
 * @param options 执行选项
 * @return 执行结果
 */
cel_execute_result_t cel_execute_with_options(const cel_program_t *program,
					       cel_context_t *ctx,
					       const cel_execute_options_t *options);

/**
 * @brief 销毁执行结果
 *
 * 释放结果值和错误信息。
 *
 * @param result 执行结果
 */
void cel_execute_result_destroy(cel_execute_result_t *result);

/* ========== 便捷 API ========== */

/**
 * @brief 编译并执行表达式 (一步完成)
 *
 * 这是最简单的使用方式，适合一次性求值。
 * 如果需要多次执行同一表达式，请使用 cel_compile() + cel_execute()。
 *
 * @param source 源代码字符串
 * @param ctx 执行上下文
 * @return 执行结果
 *
 * @example
 *   cel_context_t *ctx = cel_context_create();
 *   cel_value_t age = cel_value_int(25);
 *   cel_context_add_variable(ctx, "age", &age);
 *
 *   cel_execute_result_t result = cel_eval_expression("age >= 18", ctx);
 *   if (result.success && result.value.type == CEL_TYPE_BOOL) {
 *       printf("Is adult: %s\n", result.value.value.bool_value ? "yes" : "no");
 *   }
 *
 *   cel_execute_result_destroy(&result);
 *   cel_context_destroy(ctx);
 */
cel_execute_result_t cel_eval_expression(const char *source, cel_context_t *ctx);

/**
 * @brief 检查表达式语法
 *
 * 仅解析表达式，不执行。用于验证语法正确性。
 *
 * @param source 源代码字符串
 * @return true 语法正确, false 有语法错误
 */
bool cel_check_syntax(const char *source);

/**
 * @brief 默认编译选项
 */
cel_compile_options_t cel_default_compile_options(void);

/**
 * @brief 默认执行选项
 */
cel_execute_options_t cel_default_execute_options(void);

#ifdef __cplusplus
}
#endif

#endif /* CEL_PROGRAM_H */
