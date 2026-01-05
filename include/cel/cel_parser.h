/**
 * @file cel_parser.h
 * @brief CEL 语法分析器
 *
 * 将 Token 流解析为 AST。
 */

#ifndef CEL_PARSER_H
#define CEL_PARSER_H

#include "cel/cel_ast.h"
#include "cel/cel_error.h"
#include "cel/cel_lexer.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Parser 状态 ========== */

/**
 * @brief Parser 状态
 */
typedef struct {
	cel_lexer_t *lexer;       /* 词法分析器 */
	cel_token_t current;      /* 当前 Token */
	cel_token_t previous;     /* 前一个 Token */
	bool had_error;           /* 是否有错误 */
	bool panic_mode;          /* 恐慌模式 (错误恢复) */
	cel_error_t *error;       /* 错误信息 */
	size_t recursion_depth;   /* 递归深度 */
	size_t max_recursion;     /* 最大递归深度 */
} cel_parser_t;

/* ========== Parser API ========== */

/**
 * @brief 初始化 Parser
 *
 * @param parser Parser 状态
 * @param lexer 词法分析器
 */
void cel_parser_init(cel_parser_t *parser, cel_lexer_t *lexer);

/**
 * @brief 设置最大递归深度
 *
 * @param parser Parser 状态
 * @param max_depth 最大递归深度 (默认 100)
 */
void cel_parser_set_max_recursion(cel_parser_t *parser, size_t max_depth);

/**
 * @brief 解析表达式
 *
 * @param parser Parser 状态
 * @return AST 根节点，失败返回 NULL
 *
 * @note 调用者负责释放返回的 AST (使用 cel_ast_destroy)
 * @note 错误信息存储在 parser->error 中
 */
cel_ast_node_t *cel_parser_parse(cel_parser_t *parser);

/**
 * @brief 获取 Parser 错误
 *
 * @param parser Parser 状态
 * @return 错误对象，无错误返回 NULL
 */
cel_error_t *cel_parser_get_error(const cel_parser_t *parser);

#ifdef __cplusplus
}
#endif

#endif /* CEL_PARSER_H */
