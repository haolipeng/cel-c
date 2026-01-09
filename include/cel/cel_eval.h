/**
 * @file cel_eval.h
 * @brief CEL 求值器
 *
 * 对 AST 进行求值，产生结果值。
 */

#ifndef CEL_EVAL_H
#define CEL_EVAL_H

#include "cel/cel_ast.h"
#include "cel/cel_context.h"
#include "cel/cel_error.h"
#include "cel/cel_value.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 求值器 API ========== */

/**
 * @brief 对 AST 求值
 *
 * @param ast AST 根节点
 * @param ctx 求值上下文
 * @param result 输出结果
 * @return true 成功，false 失败
 */
bool cel_eval(const cel_ast_node_t *ast, cel_context_t *ctx,
	      cel_value_t *result);

#ifdef __cplusplus
}
#endif

#endif /* CEL_EVAL_H */
