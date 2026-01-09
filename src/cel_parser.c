/**
 * @file cel_parser.c
 * @brief CEL Parser 实现 (Pratt Parser)
 */

#include "cel/cel_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========== 运算符优先级 ========== */

typedef enum {
	PREC_NONE,
	PREC_TERNARY,     /* ? : */
	PREC_OR,          /* || */
	PREC_AND,         /* && */
	PREC_EQUALITY,    /* == != */
	PREC_COMPARISON,  /* < <= > >= */
	PREC_TERM,        /* + - */
	PREC_FACTOR,      /* * / % */
	PREC_UNARY,       /* ! - */
	PREC_POSTFIX,     /* . [] () */
	PREC_PRIMARY
} precedence_e;

/* ========== 前向声明 ========== */

static cel_ast_node_t *parse_expression(cel_parser_t *parser);
static cel_ast_node_t *parse_precedence(cel_parser_t *parser, precedence_e precedence);
static cel_ast_node_t *parse_primary(cel_parser_t *parser);
static cel_ast_node_t *parse_postfix(cel_parser_t *parser, cel_ast_node_t *left);
static cel_ast_node_t *parse_binary(cel_parser_t *parser, cel_ast_node_t *left, precedence_e precedence);
static cel_ast_node_t *parse_ternary(cel_parser_t *parser, cel_ast_node_t *condition);
static cel_ast_node_t *parse_list(cel_parser_t *parser);
static cel_ast_node_t *parse_map(cel_parser_t *parser);

static void advance(cel_parser_t *parser);
static bool check(const cel_parser_t *parser, cel_token_type_e type);
static bool match(cel_parser_t *parser, cel_token_type_e type);
static void consume(cel_parser_t *parser, cel_token_type_e type, const char *message);
static void error_at_current(cel_parser_t *parser, const char *message);
static void error_at(cel_parser_t *parser, cel_token_t *token, const char *message);
static precedence_e get_precedence(cel_token_type_e type);

/* ========== Parser 初始化 ========== */

void cel_parser_init(cel_parser_t *parser, cel_lexer_t *lexer)
{
	parser->lexer = lexer;
	parser->had_error = false;
	parser->panic_mode = false;
	parser->error = NULL;
	parser->errors = NULL;
	parser->errors_tail = NULL;
	parser->error_count = 0;
	parser->recursion_depth = 0;
	parser->max_recursion = 100;

	/* 初始化 Token */
	parser->current.type = CEL_TOKEN_EOF;
	parser->previous.type = CEL_TOKEN_EOF;
}

void cel_parser_cleanup(cel_parser_t *parser)
{
	if (!parser) {
		return;
	}

	/* 释放单一错误 */
	if (parser->error) {
		cel_error_destroy(parser->error);
		parser->error = NULL;
	}

	/* 释放错误列表 */
	cel_parse_error_t *err = parser->errors;
	while (err) {
		cel_parse_error_t *next = err->next;
		free(err->message);
		free(err);
		err = next;
	}
	parser->errors = NULL;
	parser->errors_tail = NULL;
	parser->error_count = 0;
}

void cel_parser_set_max_recursion(cel_parser_t *parser, size_t max_depth)
{
	parser->max_recursion = max_depth;
}

cel_error_t *cel_parser_get_error(const cel_parser_t *parser)
{
	return parser->error;
}

/* ========== Parser 主函数 ========== */

cel_ast_node_t *cel_parser_parse(cel_parser_t *parser)
{
	/* 读取第一个 Token */
	advance(parser);

	if (parser->current.type == CEL_TOKEN_EOF) {
		error_at_current(parser, "Empty expression");
		return NULL;
	}

	cel_ast_node_t *ast = parse_expression(parser);

	if (parser->had_error) {
		if (ast) {
			cel_ast_destroy(ast);
		}
		return NULL;
	}

	/* 检查是否到达末尾 */
	if (parser->current.type != CEL_TOKEN_EOF) {
		error_at_current(parser, "Unexpected token after expression");
		if (ast) {
			cel_ast_destroy(ast);
		}
		return NULL;
	}

	return ast;
}

/* ========== 表达式解析 ========== */

static cel_ast_node_t *parse_expression(cel_parser_t *parser)
{
	return parse_precedence(parser, PREC_TERNARY);
}

static cel_ast_node_t *parse_precedence(cel_parser_t *parser, precedence_e precedence)
{
	/* 检查递归深度 */
	if (++parser->recursion_depth > parser->max_recursion) {
		error_at_current(parser, "Expression too deeply nested");
		parser->recursion_depth--;
		return NULL;
	}

	/* 解析前缀表达式 */
	cel_ast_node_t *left = NULL;

	switch (parser->current.type) {
	case CEL_TOKEN_INT:
	case CEL_TOKEN_UINT:
	case CEL_TOKEN_DOUBLE:
	case CEL_TOKEN_STRING:
	case CEL_TOKEN_BYTES:
	case CEL_TOKEN_TRUE:
	case CEL_TOKEN_FALSE:
	case CEL_TOKEN_NULL:
	case CEL_TOKEN_IDENTIFIER:
	case CEL_TOKEN_LPAREN:
	case CEL_TOKEN_LBRACKET:
	case CEL_TOKEN_LBRACE:
		left = parse_primary(parser);
		break;

	case CEL_TOKEN_MINUS:
	case CEL_TOKEN_BANG:
		/* 一元运算符 */
		{
			cel_token_t op_token = parser->current;
			advance(parser);

			cel_ast_node_t *operand =
				parse_precedence(parser, PREC_UNARY);
			if (!operand) {
				parser->recursion_depth--;
				return NULL;
			}

			cel_unary_op_e op = (op_token.type == CEL_TOKEN_MINUS) ?
						    CEL_UNARY_NEG :
						    CEL_UNARY_NOT;
			left = cel_ast_create_unary(op, operand, op_token.loc);
		}
		break;

	default:
		error_at_current(parser, "Expected expression");
		parser->recursion_depth--;
		return NULL;
	}

	if (!left) {
		parser->recursion_depth--;
		return NULL;
	}

	/* 解析中缀和后缀表达式 */
	while (precedence <= get_precedence(parser->current.type)) {
		switch (parser->current.type) {
		/* 后缀运算符 */
		case CEL_TOKEN_DOT:
		case CEL_TOKEN_DOT_QUESTION:
		case CEL_TOKEN_LBRACKET:
		case CEL_TOKEN_LBRACKET_QUESTION:
		case CEL_TOKEN_LPAREN:
			left = parse_postfix(parser, left);
			if (!left) {
				parser->recursion_depth--;
				return NULL;
			}
			break;

		/* 三元运算符 */
		case CEL_TOKEN_QUESTION:
			left = parse_ternary(parser, left);
			if (!left) {
				parser->recursion_depth--;
				return NULL;
			}
			break;

		/* 二元运算符 */
		default:
			left = parse_binary(parser, left, precedence);
			if (!left) {
				parser->recursion_depth--;
				return NULL;
			}
			break;
		}
	}

	parser->recursion_depth--;
	return left;
}

/* ========== 主表达式解析 ========== */

static cel_ast_node_t *parse_primary(cel_parser_t *parser)
{
	cel_token_t token = parser->current;
	advance(parser);

	switch (token.type) {
	/* 整数字面量 */
	case CEL_TOKEN_INT: {
		cel_value_t value = cel_value_int(token.value.int_value);
		return cel_ast_create_literal(value, token.loc);
	}

	/* 无符号整数字面量 */
	case CEL_TOKEN_UINT: {
		cel_value_t value = cel_value_uint(token.value.uint_value);
		return cel_ast_create_literal(value, token.loc);
	}

	/* 浮点数字面量 */
	case CEL_TOKEN_DOUBLE: {
		cel_value_t value = cel_value_double(token.value.double_value);
		return cel_ast_create_literal(value, token.loc);
	}

	/* 字符串字面量 */
	case CEL_TOKEN_STRING: {
		cel_value_t value = cel_value_string_n(token.value.str.str_value,
							token.value.str.str_length);
		return cel_ast_create_literal(value, token.loc);
	}

	/* 字节字面量 */
	case CEL_TOKEN_BYTES: {
		cel_value_t value = cel_value_bytes(
			(const uint8_t *)token.value.str.str_value,
			token.value.str.str_length);
		return cel_ast_create_literal(value, token.loc);
	}

	/* 布尔值 */
	case CEL_TOKEN_TRUE: {
		cel_value_t value = cel_value_bool(true);
		return cel_ast_create_literal(value, token.loc);
	}

	case CEL_TOKEN_FALSE: {
		cel_value_t value = cel_value_bool(false);
		return cel_ast_create_literal(value, token.loc);
	}

	/* null */
	case CEL_TOKEN_NULL: {
		cel_value_t value = cel_value_null();
		return cel_ast_create_literal(value, token.loc);
	}

	/* 标识符 */
	case CEL_TOKEN_IDENTIFIER:
		return cel_ast_create_ident(token.value.str.str_value,
					     token.value.str.str_length, token.loc);

	/* 括号表达式 */
	case CEL_TOKEN_LPAREN: {
		cel_ast_node_t *expr = parse_expression(parser);
		if (!expr) {
			return NULL;
		}
		consume(parser, CEL_TOKEN_RPAREN, "Expected ')' after expression");
		return expr;
	}

	/* 列表字面量 */
	case CEL_TOKEN_LBRACKET:
		return parse_list(parser);

	/* Map 字面量 */
	case CEL_TOKEN_LBRACE:
		return parse_map(parser);

	default:
		error_at(parser, &token, "Unexpected token in expression");
		return NULL;
	}
}

/* ========== 后缀表达式解析 ========== */

static cel_ast_node_t *parse_postfix(cel_parser_t *parser,
				      cel_ast_node_t *left)
{
	switch (parser->current.type) {
	/* 字段访问: obj.field, obj.?field */
	case CEL_TOKEN_DOT:
	case CEL_TOKEN_DOT_QUESTION: {
		bool optional = (parser->current.type == CEL_TOKEN_DOT_QUESTION);
		cel_token_location_t loc = parser->current.loc;
		advance(parser);

		if (parser->current.type != CEL_TOKEN_IDENTIFIER) {
			error_at_current(parser, "Expected field name after '.'");
			cel_ast_destroy(left);
			return NULL;
		}

		cel_token_t field_token = parser->current;
		advance(parser);

		return cel_ast_create_select(left, field_token.value.str.str_value,
					      field_token.value.str.str_length,
					      optional, loc);
	}

	/* 索引访问: list[0], map["key"], list[?0] */
	case CEL_TOKEN_LBRACKET:
	case CEL_TOKEN_LBRACKET_QUESTION: {
		bool optional = (parser->current.type == CEL_TOKEN_LBRACKET_QUESTION);
		cel_token_location_t loc = parser->current.loc;
		advance(parser);

		cel_ast_node_t *index = parse_expression(parser);
		if (!index) {
			cel_ast_destroy(left);
			return NULL;
		}

		consume(parser, CEL_TOKEN_RBRACKET, "Expected ']' after index");

		return cel_ast_create_index(left, index, optional, loc);
	}

	/* 函数调用: func(arg1, arg2) */
	case CEL_TOKEN_LPAREN: {
		advance(parser);

		/* 解析参数列表 */
		cel_ast_node_t **args = NULL;
		size_t arg_count = 0;
		size_t arg_capacity = 0;

		if (parser->current.type != CEL_TOKEN_RPAREN) {
			do {
				/* 扩展参数数组 */
				if (arg_count >= arg_capacity) {
					arg_capacity = arg_capacity == 0 ? 4 : arg_capacity * 2;
					cel_ast_node_t **new_args = realloc(
						args, arg_capacity * sizeof(cel_ast_node_t *));
					if (!new_args) {
						for (size_t i = 0; i < arg_count; i++) {
							cel_ast_destroy(args[i]);
						}
						free(args);
						cel_ast_destroy(left);
						error_at_current(parser, "Out of memory");
						return NULL;
					}
					args = new_args;
				}

				/* 解析参数 */
				cel_ast_node_t *arg = parse_expression(parser);
				if (!arg) {
					for (size_t i = 0; i < arg_count; i++) {
						cel_ast_destroy(args[i]);
					}
					free(args);
					cel_ast_destroy(left);
					return NULL;
				}

				args[arg_count++] = arg;

			} while (match(parser, CEL_TOKEN_COMMA));
		}

		consume(parser, CEL_TOKEN_RPAREN, "Expected ')' after arguments");

		/* 判断是函数调用还是方法调用 */
		if (left->type == CEL_AST_IDENT) {
			/* 函数调用: func(args) */
			const char *func_name = left->as.ident.name;
			size_t func_length = left->as.ident.length;
			cel_token_location_t func_loc = left->loc;

			free(left); /* 释放标识符节点 */

			return cel_ast_create_call(func_name, func_length, NULL,
						   args, arg_count, func_loc);
		} else if (left->type == CEL_AST_SELECT) {
			/* 方法调用: obj.method(args) */
			/* SELECT 节点包含: operand (对象), field (方法名) */
			const char *method_name = left->as.select.field;
			size_t method_length = left->as.select.field_length;
			cel_ast_node_t *target = left->as.select.operand;
			cel_token_location_t call_loc = left->loc;

			/* 将 SELECT 节点转换为 CALL 节点 */
			/* target 已被保存，无需单独复制 */
			left->as.select.operand = NULL; /* 防止 destroy 时释放 */
			free(left); /* 释放 SELECT 节点本身 (field 是指向原始数据的指针) */

			return cel_ast_create_call(method_name, method_length, target,
						   args, arg_count, call_loc);
		} else {
			/* 其他类型不能作为调用目标 */
			error_at(parser, &parser->previous,
				 "Invalid call target");
			for (size_t i = 0; i < arg_count; i++) {
				cel_ast_destroy(args[i]);
			}
			free(args);
			cel_ast_destroy(left);
			return NULL;
		}
	}

	default:
		return left;
	}
}

/* ========== 二元运算符解析 ========== */

static cel_ast_node_t *parse_binary(cel_parser_t *parser,
				     cel_ast_node_t *left,
				     precedence_e precedence __attribute__((unused)))
{
	cel_token_t op_token = parser->current;
	advance(parser);

	/* 获取运算符类型 */
	cel_binary_op_e op;
	switch (op_token.type) {
	case CEL_TOKEN_PLUS:
		op = CEL_BINARY_ADD;
		break;
	case CEL_TOKEN_MINUS:
		op = CEL_BINARY_SUB;
		break;
	case CEL_TOKEN_STAR:
		op = CEL_BINARY_MUL;
		break;
	case CEL_TOKEN_SLASH:
		op = CEL_BINARY_DIV;
		break;
	case CEL_TOKEN_PERCENT:
		op = CEL_BINARY_MOD;
		break;
	case CEL_TOKEN_EQUAL_EQUAL:
		op = CEL_BINARY_EQ;
		break;
	case CEL_TOKEN_BANG_EQUAL:
		op = CEL_BINARY_NE;
		break;
	case CEL_TOKEN_LESS:
		op = CEL_BINARY_LT;
		break;
	case CEL_TOKEN_LESS_EQUAL:
		op = CEL_BINARY_LE;
		break;
	case CEL_TOKEN_GREATER:
		op = CEL_BINARY_GT;
		break;
	case CEL_TOKEN_GREATER_EQUAL:
		op = CEL_BINARY_GE;
		break;
	case CEL_TOKEN_AND_AND:
		op = CEL_BINARY_AND;
		break;
	case CEL_TOKEN_OR_OR:
		op = CEL_BINARY_OR;
		break;
	case CEL_TOKEN_IN:
		op = CEL_BINARY_IN;
		break;
	default:
		error_at(parser, &op_token, "Unknown binary operator");
		cel_ast_destroy(left);
		return NULL;
	}

	/* 解析右操作数 (左结合) */
	precedence_e next_prec = (precedence_e)(get_precedence(op_token.type) + 1);
	cel_ast_node_t *right = parse_precedence(parser, next_prec);
	if (!right) {
		cel_ast_destroy(left);
		return NULL;
	}

	return cel_ast_create_binary(op, left, right, op_token.loc);
}

/* ========== 三元运算符解析 ========== */

static cel_ast_node_t *parse_ternary(cel_parser_t *parser,
				      cel_ast_node_t *condition)
{
	cel_token_location_t loc = parser->current.loc;
	advance(parser); /* 跳过 ? */

	cel_ast_node_t *if_true = parse_expression(parser);
	if (!if_true) {
		cel_ast_destroy(condition);
		return NULL;
	}

	consume(parser, CEL_TOKEN_COLON, "Expected ':' in ternary expression");

	cel_ast_node_t *if_false = parse_precedence(parser, PREC_TERNARY);
	if (!if_false) {
		cel_ast_destroy(condition);
		cel_ast_destroy(if_true);
		return NULL;
	}

	return cel_ast_create_ternary(condition, if_true, if_false, loc);
}

/* ========== 列表字面量解析 ========== */

static cel_ast_node_t *parse_list(cel_parser_t *parser)
{
	cel_token_location_t loc = parser->previous.loc; /* [ 的位置 */

	cel_ast_node_t **elements = NULL;
	size_t element_count = 0;
	size_t element_capacity = 0;

	if (parser->current.type != CEL_TOKEN_RBRACKET) {
		do {
			/* 扩展元素数组 */
			if (element_count >= element_capacity) {
				element_capacity = element_capacity == 0 ? 4 : element_capacity * 2;
				cel_ast_node_t **new_elements = realloc(
					elements, element_capacity * sizeof(cel_ast_node_t *));
				if (!new_elements) {
					for (size_t i = 0; i < element_count; i++) {
						cel_ast_destroy(elements[i]);
					}
					free(elements);
					error_at_current(parser, "Out of memory");
					return NULL;
				}
				elements = new_elements;
			}

			/* 解析元素 */
			cel_ast_node_t *element = parse_expression(parser);
			if (!element) {
				for (size_t i = 0; i < element_count; i++) {
					cel_ast_destroy(elements[i]);
				}
				free(elements);
				return NULL;
			}

			elements[element_count++] = element;

		} while (match(parser, CEL_TOKEN_COMMA));
	}

	consume(parser, CEL_TOKEN_RBRACKET, "Expected ']' after list elements");

	return cel_ast_create_list(elements, element_count, loc);
}

/* ========== Map 字面量解析 ========== */

static cel_ast_node_t *parse_map(cel_parser_t *parser)
{
	cel_token_location_t loc = parser->previous.loc; /* { 的位置 */

	cel_ast_map_entry_t *entries = NULL;
	size_t entry_count = 0;
	size_t entry_capacity = 0;

	if (parser->current.type != CEL_TOKEN_RBRACE) {
		do {
			/* 扩展条目数组 */
			if (entry_count >= entry_capacity) {
				entry_capacity = entry_capacity == 0 ? 4 : entry_capacity * 2;
				cel_ast_map_entry_t *new_entries = realloc(
					entries, entry_capacity * sizeof(cel_ast_map_entry_t));
				if (!new_entries) {
					for (size_t i = 0; i < entry_count; i++) {
						cel_ast_destroy(entries[i].key);
						cel_ast_destroy(entries[i].value);
					}
					free(entries);
					error_at_current(parser, "Out of memory");
					return NULL;
				}
				entries = new_entries;
			}

			/* 解析键 */
			cel_ast_node_t *key = parse_expression(parser);
			if (!key) {
				for (size_t i = 0; i < entry_count; i++) {
					cel_ast_destroy(entries[i].key);
					cel_ast_destroy(entries[i].value);
				}
				free(entries);
				return NULL;
			}

			consume(parser, CEL_TOKEN_COLON, "Expected ':' after map key");

			/* 解析值 */
			cel_ast_node_t *value = parse_expression(parser);
			if (!value) {
				cel_ast_destroy(key);
				for (size_t i = 0; i < entry_count; i++) {
					cel_ast_destroy(entries[i].key);
					cel_ast_destroy(entries[i].value);
				}
				free(entries);
				return NULL;
			}

			entries[entry_count].key = key;
			entries[entry_count].value = value;
			entry_count++;

		} while (match(parser, CEL_TOKEN_COMMA));
	}

	consume(parser, CEL_TOKEN_RBRACE, "Expected '}' after map entries");

	return cel_ast_create_map(entries, entry_count, loc);
}

/* ========== 辅助函数 ========== */

static void advance(cel_parser_t *parser)
{
	parser->previous = parser->current;

	for (;;) {
		cel_lexer_next_token(parser->lexer, &parser->current);

		if (parser->current.type != CEL_TOKEN_ERROR) {
			break;
		}

		error_at_current(parser, parser->current.value.str.str_value);
	}
}

static bool check(const cel_parser_t *parser, cel_token_type_e type)
{
	return parser->current.type == type;
}

static bool match(cel_parser_t *parser, cel_token_type_e type)
{
	if (!check(parser, type)) {
		return false;
	}
	advance(parser);
	return true;
}

static void consume(cel_parser_t *parser, cel_token_type_e type,
		    const char *message)
{
	if (parser->current.type == type) {
		advance(parser);
		return;
	}

	error_at_current(parser, message);
}

static void error_at_current(cel_parser_t *parser, const char *message)
{
	error_at(parser, &parser->current, message);
}

static void error_at(cel_parser_t *parser, cel_token_t *token,
		     const char *message)
{
	if (parser->panic_mode) {
		return;
	}
	parser->panic_mode = true;
	parser->had_error = true;

	/* 创建错误对象 */
	char error_msg[256];
	snprintf(error_msg, sizeof(error_msg), "[line %zu, col %zu] Error: %s",
		 token->loc.line, token->loc.column, message);

	parser->error = cel_error_create(CEL_ERROR_PARSE, error_msg);
}

static precedence_e get_precedence(cel_token_type_e type)
{
	switch (type) {
	case CEL_TOKEN_QUESTION:
		return PREC_TERNARY;
	case CEL_TOKEN_OR_OR:
		return PREC_OR;
	case CEL_TOKEN_AND_AND:
		return PREC_AND;
	case CEL_TOKEN_EQUAL_EQUAL:
	case CEL_TOKEN_BANG_EQUAL:
		return PREC_EQUALITY;
	case CEL_TOKEN_LESS:
	case CEL_TOKEN_LESS_EQUAL:
	case CEL_TOKEN_GREATER:
	case CEL_TOKEN_GREATER_EQUAL:
	case CEL_TOKEN_IN:
		return PREC_COMPARISON;
	case CEL_TOKEN_PLUS:
	case CEL_TOKEN_MINUS:
		return PREC_TERM;
	case CEL_TOKEN_STAR:
	case CEL_TOKEN_SLASH:
	case CEL_TOKEN_PERCENT:
		return PREC_FACTOR;
	case CEL_TOKEN_DOT:
	case CEL_TOKEN_DOT_QUESTION:
	case CEL_TOKEN_LBRACKET:
	case CEL_TOKEN_LBRACKET_QUESTION:
	case CEL_TOKEN_LPAREN:
		return PREC_POSTFIX;
	default:
		return PREC_NONE;
	}
}
