/**
 * @file cel_parser_api.c
 * @brief CEL Parser 高层 API 实现
 *
 * 实现解析器的高层封装,包括错误收集和格式化。
 */

#include "cel/cel_parser.h"
#include "cel/cel_macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* strdup 是 POSIX 函数,需要 _POSIX_C_SOURCE */
#ifndef _POSIX_C_SOURCE
char *strdup(const char *s)
{
	size_t len = strlen(s) + 1;
	char *dup = malloc(len);
	if (dup) {
		memcpy(dup, s, len);
	}
	return dup;
}
#endif

/* ========== 内部辅助函数 ========== */

/* 预留给未来的错误收集功能 */
#if 0
/**
 * @brief 添加解析错误到列表
 */
static void add_parse_error(cel_parser_t *parser, const char *message,
			     const cel_source_range_t *location)
{
	cel_parse_error_t *error = malloc(sizeof(cel_parse_error_t));
	if (!error) {
		return; /* 内存不足,忽略 */
	}

	error->message = strdup(message);
	error->location = *location;
	error->next = NULL;

	/* 添加到链表 */
	if (parser->errors_tail) {
		parser->errors_tail->next = error;
	} else {
		parser->errors = error;
	}
	parser->errors_tail = error;
	parser->error_count++;
}
#endif

/* ========== 源位置工具 ========== */

cel_source_location_t cel_source_location_from_token(const cel_token_t *token)
{
	cel_source_location_t loc;
	loc.line = token->loc.line;
	loc.column = token->loc.column;
	loc.offset = token->loc.offset;
	return loc;
}

cel_source_range_t cel_source_range_from_token(const cel_token_t *token)
{
	cel_source_range_t range;
	range.start.line = token->loc.line;
	range.start.column = token->loc.column;
	range.start.offset = token->loc.offset;
	range.end.line = token->loc.line;
	range.end.column = token->loc.column + token->loc.length;
	range.end.offset = token->loc.offset + token->loc.length;
	return range;
}

/* ========== 高层 API ========== */

cel_parse_result_t cel_parse(const char *source)
{
	return cel_parse_with_options(source, 0);
}

cel_parse_result_t cel_parse_with_options(const char *source, size_t max_recursion)
{
	cel_parse_result_t result;
	result.ast = NULL;
	result.errors = NULL;
	result.error_count = 0;
	result.has_errors = false;

	if (!source) {
		result.has_errors = true;
		result.error_count = 1;
		result.errors = malloc(sizeof(cel_parse_error_t));
		if (result.errors) {
			result.errors->message = strdup("Source code is NULL");
			result.errors->location.start.line = 0;
			result.errors->location.start.column = 0;
			result.errors->location.start.offset = 0;
			result.errors->location.end = result.errors->location.start;
			result.errors->next = NULL;
		}
		return result;
	}

	/* 初始化词法分析器 */
	cel_lexer_t lexer;
	cel_lexer_init(&lexer, source);

	/* 初始化解析器 */
	cel_parser_t parser;
	cel_parser_init(&parser, &lexer);
	if (max_recursion > 0) {
		cel_parser_set_max_recursion(&parser, max_recursion);
	}

	/* 解析表达式 */
	result.ast = cel_parser_parse(&parser);

	/* 收集错误 */
	if (parser.error_count > 0) {
		result.has_errors = true;
		result.errors = parser.errors;
		result.error_count = parser.error_count;
	} else if (parser.error) {
		/* 兼容旧的单错误 API */
		result.has_errors = true;
		result.error_count = 1;
		result.errors = malloc(sizeof(cel_parse_error_t));
		if (result.errors) {
			result.errors->message = strdup(parser.error->message);
			result.errors->location.start.line = 1;
			result.errors->location.start.column = 1;
			result.errors->location.start.offset = 0;
			result.errors->location.end = result.errors->location.start;
			result.errors->next = NULL;
		}
		cel_error_destroy(parser.error);
	}

	/* 注意: 宏展开应该在解析器内部完成,而不是在这里 */
	/* 因为宏展开需要在解析时进行,以便生成正确的 AST 结构 */

	return result;
}

/* ========== 解析结果管理 ========== */

void cel_parse_result_destroy(cel_parse_result_t *result)
{
	if (!result) {
		return;
	}

	/* 销毁 AST */
	if (result->ast) {
		cel_ast_destroy(result->ast);
		result->ast = NULL;
	}

	/* 销毁错误列表 */
	cel_parse_error_t *error = result->errors;
	while (error) {
		cel_parse_error_t *next = error->next;
		free(error->message);
		free(error);
		error = next;
	}
	result->errors = NULL;
	result->error_count = 0;
	result->has_errors = false;
}

/* ========== 错误格式化 ========== */

char *cel_parse_error_format(const cel_parse_error_t *error, const char *source)
{
	if (!error) {
		return NULL;
	}

	/* 计算需要的缓冲区大小 */
	size_t buf_size = 512;
	char *buffer = malloc(buf_size);
	if (!buffer) {
		return NULL;
	}

	/* 格式化错误头 */
	int written = snprintf(buffer, buf_size,
			       "Parse error at line %zu, column %zu:\n  %s\n",
			       error->location.start.line,
			       error->location.start.column, error->message);

	if (written < 0 || (size_t)written >= buf_size) {
		/* 缓冲区不够,重新分配 */
		buf_size = written + 256;
		char *new_buffer = realloc(buffer, buf_size);
		if (!new_buffer) {
			free(buffer);
			return NULL;
		}
		buffer = new_buffer;
		snprintf(buffer, buf_size,
			 "Parse error at line %zu, column %zu:\n  %s\n",
			 error->location.start.line,
			 error->location.start.column, error->message);
	}

	/* 如果有源代码,添加上下文 */
	if (source && error->location.start.line > 0) {
		/* 找到错误所在行 */
		const char *line_start = source;
		size_t current_line = 1;

		/* 跳到错误行 */
		while (current_line < error->location.start.line && *line_start) {
			if (*line_start == '\n') {
				current_line++;
			}
			line_start++;
		}

		/* 找到行尾 */
		const char *line_end = line_start;
		while (*line_end && *line_end != '\n') {
			line_end++;
		}

		/* 计算行长度 */
		size_t line_len = line_end - line_start;
		if (line_len > 0 && line_len < 200) {
			/* 追加源代码行 */
			size_t current_len = strlen(buffer);
			size_t needed = current_len + line_len + 100;
			if (needed > buf_size) {
				char *new_buffer = realloc(buffer, needed);
				if (new_buffer) {
					buffer = new_buffer;
					buf_size = needed;
				}
			}

			/* 添加源代码行 */
			strncat(buffer, "  ", buf_size - strlen(buffer) - 1);
			strncat(buffer, line_start, line_len);
			strncat(buffer, "\n", buf_size - strlen(buffer) - 1);

			/* 添加指示符 */
			strncat(buffer, "  ", buf_size - strlen(buffer) - 1);
			for (size_t i = 1; i < error->location.start.column &&
					   strlen(buffer) < buf_size - 2;
			     i++) {
				strcat(buffer, " ");
			}
			strncat(buffer, "^\n", buf_size - strlen(buffer) - 1);
		}
	}

	return buffer;
}

char *cel_parse_result_format_errors(const cel_parse_result_t *result,
				      const char *source)
{
	if (!result || !result->has_errors) {
		return NULL;
	}

	/* 估算总大小 */
	size_t total_size = 1024;
	char *buffer = malloc(total_size);
	if (!buffer) {
		return NULL;
	}
	buffer[0] = '\0';

	/* 格式化每个错误 */
	size_t error_num = 1;
	for (cel_parse_error_t *error = result->errors; error; error = error->next) {
		char *error_str = cel_parse_error_format(error, source);
		if (error_str) {
			/* 添加错误编号 */
			char header[64];
			snprintf(header, sizeof(header), "[Error %zu/%zu]\n",
				 error_num, result->error_count);

			size_t needed = strlen(buffer) + strlen(header) +
					strlen(error_str) + 2;
			if (needed > total_size) {
				total_size = needed * 2;
				char *new_buffer = realloc(buffer, total_size);
				if (!new_buffer) {
					free(buffer);
					free(error_str);
					return NULL;
				}
				buffer = new_buffer;
			}

			strcat(buffer, header);
			strcat(buffer, error_str);
			strcat(buffer, "\n");
			free(error_str);
		}
		error_num++;
	}

	return buffer;
}
