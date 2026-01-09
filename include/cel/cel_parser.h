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

/* ========== 源代码位置 ========== */

/**
 * @brief 源代码位置信息
 */
typedef struct {
	size_t line;     /* 行号 (1-based) */
	size_t column;   /* 列号 (1-based) */
	size_t offset;   /* 字符偏移 (0-based) */
} cel_source_location_t;

/**
 * @brief 源代码范围
 */
typedef struct {
	cel_source_location_t start;  /* 起始位置 */
	cel_source_location_t end;    /* 结束位置 */
} cel_source_range_t;

/* ========== 解析错误 ========== */

/**
 * @brief 解析错误信息
 */
typedef struct cel_parse_error {
	char *message;                  /* 错误消息 */
	cel_source_range_t location;    /* 错误位置 */
	struct cel_parse_error *next;   /* 下一个错误 (链表) */
} cel_parse_error_t;

/**
 * @brief 解析结果
 */
typedef struct {
	cel_ast_node_t *ast;           /* 解析成功的 AST (可能为 NULL) */
	cel_parse_error_t *errors;     /* 错误列表 (链表头) */
	size_t error_count;            /* 错误数量 */
	bool has_errors;               /* 是否有错误 */
} cel_parse_result_t;

/* ========== Parser 状态 ========== */

/**
 * @brief Parser 状态
 */
typedef struct {
	cel_lexer_t *lexer;            /* 词法分析器 */
	cel_token_t current;           /* 当前 Token */
	cel_token_t previous;          /* 前一个 Token */
	bool had_error;                /* 是否有错误 */
	bool panic_mode;               /* 恐慌模式 (错误恢复) */
	cel_error_t *error;            /* 单一错误信息 (兼容旧 API) */
	cel_parse_error_t *errors;     /* 错误列表头 */
	cel_parse_error_t *errors_tail;/* 错误列表尾 */
	size_t error_count;            /* 错误数量 */
	size_t recursion_depth;        /* 递归深度 */
	size_t max_recursion;          /* 最大递归深度 */
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
 * @brief 清理 Parser 资源
 *
 * @param parser Parser 状态
 *
 * @note 释放 parser 中的错误对象
 */
void cel_parser_cleanup(cel_parser_t *parser);

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

/* ========== 高层 API ========== */

/**
 * @brief 解析 CEL 表达式 (高层 API)
 *
 * 这是推荐使用的 API，提供完整的错误收集和位置追踪。
 *
 * @param source 源代码字符串
 * @return 解析结果对象
 *
 * @note 调用者负责使用 cel_parse_result_destroy() 释放结果
 *
 * @example
 *   cel_parse_result_t result = cel_parse("1 + 2");
 *   if (result.has_errors) {
 *       // 处理错误
 *       for (cel_parse_error_t *err = result.errors; err; err = err->next) {
 *           printf("Error at line %zu: %s\n", err->location.start.line, err->message);
 *       }
 *   } else {
 *       // 使用 AST
 *       cel_eval(result.ast, ctx);
 *   }
 *   cel_parse_result_destroy(&result);
 */
cel_parse_result_t cel_parse(const char *source);

/**
 * @brief 解析 CEL 表达式 (带选项)
 *
 * @param source 源代码字符串
 * @param max_recursion 最大递归深度 (0 = 使用默认值 100)
 * @return 解析结果对象
 */
cel_parse_result_t cel_parse_with_options(const char *source, size_t max_recursion);

/* ========== 解析结果管理 ========== */

/**
 * @brief 销毁解析结果
 *
 * 释放 AST 和所有错误信息。
 *
 * @param result 解析结果
 */
void cel_parse_result_destroy(cel_parse_result_t *result);

/**
 * @brief 格式化解析错误为字符串
 *
 * @param error 解析错误
 * @param source 源代码 (用于显示错误上下文)
 * @return 格式化的错误消息 (调用者负责释放)
 */
char *cel_parse_error_format(const cel_parse_error_t *error, const char *source);

/**
 * @brief 格式化所有解析错误
 *
 * @param result 解析结果
 * @param source 源代码
 * @return 格式化的错误消息 (调用者负责释放)
 */
char *cel_parse_result_format_errors(const cel_parse_result_t *result, const char *source);

/* ========== 源位置工具 ========== */

/**
 * @brief 从 Token 创建源位置
 *
 * @param token Token
 * @return 源位置
 */
cel_source_location_t cel_source_location_from_token(const cel_token_t *token);

/**
 * @brief 从 Token 创建源范围
 *
 * @param token Token
 * @return 源范围
 */
cel_source_range_t cel_source_range_from_token(const cel_token_t *token);

#ifdef __cplusplus
}
#endif

#endif /* CEL_PARSER_H */
