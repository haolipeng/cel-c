/**
 * @file cel_eval.h
 * @brief CEL 求值器
 *
 * 对 AST 进行求值，产生结果值。
 */

#ifndef CEL_EVAL_H
#define CEL_EVAL_H

#include "cel/cel_ast.h"
#include "cel/cel_error.h"
#include "cel/cel_value.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 求值上下文 ========== */

/**
 * @brief 变量绑定
 */
typedef struct {
	const char *name;   /* 变量名 */
	size_t name_length; /* 名称长度 */
	cel_value_t value;  /* 变量值 */
} cel_binding_t;

/**
 * @brief 求值上下文
 *
 * 存储变量绑定和函数注册。
 */
typedef struct {
	cel_binding_t *bindings;  /* 变量绑定数组 */
	size_t binding_count;     /* 绑定数量 */
	size_t binding_capacity;  /* 绑定容量 */
	cel_error_t *error;       /* 错误信息 */
} cel_context_t;

/* ========== 求值器 API ========== */

/**
 * @brief 创建求值上下文
 *
 * @return 新的上下文，失败返回 NULL
 */
cel_context_t *cel_context_create(void);

/**
 * @brief 销毁求值上下文
 *
 * @param ctx 上下文
 */
void cel_context_destroy(cel_context_t *ctx);

/**
 * @brief 添加变量绑定
 *
 * @param ctx 上下文
 * @param name 变量名
 * @param name_length 名称长度
 * @param value 变量值
 * @return true 成功，false 失败
 */
bool cel_context_add_binding(cel_context_t *ctx, const char *name,
			      size_t name_length, cel_value_t value);

/**
 * @brief 查找变量
 *
 * @param ctx 上下文
 * @param name 变量名
 * @param name_length 名称长度
 * @param out 输出值
 * @return true 找到，false 未找到
 */
bool cel_context_lookup(const cel_context_t *ctx, const char *name,
			 size_t name_length, cel_value_t *out);

/**
 * @brief 对 AST 求值
 *
 * @param ast AST 根节点
 * @param ctx 求值上下文
 * @param result 输出结果
 * @return true 成功，false 失败
 *
 * @note 失败时错误信息存储在 ctx->error 中
 */
bool cel_eval(const cel_ast_node_t *ast, cel_context_t *ctx,
	      cel_value_t *result);

/**
 * @brief 获取求值错误
 *
 * @param ctx 上下文
 * @return 错误对象，无错误返回 NULL
 */
cel_error_t *cel_context_get_error(const cel_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* CEL_EVAL_H */
