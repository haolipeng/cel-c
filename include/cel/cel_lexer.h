/**
 * @file cel_lexer.h
 * @brief CEL 词法分析器
 *
 * 将 CEL 表达式源代码分解为 Token 流。
 */

#ifndef CEL_LEXER_H
#define CEL_LEXER_H

#include "cel/cel_token.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 词法分析器状态 ========== */

/**
 * @brief 词法分析器状态
 *
 * 维护词法分析的当前状态。
 */
typedef struct {
	const char *source;   /* 源代码文本 (必须以 \0 结尾) */
	const char *start;    /* 当前 Token 的起始位置 */
	const char *current;  /* 当前扫描位置 */
	size_t line;          /* 当前行号 (1-based) */
	size_t column;        /* 当前列号 (1-based) */
	size_t line_start;    /* 当前行的起始偏移 (0-based) */
} cel_lexer_t;

/* ========== 词法分析器 API ========== */

/**
 * @brief 初始化词法分析器
 *
 * @param lexer 词法分析器状态
 * @param source 源代码文本 (必须以 \0 结尾，生命周期由调用者管理)
 */
void cel_lexer_init(cel_lexer_t *lexer, const char *source);

/**
 * @brief 扫描下一个 Token
 *
 * @param lexer 词法分析器状态
 * @param token 输出 Token (由调用者分配)
 * @return true 成功扫描到 Token, false 到达文件末尾
 *
 * @note Token 中的字符串指针指向源代码,无需单独释放
 * @note 遇到词法错误时,返回 CEL_TOKEN_ERROR 类型的 Token
 */
bool cel_lexer_next_token(cel_lexer_t *lexer, cel_token_t *token);

/**
 * @brief 预览下一个 Token (不移动扫描位置)
 *
 * @param lexer 词法分析器状态
 * @param token 输出 Token
 * @return true 成功预览到 Token, false 到达文件末尾
 */
bool cel_lexer_peek_token(cel_lexer_t *lexer, cel_token_t *token);

/**
 * @brief 跳过当前 Token (配合 peek 使用)
 *
 * @param lexer 词法分析器状态
 */
void cel_lexer_skip_token(cel_lexer_t *lexer);

/**
 * @brief 检查词法分析器是否到达末尾
 *
 * @param lexer 词法分析器状态
 * @return true 已到达末尾
 */
bool cel_lexer_is_at_end(const cel_lexer_t *lexer);

#ifdef __cplusplus
}
#endif

#endif /* CEL_LEXER_H */
