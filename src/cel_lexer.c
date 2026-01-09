/**
 * @file cel_lexer.c
 * @brief CEL 词法分析器实现
 */

#include "cel/cel_lexer.h"
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========== 辅助函数声明 ========== */

static bool is_at_end(const cel_lexer_t *lexer);
static char peek(const cel_lexer_t *lexer);
static char peek_next(const cel_lexer_t *lexer);
static char advance(cel_lexer_t *lexer);
static bool match(cel_lexer_t *lexer, char expected);
static void skip_whitespace(cel_lexer_t *lexer);
static void skip_line_comment(cel_lexer_t *lexer);
static cel_token_t make_token(cel_lexer_t *lexer, cel_token_type_e type);
static cel_token_t make_error_token(cel_lexer_t *lexer, const char *message);
static cel_token_t scan_number(cel_lexer_t *lexer);
static cel_token_t scan_string(cel_lexer_t *lexer);
static cel_token_t scan_bytes(cel_lexer_t *lexer);
static cel_token_t scan_identifier(cel_lexer_t *lexer);
static cel_token_type_e check_keyword(const char *start, size_t length,
				       const char *rest, cel_token_type_e type);
static cel_token_type_e identifier_type(const char *start, size_t length);

/* ========== Token 辅助函数实现 ========== */

const char *cel_token_type_name(cel_token_type_e type)
{
	switch (type) {
	case CEL_TOKEN_EOF:
		return "EOF";
	case CEL_TOKEN_ERROR:
		return "ERROR";
	case CEL_TOKEN_INT:
		return "INT";
	case CEL_TOKEN_UINT:
		return "UINT";
	case CEL_TOKEN_DOUBLE:
		return "DOUBLE";
	case CEL_TOKEN_STRING:
		return "STRING";
	case CEL_TOKEN_BYTES:
		return "BYTES";
	case CEL_TOKEN_TRUE:
		return "TRUE";
	case CEL_TOKEN_FALSE:
		return "FALSE";
	case CEL_TOKEN_NULL:
		return "NULL";
	case CEL_TOKEN_IDENTIFIER:
		return "IDENTIFIER";
	case CEL_TOKEN_PLUS:
		return "+";
	case CEL_TOKEN_MINUS:
		return "-";
	case CEL_TOKEN_STAR:
		return "*";
	case CEL_TOKEN_SLASH:
		return "/";
	case CEL_TOKEN_PERCENT:
		return "%";
	case CEL_TOKEN_EQUAL_EQUAL:
		return "==";
	case CEL_TOKEN_BANG_EQUAL:
		return "!=";
	case CEL_TOKEN_LESS:
		return "<";
	case CEL_TOKEN_LESS_EQUAL:
		return "<=";
	case CEL_TOKEN_GREATER:
		return ">";
	case CEL_TOKEN_GREATER_EQUAL:
		return ">=";
	case CEL_TOKEN_AND_AND:
		return "&&";
	case CEL_TOKEN_OR_OR:
		return "||";
	case CEL_TOKEN_BANG:
		return "!";
	case CEL_TOKEN_QUESTION:
		return "?";
	case CEL_TOKEN_COLON:
		return ":";
	case CEL_TOKEN_DOT:
		return ".";
	case CEL_TOKEN_DOT_QUESTION:
		return ".?";
	case CEL_TOKEN_LBRACKET:
		return "[";
	case CEL_TOKEN_RBRACKET:
		return "]";
	case CEL_TOKEN_LBRACKET_QUESTION:
		return "[?";
	case CEL_TOKEN_LPAREN:
		return "(";
	case CEL_TOKEN_RPAREN:
		return ")";
	case CEL_TOKEN_LBRACE:
		return "{";
	case CEL_TOKEN_RBRACE:
		return "}";
	case CEL_TOKEN_COMMA:
		return ",";
	case CEL_TOKEN_IN:
		return "in";
	default:
		return "<unknown>";
	}
}

bool cel_token_is_literal(cel_token_type_e type)
{
	return type >= CEL_TOKEN_INT && type <= CEL_TOKEN_NULL;
}

bool cel_token_is_operator(cel_token_type_e type)
{
	return (type >= CEL_TOKEN_PLUS && type <= CEL_TOKEN_COMMA) ||
	       type == CEL_TOKEN_IN;
}

bool cel_token_is_keyword(cel_token_type_e type)
{
	return type == CEL_TOKEN_TRUE || type == CEL_TOKEN_FALSE ||
	       type == CEL_TOKEN_NULL || type == CEL_TOKEN_IN;
}

/* ========== 词法分析器实现 ========== */

void cel_lexer_init(cel_lexer_t *lexer, const char *source)
{
	lexer->source = source;
	lexer->start = source;
	lexer->current = source;
	lexer->line = 1;
	lexer->column = 1;
	lexer->line_start = 0;
}

bool cel_lexer_next_token(cel_lexer_t *lexer, cel_token_t *token)
{
	skip_whitespace(lexer);

	lexer->start = lexer->current;

	if (is_at_end(lexer)) {
		*token = make_token(lexer, CEL_TOKEN_EOF);
		return false;
	}

	char c = advance(lexer);

	/* 标识符和关键字 */
	if (isalpha(c) || c == '_') {
		*token = scan_identifier(lexer);
		return true;
	}

	/* 数字字面量 */
	if (isdigit(c)) {
		*token = scan_number(lexer);
		return true;
	}

	/* 运算符和符号 */
	switch (c) {
	case '+':
		*token = make_token(lexer, CEL_TOKEN_PLUS);
		return true;
	case '-':
		*token = make_token(lexer, CEL_TOKEN_MINUS);
		return true;
	case '*':
		*token = make_token(lexer, CEL_TOKEN_STAR);
		return true;
	case '/':
		/* 检查是否是注释 */
		if (match(lexer, '/')) {
			skip_line_comment(lexer);
			return cel_lexer_next_token(lexer, token);
		}
		*token = make_token(lexer, CEL_TOKEN_SLASH);
		return true;
	case '%':
		*token = make_token(lexer, CEL_TOKEN_PERCENT);
		return true;
	case '=':
		*token = make_token(lexer, match(lexer, '=') ?
						  CEL_TOKEN_EQUAL_EQUAL :
						  CEL_TOKEN_ERROR);
		if (token->type == CEL_TOKEN_ERROR) {
			token->value.str.str_value = "Unexpected '=' (use '==')";
		}
		return true;
	case '!':
		*token = make_token(lexer, match(lexer, '=') ?
						  CEL_TOKEN_BANG_EQUAL :
						  CEL_TOKEN_BANG);
		return true;
	case '<':
		*token = make_token(lexer, match(lexer, '=') ?
						  CEL_TOKEN_LESS_EQUAL :
						  CEL_TOKEN_LESS);
		return true;
	case '>':
		*token = make_token(lexer, match(lexer, '=') ?
						  CEL_TOKEN_GREATER_EQUAL :
						  CEL_TOKEN_GREATER);
		return true;
	case '&':
		if (!match(lexer, '&')) {
			*token = make_error_token(lexer,
						  "Unexpected '&' (use '&&')");
			return true;
		}
		*token = make_token(lexer, CEL_TOKEN_AND_AND);
		return true;
	case '|':
		if (!match(lexer, '|')) {
			*token = make_error_token(lexer,
						  "Unexpected '|' (use '||')");
			return true;
		}
		*token = make_token(lexer, CEL_TOKEN_OR_OR);
		return true;
	case '?':
		*token = make_token(lexer, CEL_TOKEN_QUESTION);
		return true;
	case ':':
		*token = make_token(lexer, CEL_TOKEN_COLON);
		return true;
	case '.':
		if (match(lexer, '?')) {
			*token = make_token(lexer, CEL_TOKEN_DOT_QUESTION);
		} else if (isdigit(peek(lexer))) {
			/* .123 是浮点数 */
			lexer->current--; /* 回退 */
			lexer->column--;
			*token = scan_number(lexer);
		} else {
			*token = make_token(lexer, CEL_TOKEN_DOT);
		}
		return true;
	case '[':
		*token = make_token(lexer, match(lexer, '?') ?
						  CEL_TOKEN_LBRACKET_QUESTION :
						  CEL_TOKEN_LBRACKET);
		return true;
	case ']':
		*token = make_token(lexer, CEL_TOKEN_RBRACKET);
		return true;
	case '(':
		*token = make_token(lexer, CEL_TOKEN_LPAREN);
		return true;
	case ')':
		*token = make_token(lexer, CEL_TOKEN_RPAREN);
		return true;
	case '{':
		*token = make_token(lexer, CEL_TOKEN_LBRACE);
		return true;
	case '}':
		*token = make_token(lexer, CEL_TOKEN_RBRACE);
		return true;
	case ',':
		*token = make_token(lexer, CEL_TOKEN_COMMA);
		return true;
	case '"':
		*token = scan_string(lexer);
		return true;
	case 'b':
		/* 检查是否是 bytes 字面量 */
		if (peek(lexer) == '"') {
			*token = scan_bytes(lexer);
			return true;
		}
		/* 否则是标识符 */
		*token = scan_identifier(lexer);
		return true;
	default:
		*token = make_error_token(lexer, "Unexpected character");
		return true;
	}
}

bool cel_lexer_peek_token(cel_lexer_t *lexer, cel_token_t *token)
{
	/* 保存当前状态 */
	cel_lexer_t saved = *lexer;

	/* 扫描下一个 Token */
	bool result = cel_lexer_next_token(lexer, token);

	/* 恢复状态 */
	*lexer = saved;

	return result;
}

void cel_lexer_skip_token(cel_lexer_t *lexer)
{
	cel_token_t token;
	cel_lexer_next_token(lexer, &token);
}

bool cel_lexer_is_at_end(const cel_lexer_t *lexer)
{
	return is_at_end(lexer);
}

/* ========== 私有辅助函数 ========== */

static bool is_at_end(const cel_lexer_t *lexer)
{
	return *lexer->current == '\0';
}

static char peek(const cel_lexer_t *lexer)
{
	return *lexer->current;
}

static char peek_next(const cel_lexer_t *lexer)
{
	if (is_at_end(lexer)) {
		return '\0';
	}
	return lexer->current[1];
}

static char advance(cel_lexer_t *lexer)
{
	char c = *lexer->current++;
	lexer->column++;
	return c;
}

static bool match(cel_lexer_t *lexer, char expected)
{
	if (is_at_end(lexer)) {
		return false;
	}
	if (*lexer->current != expected) {
		return false;
	}
	lexer->current++;
	lexer->column++;
	return true;
}

static void skip_whitespace(cel_lexer_t *lexer)
{
	for (;;) {
		char c = peek(lexer);
		switch (c) {
		case ' ':
		case '\r':
		case '\t':
			advance(lexer);
			break;
		case '\n':
			lexer->line++;
			lexer->column = 0; /* 下一个 advance 会 +1 */
			lexer->line_start = (lexer->current - lexer->source) + 1;
			advance(lexer);
			break;
		default:
			return;
		}
	}
}

static void skip_line_comment(cel_lexer_t *lexer)
{
	while (peek(lexer) != '\n' && !is_at_end(lexer)) {
		advance(lexer);
	}
}

static cel_token_t make_token(cel_lexer_t *lexer, cel_token_type_e type)
{
	cel_token_t token;
	token.type = type;
	token.loc.source = lexer->source;
	token.loc.line = lexer->line;
	token.loc.column = lexer->column - (lexer->current - lexer->start);
	token.loc.offset = lexer->start - lexer->source;
	token.loc.length = lexer->current - lexer->start;

	/* 默认初始化值 */
	token.value.str.str_value = NULL;
	token.value.str.str_length = 0;

	return token;
}

static cel_token_t make_error_token(cel_lexer_t *lexer, const char *message)
{
	cel_token_t token = make_token(lexer, CEL_TOKEN_ERROR);
	token.value.str.str_value = message;
	return token;
}

static cel_token_t scan_number(cel_lexer_t *lexer)
{
	bool is_hex = false;
	bool is_float = false;

	/* 检查十六进制 */
	if (peek(lexer) == 'x' || peek(lexer) == 'X') {
		if (lexer->current - lexer->start == 1 &&
		    lexer->start[0] == '0') {
			is_hex = true;
			advance(lexer); /* 跳过 x/X */
		}
	}

	/* 扫描数字 */
	if (is_hex) {
		while (isxdigit(peek(lexer))) {
			advance(lexer);
		}
	} else {
		/* 扫描整数部分 */
		while (isdigit(peek(lexer))) {
			advance(lexer);
		}

		/* 检查小数点 */
		if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
			is_float = true;
			advance(lexer); /* 跳过 . */

			while (isdigit(peek(lexer))) {
				advance(lexer);
			}
		}

		/* 检查指数 */
		if (peek(lexer) == 'e' || peek(lexer) == 'E') {
			is_float = true;
			advance(lexer); /* 跳过 e/E */

			if (peek(lexer) == '+' || peek(lexer) == '-') {
				advance(lexer);
			}

			while (isdigit(peek(lexer))) {
				advance(lexer);
			}
		}
	}

	/* 检查无符号后缀 */
	bool is_unsigned = false;
	if (peek(lexer) == 'u' || peek(lexer) == 'U') {
		is_unsigned = true;
		advance(lexer);
	}

	cel_token_t token = make_token(lexer, is_float ? CEL_TOKEN_DOUBLE :
					       is_unsigned ? CEL_TOKEN_UINT :
							     CEL_TOKEN_INT);

	/* 解析值 */
	size_t length = lexer->current - lexer->start;
	if (is_unsigned) {
		length--; /* 不包含 'u' 后缀 */
	}

	char buffer[64];
	if (length >= sizeof(buffer)) {
		return make_error_token(lexer, "Number too long");
	}

	memcpy(buffer, lexer->start, length);
	buffer[length] = '\0';

	char *endptr;
	errno = 0;

	if (is_float) {
		token.value.double_value = strtod(buffer, &endptr);
		if (errno == ERANGE) {
			return make_error_token(lexer,
						"Float out of range");
		}
	} else if (is_unsigned) {
		token.value.uint_value = strtoull(buffer, &endptr, is_hex ? 16 : 10);
		if (errno == ERANGE) {
			return make_error_token(lexer,
						"Unsigned integer out of range");
		}
	} else {
		token.value.int_value = strtoll(buffer, &endptr, is_hex ? 16 : 10);
		if (errno == ERANGE) {
			return make_error_token(lexer,
						"Integer out of range");
		}
	}

	return token;
}

static cel_token_t scan_string(cel_lexer_t *lexer)
{
	/* 字符串已经由 " 开始 */
	while (!is_at_end(lexer) && peek(lexer) != '"') {
		if (peek(lexer) == '\n') {
			return make_error_token(
				lexer, "Unterminated string (newline)");
		}

		if (peek(lexer) == '\\') {
			advance(lexer); /* 跳过 \ */
			if (is_at_end(lexer)) {
				return make_error_token(
					lexer, "Unterminated escape sequence");
			}
			advance(lexer); /* 跳过转义字符 */
		} else {
			advance(lexer);
		}
	}

	if (is_at_end(lexer)) {
		return make_error_token(lexer, "Unterminated string");
	}

	/* 跳过结束的 " */
	advance(lexer);

	cel_token_t token = make_token(lexer, CEL_TOKEN_STRING);
	/* 字符串内容 (不包含引号) */
	token.value.str.str_value = lexer->start + 1;
	token.value.str.str_length = (lexer->current - lexer->start) - 2;

	return token;
}

static cel_token_t scan_bytes(cel_lexer_t *lexer)
{
	/* b 已经被扫描，现在期望 " */
	if (peek(lexer) != '"') {
		return make_error_token(lexer,
					"Expected '\"' after 'b' for bytes literal");
	}

	advance(lexer); /* 跳过 " */

	/* 扫描字符串内容 */
	while (!is_at_end(lexer) && peek(lexer) != '"') {
		if (peek(lexer) == '\n') {
			return make_error_token(
				lexer, "Unterminated bytes literal (newline)");
		}

		if (peek(lexer) == '\\') {
			advance(lexer); /* 跳过 \ */
			if (is_at_end(lexer)) {
				return make_error_token(
					lexer, "Unterminated escape sequence");
			}
			advance(lexer); /* 跳过转义字符 */
		} else {
			advance(lexer);
		}
	}

	if (is_at_end(lexer)) {
		return make_error_token(lexer, "Unterminated bytes literal");
	}

	/* 跳过结束的 " */
	advance(lexer);

	cel_token_t token = make_token(lexer, CEL_TOKEN_BYTES);
	/* 字节内容 (不包含 b"...") */
	token.value.str.str_value = lexer->start + 2; /* 跳过 b" */
	token.value.str.str_length = (lexer->current - lexer->start) - 3; /* 减去 b" 和 " */

	return token;
}

static cel_token_t scan_identifier(cel_lexer_t *lexer)
{
	/* 检查是否是 bytes 字面量 (b"...") */
	if (lexer->start[0] == 'b' && peek(lexer) == '"') {
		/* 直接调用 scan_bytes，它会处理 " */
		return scan_bytes(lexer);
	}

	/* 扫描标识符 */
	while (isalnum(peek(lexer)) || peek(lexer) == '_') {
		advance(lexer);
	}

	cel_token_t token = make_token(lexer,
				       identifier_type(lexer->start,
						       lexer->current - lexer->start));

	/* 对于标识符，保存文本 */
	if (token.type == CEL_TOKEN_IDENTIFIER) {
		token.value.str.str_value = lexer->start;
		token.value.str.str_length = lexer->current - lexer->start;
	}

	return token;
}

__attribute__((unused))
static cel_token_type_e check_keyword(const char *start, size_t length,
				       const char *rest, cel_token_type_e type)
{
	size_t rest_len = strlen(rest);
	if (length == rest_len + 1 &&
	    memcmp(start + 1, rest, rest_len) == 0) {
		return type;
	}
	return CEL_TOKEN_IDENTIFIER;
}

static cel_token_type_e identifier_type(const char *start, size_t length)
{
	/* 使用 trie 结构识别关键字 */
	switch (start[0]) {
	case 't':
		if (length == 4 && memcmp(start, "true", 4) == 0) {
			return CEL_TOKEN_TRUE;
		}
		break;
	case 'f':
		if (length == 5 && memcmp(start, "false", 5) == 0) {
			return CEL_TOKEN_FALSE;
		}
		break;
	case 'n':
		if (length == 4 && memcmp(start, "null", 4) == 0) {
			return CEL_TOKEN_NULL;
		}
		break;
	case 'i':
		if (length == 2 && memcmp(start, "in", 2) == 0) {
			return CEL_TOKEN_IN;
		}
		break;
	}

	return CEL_TOKEN_IDENTIFIER;
}
