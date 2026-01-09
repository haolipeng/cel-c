/**
 * @file cel_eval.c
 * @brief CEL 求值器实现
 */

#define _GNU_SOURCE  /* for timegm */

#include "cel/cel_eval.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef CEL_ENABLE_REGEX
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#endif

/* strndup 可能不在某些平台上可用 */
#ifndef _POSIX_C_SOURCE
static char *strndup(const char *s, size_t n)
{
	size_t len = strlen(s);
	if (len > n) {
		len = n;
	}
	char *result = malloc(len + 1);
	if (result) {
		memcpy(result, s, len);
		result[len] = '\0';
	}
	return result;
}
#endif

/* ========== 前向声明 ========== */

static bool eval_node(const cel_ast_node_t *node, cel_context_t *ctx,
		      cel_value_t *result);
static bool eval_unary(const cel_ast_unary_t *unary, cel_context_t *ctx,
			cel_value_t *result);
static bool eval_binary(const cel_ast_binary_t *binary, cel_context_t *ctx,
			 cel_value_t *result);
static bool eval_ternary(const cel_ast_ternary_t *ternary, cel_context_t *ctx,
			  cel_value_t *result);
static bool eval_select(const cel_ast_select_t *select, cel_context_t *ctx,
			 cel_value_t *result);
static bool eval_index(const cel_ast_index_t *index, cel_context_t *ctx,
			cel_value_t *result);
static bool eval_call(const cel_ast_call_t *call, cel_context_t *ctx,
		      cel_value_t *result);
static bool eval_list(const cel_ast_list_t *list, cel_context_t *ctx,
		      cel_value_t *result);
static bool eval_map(const cel_ast_map_t *map, cel_context_t *ctx,
		     cel_value_t *result);
static bool eval_comprehension(const cel_ast_comprehension_t *comp,
				 cel_context_t *ctx, cel_value_t *result);

static void set_error(cel_context_t *ctx, const char *message);

/* ========== 求值主函数 ========== */

bool cel_eval(const cel_ast_node_t *ast, cel_context_t *ctx,
	      cel_value_t *result)
{
	if (!ast || !ctx || !result) {
		return false;
	}

	/* TODO: 添加错误清除逻辑 */

	return eval_node(ast, ctx, result);
}

/* ========== 节点求值 ========== */

static bool eval_node(const cel_ast_node_t *node, cel_context_t *ctx,
		      cel_value_t *result)
{
	if (!node) {
		set_error(ctx, "NULL AST node");
		return false;
	}

	switch (node->type) {
	case CEL_AST_LITERAL:
		/* 字面量直接返回值 */
		*result = node->as.literal.value;
		return true;

	case CEL_AST_IDENT: {
		/* 标识符查找变量 */
		/* 构造 null 结尾的变量名 */
		char *var_name = strndup(node->as.ident.name, node->as.ident.length);
		if (!var_name) {
			set_error(ctx, "Out of memory");
			return false;
		}

		cel_value_t *value = cel_context_get_variable(ctx, var_name);
		free(var_name);

		if (!value) {
			char error_msg[256];
			snprintf(error_msg, sizeof(error_msg),
				 "Undefined variable: %.*s",
				 (int)node->as.ident.length,
				 node->as.ident.name);
			set_error(ctx, error_msg);
			return false;
		}

		/* 复制值 */
		*result = *value;
		return true;
	}

	case CEL_AST_UNARY:
		return eval_unary(&node->as.unary, ctx, result);

	case CEL_AST_BINARY:
		return eval_binary(&node->as.binary, ctx, result);

	case CEL_AST_TERNARY:
		return eval_ternary(&node->as.ternary, ctx, result);

	case CEL_AST_SELECT:
		return eval_select(&node->as.select, ctx, result);

	case CEL_AST_INDEX:
		return eval_index(&node->as.index, ctx, result);

	case CEL_AST_CALL:
		return eval_call(&node->as.call, ctx, result);

	case CEL_AST_LIST:
		return eval_list(&node->as.list, ctx, result);

	case CEL_AST_MAP:
		return eval_map(&node->as.map, ctx, result);

	case CEL_AST_STRUCT:
		set_error(ctx, "Struct literals not yet implemented");
		return false;

	case CEL_AST_COMPREHENSION:
		return eval_comprehension(&node->as.comprehension, ctx, result);

	default:
		set_error(ctx, "Unknown AST node type");
		return false;
	}
}

/* ========== 一元运算求值 ========== */

static bool eval_unary(const cel_ast_unary_t *unary, cel_context_t *ctx,
			cel_value_t *result)
{
	cel_value_t operand;
	if (!eval_node(unary->operand, ctx, &operand)) {
		return false;
	}

	switch (unary->op) {
	case CEL_UNARY_NEG:
		/* 取负 */
		if (operand.type == CEL_TYPE_INT) {
			*result = cel_value_int(-operand.value.int_value);
			return true;
		} else if (operand.type == CEL_TYPE_DOUBLE) {
			*result = cel_value_double(-operand.value.double_value);
			return true;
		} else {
			set_error(ctx, "Negation requires numeric operand");
			return false;
		}

	case CEL_UNARY_NOT:
		/* 逻辑非 */
		if (operand.type == CEL_TYPE_BOOL) {
			*result = cel_value_bool(!operand.value.bool_value);
			return true;
		} else {
			set_error(ctx, "Logical NOT requires boolean operand");
			return false;
		}

	default:
		set_error(ctx, "Unknown unary operator");
		return false;
	}
}

/* ========== 二元运算求值 ========== */

static bool eval_binary(const cel_ast_binary_t *binary, cel_context_t *ctx,
			 cel_value_t *result)
{
	cel_value_t left, right;

	/* 短路求值 */
	if (binary->op == CEL_BINARY_AND || binary->op == CEL_BINARY_OR) {
		if (!eval_node(binary->left, ctx, &left)) {
			return false;
		}

		if (left.type != CEL_TYPE_BOOL) {
			set_error(ctx, "Logical operator requires boolean operands");
			return false;
		}

		/* 短路 */
		if (binary->op == CEL_BINARY_AND && !left.value.bool_value) {
			*result = cel_value_bool(false);
			return true;
		}
		if (binary->op == CEL_BINARY_OR && left.value.bool_value) {
			*result = cel_value_bool(true);
			return true;
		}

		if (!eval_node(binary->right, ctx, &right)) {
			return false;
		}

		if (right.type != CEL_TYPE_BOOL) {
			set_error(ctx, "Logical operator requires boolean operands");
			return false;
		}

		*result = cel_value_bool(right.value.bool_value);
		return true;
	}

	/* 普通二元运算 */
	if (!eval_node(binary->left, ctx, &left)) {
		return false;
	}
	if (!eval_node(binary->right, ctx, &right)) {
		return false;
	}

	/* 算术运算 */
	if (binary->op >= CEL_BINARY_ADD && binary->op <= CEL_BINARY_MOD) {
		if (left.type == CEL_TYPE_INT && right.type == CEL_TYPE_INT) {
			int64_t l = left.value.int_value;
			int64_t r = right.value.int_value;

			switch (binary->op) {
			case CEL_BINARY_ADD:
				*result = cel_value_int(l + r);
				return true;
			case CEL_BINARY_SUB:
				*result = cel_value_int(l - r);
				return true;
			case CEL_BINARY_MUL:
				*result = cel_value_int(l * r);
				return true;
			case CEL_BINARY_DIV:
				if (r == 0) {
					set_error(ctx, "Division by zero");
					return false;
				}
				*result = cel_value_int(l / r);
				return true;
			case CEL_BINARY_MOD:
				if (r == 0) {
					set_error(ctx, "Modulo by zero");
					return false;
				}
				*result = cel_value_int(l % r);
				return true;
			default:
				break;
			}
		} else if (left.type == CEL_TYPE_DOUBLE ||
			   right.type == CEL_TYPE_DOUBLE) {
			double l = (left.type == CEL_TYPE_DOUBLE) ?
					   left.value.double_value :
					   (double)left.value.int_value;
			double r = (right.type == CEL_TYPE_DOUBLE) ?
					   right.value.double_value :
					   (double)right.value.int_value;

			switch (binary->op) {
			case CEL_BINARY_ADD:
				*result = cel_value_double(l + r);
				return true;
			case CEL_BINARY_SUB:
				*result = cel_value_double(l - r);
				return true;
			case CEL_BINARY_MUL:
				*result = cel_value_double(l * r);
				return true;
			case CEL_BINARY_DIV:
				if (r == 0.0) {
					set_error(ctx, "Division by zero");
					return false;
				}
				*result = cel_value_double(l / r);
				return true;
			case CEL_BINARY_MOD:
				if (r == 0.0) {
					set_error(ctx, "Modulo by zero");
					return false;
				}
				*result = cel_value_double(fmod(l, r));
				return true;
			default:
				break;
			}
		} else if (binary->op == CEL_BINARY_ADD &&
			   left.type == CEL_TYPE_STRING &&
			   right.type == CEL_TYPE_STRING) {
			/* 字符串连接 */
			*result = cel_string_concat(&left, &right);
			return true;
		} else if (binary->op == CEL_BINARY_ADD &&
			   left.type == CEL_TYPE_LIST &&
			   right.type == CEL_TYPE_LIST) {
			/* 列表连接 */
			cel_list_t *left_list = left.value.list_value;
			cel_list_t *right_list = right.value.list_value;

			/* 创建新列表 */
			size_t left_size = cel_list_size(left_list);
			size_t right_size = cel_list_size(right_list);
			cel_list_t *new_list = cel_list_create(left_size + right_size);

			if (!new_list) {
				set_error(ctx, "Failed to create list for concatenation");
				return false;
			}

			/* 复制左列表元素 */
			for (size_t i = 0; i < left_size; i++) {
				cel_value_t *elem = cel_list_get(left_list, i);
				if (!elem) {
					cel_list_release(new_list);
					set_error(ctx, "Failed to get element from left list");
					return false;
				}
				if (!cel_list_append(new_list, elem)) {
					cel_list_release(new_list);
					set_error(ctx, "Failed to append element to new list");
					return false;
				}
			}

			/* 复制右列表元素 */
			for (size_t i = 0; i < right_size; i++) {
				cel_value_t *elem = cel_list_get(right_list, i);
				if (!elem) {
					cel_list_release(new_list);
					set_error(ctx, "Failed to get element from right list");
					return false;
				}
				if (!cel_list_append(new_list, elem)) {
					cel_list_release(new_list);
					set_error(ctx, "Failed to append element to new list");
					return false;
				}
			}

			result->type = CEL_TYPE_LIST;
			result->value.list_value = new_list;
			return true;
		} else {
			set_error(ctx, "Type mismatch in arithmetic operation");
			return false;
		}
	}

	/* 比较运算 */
	if (binary->op >= CEL_BINARY_EQ && binary->op <= CEL_BINARY_GE) {
		/* 相等性比较 */
		if (binary->op == CEL_BINARY_EQ) {
			*result = cel_value_bool(cel_value_equals(&left, &right));
			return true;
		}
		if (binary->op == CEL_BINARY_NE) {
			*result = cel_value_bool(!cel_value_equals(&left, &right));
			return true;
		}

		/* 顺序比较 - 只支持数值类型 */
		if (left.type == CEL_TYPE_INT && right.type == CEL_TYPE_INT) {
			int64_t l = left.value.int_value;
			int64_t r = right.value.int_value;
			switch (binary->op) {
			case CEL_BINARY_LT:
				*result = cel_value_bool(l < r);
				return true;
			case CEL_BINARY_LE:
				*result = cel_value_bool(l <= r);
				return true;
			case CEL_BINARY_GT:
				*result = cel_value_bool(l > r);
				return true;
			case CEL_BINARY_GE:
				*result = cel_value_bool(l >= r);
				return true;
			default:
				break;
			}
		} else if (left.type == CEL_TYPE_DOUBLE || right.type == CEL_TYPE_DOUBLE) {
			double l = (left.type == CEL_TYPE_DOUBLE) ?
					   left.value.double_value :
					   (double)left.value.int_value;
			double r = (right.type == CEL_TYPE_DOUBLE) ?
					   right.value.double_value :
					   (double)right.value.int_value;
			switch (binary->op) {
			case CEL_BINARY_LT:
				*result = cel_value_bool(l < r);
				return true;
			case CEL_BINARY_LE:
				*result = cel_value_bool(l <= r);
				return true;
			case CEL_BINARY_GT:
				*result = cel_value_bool(l > r);
				return true;
			case CEL_BINARY_GE:
				*result = cel_value_bool(l >= r);
				return true;
			default:
				break;
			}
		} else {
			set_error(ctx, "Comparison requires numeric operands");
			return false;
		}
	}

	/* in 运算符 */
	if (binary->op == CEL_BINARY_IN) {
		if (right.type == CEL_TYPE_LIST) {
			cel_list_t *list = right.value.list_value;
			for (size_t i = 0; i < list->length; i++) {
				cel_value_t *item = cel_list_get(list, i);
				if (item && cel_value_equals(&left, item)) {
					*result = cel_value_bool(true);
					return true;
				}
			}
			*result = cel_value_bool(false);
			return true;
		} else if (right.type == CEL_TYPE_MAP) {
			cel_map_t *map = right.value.map_value;
			cel_value_t *found = cel_map_get(map, &left);
			*result = cel_value_bool(found != NULL);
			return true;
		} else {
			set_error(ctx, "'in' operator requires list or map");
			return false;
		}
	}

	set_error(ctx, "Unknown binary operator");
	return false;
}

/* ========== 三元运算求值 ========== */

static bool eval_ternary(const cel_ast_ternary_t *ternary, cel_context_t *ctx,
			  cel_value_t *result)
{
	cel_value_t condition;
	if (!eval_node(ternary->condition, ctx, &condition)) {
		return false;
	}

	if (condition.type != CEL_TYPE_BOOL) {
		set_error(ctx, "Ternary condition must be boolean");
		return false;
	}

	if (condition.value.bool_value) {
		return eval_node(ternary->if_true, ctx, result);
	} else {
		return eval_node(ternary->if_false, ctx, result);
	}
}

/* ========== 字段访问求值 ========== */

static bool eval_select(const cel_ast_select_t *select, cel_context_t *ctx,
			 cel_value_t *result)
{
	cel_value_t operand;
	if (!eval_node(select->operand, ctx, &operand)) {
		return false;
	}

	if (operand.type != CEL_TYPE_MAP) {
		if (select->optional) {
			*result = cel_value_null();
			return true;
		}
		set_error(ctx, "Field access requires map");
		return false;
	}

	/* 将字段名转换为字符串值 */
	cel_value_t field_key = cel_value_string_n(select->field,
						    select->field_length);

	cel_value_t *value = cel_map_get(operand.value.map_value, &field_key);

	if (!value) {
		if (select->optional) {
			*result = cel_value_null();
			return true;
		}
		char error_msg[256];
		snprintf(error_msg, sizeof(error_msg), "Field not found: %.*s",
			 (int)select->field_length, select->field);
		set_error(ctx, error_msg);
		return false;
	}

	*result = *value;
	return true;
}

/* ========== 索引访问求值 ========== */

static bool eval_index(const cel_ast_index_t *index, cel_context_t *ctx,
			cel_value_t *result)
{
	cel_value_t operand, index_val;

	if (!eval_node(index->operand, ctx, &operand)) {
		return false;
	}
	if (!eval_node(index->index, ctx, &index_val)) {
		return false;
	}

	if (operand.type == CEL_TYPE_LIST) {
		if (index_val.type != CEL_TYPE_INT) {
			set_error(ctx, "List index must be integer");
			return false;
		}

		cel_list_t *list = operand.value.list_value;
		int64_t idx = index_val.value.int_value;

		if (idx < 0 || (size_t)idx >= list->length) {
			if (index->optional) {
				*result = cel_value_null();
				return true;
			}
			set_error(ctx, "List index out of bounds");
			return false;
		}

		cel_value_t *item = cel_list_get(list, (size_t)idx);
		if (!item) {
			set_error(ctx, "Failed to get list item");
			return false;
		}
		*result = *item;
		return true;

	} else if (operand.type == CEL_TYPE_MAP) {
		cel_value_t *value = cel_map_get(operand.value.map_value,
						  &index_val);
		if (!value) {
			if (index->optional) {
				*result = cel_value_null();
				return true;
			}
			set_error(ctx, "Map key not found");
			return false;
		}

		*result = *value;
		return true;

	} else {
		set_error(ctx, "Index access requires list or map");
		return false;
	}
}

/* ========== 内置函数实现 ========== */

/**
 * @brief 检查函数名是否匹配
 */
static bool func_name_equals(const char *name, size_t length, const char *expected)
{
	size_t expected_len = strlen(expected);
	return length == expected_len && memcmp(name, expected, length) == 0;
}

/**
 * @brief 内置 size() 函数
 * 支持: size(container) 或 container.size()
 */
static bool builtin_size(const cel_ast_call_t *call, cel_context_t *ctx,
			  cel_value_t *result)
{
	cel_value_t arg;

	/* 方法调用: container.size() */
	if (call->target && call->arg_count == 0) {
		if (!eval_node(call->target, ctx, &arg)) {
			return false;
		}
	}
	/* 函数调用: size(container) */
	else if (!call->target && call->arg_count == 1) {
		if (!eval_node(call->args[0], ctx, &arg)) {
			return false;
		}
	} else {
		set_error(ctx, "size() requires exactly 1 argument");
		return false;
	}

	if (arg.type == CEL_TYPE_STRING) {
		*result = cel_value_int((int64_t)cel_string_length(&arg));
		return true;
	} else if (arg.type == CEL_TYPE_LIST) {
		*result = cel_value_int((int64_t)arg.value.list_value->length);
		return true;
	} else if (arg.type == CEL_TYPE_MAP) {
		*result = cel_value_int((int64_t)arg.value.map_value->size);
		return true;
	} else if (arg.type == CEL_TYPE_BYTES) {
		*result = cel_value_int((int64_t)arg.value.bytes_value->length);
		return true;
	} else {
		set_error(ctx, "size() requires string, bytes, list, or map");
		return false;
	}
}

/**
 * @brief 内置 contains() 函数
 * 支持: contains(container, elem) 或 container.contains(elem)
 */
static bool builtin_contains(const cel_ast_call_t *call, cel_context_t *ctx,
			      cel_value_t *result)
{
	cel_value_t container, elem;

	/* 方法调用: container.contains(elem) */
	if (call->target && call->arg_count == 1) {
		if (!eval_node(call->target, ctx, &container)) {
			return false;
		}
		if (!eval_node(call->args[0], ctx, &elem)) {
			return false;
		}
	}
	/* 函数调用: contains(container, elem) */
	else if (!call->target && call->arg_count == 2) {
		if (!eval_node(call->args[0], ctx, &container)) {
			return false;
		}
		if (!eval_node(call->args[1], ctx, &elem)) {
			return false;
		}
	} else {
		set_error(ctx, "contains() requires 2 arguments");
		return false;
	}

	if (container.type == CEL_TYPE_LIST) {
		cel_list_t *list = container.value.list_value;
		for (size_t i = 0; i < list->length; i++) {
			cel_value_t *item = cel_list_get(list, i);
			if (item && cel_value_equals(&elem, item)) {
				*result = cel_value_bool(true);
				return true;
			}
		}
		*result = cel_value_bool(false);
		return true;
	} else if (container.type == CEL_TYPE_STRING) {
		if (elem.type != CEL_TYPE_STRING) {
			set_error(ctx, "string.contains() requires string argument");
			return false;
		}
		/* 使用 strstr 检查子串 */
		const char *haystack = container.value.string_value->data;
		const char *needle = elem.value.string_value->data;
		*result = cel_value_bool(strstr(haystack, needle) != NULL);
		return true;
	} else {
		set_error(ctx, "contains() requires list or string");
		return false;
	}
}

/**
 * @brief 内置 startsWith() 函数
 * 支持: startsWith(str, prefix) 或 str.startsWith(prefix)
 */
static bool builtin_startsWith(const cel_ast_call_t *call, cel_context_t *ctx,
				cel_value_t *result)
{
	cel_value_t str, prefix;

	/* 方法调用: str.startsWith(prefix) */
	if (call->target && call->arg_count == 1) {
		if (!eval_node(call->target, ctx, &str)) {
			return false;
		}
		if (!eval_node(call->args[0], ctx, &prefix)) {
			return false;
		}
	}
	/* 函数调用: startsWith(str, prefix) */
	else if (!call->target && call->arg_count == 2) {
		if (!eval_node(call->args[0], ctx, &str)) {
			return false;
		}
		if (!eval_node(call->args[1], ctx, &prefix)) {
			return false;
		}
	} else {
		set_error(ctx, "startsWith() requires 2 arguments");
		return false;
	}

	if (str.type != CEL_TYPE_STRING || prefix.type != CEL_TYPE_STRING) {
		set_error(ctx, "startsWith() requires string arguments");
		return false;
	}

	size_t str_len = cel_string_length(&str);
	size_t prefix_len = cel_string_length(&prefix);

	if (prefix_len > str_len) {
		*result = cel_value_bool(false);
		return true;
	}

	*result = cel_value_bool(
		memcmp(str.value.string_value->data,
		       prefix.value.string_value->data,
		       prefix_len) == 0);
	return true;
}

/**
 * @brief 内置 endsWith() 函数
 * 支持: endsWith(str, suffix) 或 str.endsWith(suffix)
 */
static bool builtin_endsWith(const cel_ast_call_t *call, cel_context_t *ctx,
			      cel_value_t *result)
{
	cel_value_t str, suffix;

	/* 方法调用: str.endsWith(suffix) */
	if (call->target && call->arg_count == 1) {
		if (!eval_node(call->target, ctx, &str)) {
			return false;
		}
		if (!eval_node(call->args[0], ctx, &suffix)) {
			return false;
		}
	}
	/* 函数调用: endsWith(str, suffix) */
	else if (!call->target && call->arg_count == 2) {
		if (!eval_node(call->args[0], ctx, &str)) {
			return false;
		}
		if (!eval_node(call->args[1], ctx, &suffix)) {
			return false;
		}
	} else {
		set_error(ctx, "endsWith() requires 2 arguments");
		return false;
	}

	if (str.type != CEL_TYPE_STRING || suffix.type != CEL_TYPE_STRING) {
		set_error(ctx, "endsWith() requires string arguments");
		return false;
	}

	size_t str_len = cel_string_length(&str);
	size_t suffix_len = cel_string_length(&suffix);

	if (suffix_len > str_len) {
		*result = cel_value_bool(false);
		return true;
	}

	*result = cel_value_bool(
		memcmp(str.value.string_value->data + (str_len - suffix_len),
		       suffix.value.string_value->data,
		       suffix_len) == 0);
	return true;
}

#ifdef CEL_ENABLE_REGEX
/**
 * @brief 内置 matches() 函数 - 正则表达式匹配
 * 支持: matches(str, pattern) 或 str.matches(pattern)
 */
static bool builtin_matches(const cel_ast_call_t *call, cel_context_t *ctx,
			    cel_value_t *result)
{
	cel_value_t str, pattern;

	/* 方法调用: str.matches(pattern) */
	if (call->target && call->arg_count == 1) {
		if (!eval_node(call->target, ctx, &str)) {
			return false;
		}
		if (!eval_node(call->args[0], ctx, &pattern)) {
			return false;
		}
	}
	/* 函数调用: matches(str, pattern) */
	else if (!call->target && call->arg_count == 2) {
		if (!eval_node(call->args[0], ctx, &str)) {
			return false;
		}
		if (!eval_node(call->args[1], ctx, &pattern)) {
			return false;
		}
	} else {
		set_error(ctx, "matches() requires 2 arguments");
		return false;
	}

	if (str.type != CEL_TYPE_STRING || pattern.type != CEL_TYPE_STRING) {
		set_error(ctx, "matches() requires string arguments");
		return false;
	}

	const char *subject = str.value.string_value->data;
	size_t subject_len = cel_string_length(&str);
	const char *regex = pattern.value.string_value->data;

	/* 编译正则表达式 */
	int errornumber;
	PCRE2_SIZE erroroffset;
	pcre2_code *re = pcre2_compile(
		(PCRE2_SPTR)regex,
		PCRE2_ZERO_TERMINATED,
		0,
		&errornumber,
		&erroroffset,
		NULL);

	if (re == NULL) {
		PCRE2_UCHAR buffer[256];
		pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
		char error_msg[512];
		snprintf(error_msg, sizeof(error_msg),
			 "regex compile error at offset %zu: %s",
			 (size_t)erroroffset, buffer);
		set_error(ctx, error_msg);
		return false;
	}

	/* 创建匹配数据 */
	pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);
	if (match_data == NULL) {
		pcre2_code_free(re);
		set_error(ctx, "failed to create match data");
		return false;
	}

	/* 执行匹配 */
	int rc = pcre2_match(
		re,
		(PCRE2_SPTR)subject,
		subject_len,
		0,
		0,
		match_data,
		NULL);

	/* 清理 */
	pcre2_match_data_free(match_data);
	pcre2_code_free(re);

	/* 返回结果 */
	*result = cel_value_bool(rc >= 0);
	return true;
}
#endif /* CEL_ENABLE_REGEX */

/**
 * @brief 内置 int() 类型转换函数
 */
static bool builtin_int(const cel_ast_call_t *call, cel_context_t *ctx,
			 cel_value_t *result)
{
	if (call->arg_count != 1) {
		set_error(ctx, "int() requires exactly 1 argument");
		return false;
	}

	cel_value_t arg;
	if (!eval_node(call->args[0], ctx, &arg)) {
		return false;
	}

	switch (arg.type) {
	case CEL_TYPE_INT:
		*result = arg;
		return true;
	case CEL_TYPE_UINT:
		/* 检查溢出 */
		if (arg.value.uint_value > (uint64_t)INT64_MAX) {
			set_error(ctx, "uint to int overflow");
			return false;
		}
		*result = cel_value_int((int64_t)arg.value.uint_value);
		return true;
	case CEL_TYPE_DOUBLE:
		*result = cel_value_int((int64_t)arg.value.double_value);
		return true;
	case CEL_TYPE_STRING: {
		/* 尝试解析字符串为整数 */
		const char *str = arg.value.string_value->data;
		char *end;
		long long val = strtoll(str, &end, 10);
		if (*end != '\0') {
			set_error(ctx, "invalid integer string");
			return false;
		}
		*result = cel_value_int((int64_t)val);
		return true;
	}
	default:
		set_error(ctx, "int() cannot convert this type");
		return false;
	}
}

/**
 * @brief 内置 uint() 类型转换函数
 */
static bool builtin_uint(const cel_ast_call_t *call, cel_context_t *ctx,
			  cel_value_t *result)
{
	if (call->arg_count != 1) {
		set_error(ctx, "uint() requires exactly 1 argument");
		return false;
	}

	cel_value_t arg;
	if (!eval_node(call->args[0], ctx, &arg)) {
		return false;
	}

	switch (arg.type) {
	case CEL_TYPE_UINT:
		*result = arg;
		return true;
	case CEL_TYPE_INT:
		if (arg.value.int_value < 0) {
			set_error(ctx, "int to uint: negative value");
			return false;
		}
		*result = cel_value_uint((uint64_t)arg.value.int_value);
		return true;
	case CEL_TYPE_DOUBLE:
		if (arg.value.double_value < 0) {
			set_error(ctx, "double to uint: negative value");
			return false;
		}
		*result = cel_value_uint((uint64_t)arg.value.double_value);
		return true;
	case CEL_TYPE_STRING: {
		const char *str = arg.value.string_value->data;
		char *end;
		unsigned long long val = strtoull(str, &end, 10);
		if (*end != '\0') {
			set_error(ctx, "invalid unsigned integer string");
			return false;
		}
		*result = cel_value_uint((uint64_t)val);
		return true;
	}
	default:
		set_error(ctx, "uint() cannot convert this type");
		return false;
	}
}

/**
 * @brief 内置 double() 类型转换函数
 */
static bool builtin_double(const cel_ast_call_t *call, cel_context_t *ctx,
			    cel_value_t *result)
{
	if (call->arg_count != 1) {
		set_error(ctx, "double() requires exactly 1 argument");
		return false;
	}

	cel_value_t arg;
	if (!eval_node(call->args[0], ctx, &arg)) {
		return false;
	}

	switch (arg.type) {
	case CEL_TYPE_DOUBLE:
		*result = arg;
		return true;
	case CEL_TYPE_INT:
		*result = cel_value_double((double)arg.value.int_value);
		return true;
	case CEL_TYPE_UINT:
		*result = cel_value_double((double)arg.value.uint_value);
		return true;
	case CEL_TYPE_STRING: {
		const char *str = arg.value.string_value->data;
		char *end;
		double val = strtod(str, &end);
		if (*end != '\0') {
			set_error(ctx, "invalid double string");
			return false;
		}
		*result = cel_value_double(val);
		return true;
	}
	default:
		set_error(ctx, "double() cannot convert this type");
		return false;
	}
}

/**
 * @brief 内置 string() 类型转换函数
 */
static bool builtin_string(const cel_ast_call_t *call, cel_context_t *ctx,
			    cel_value_t *result)
{
	if (call->arg_count != 1) {
		set_error(ctx, "string() requires exactly 1 argument");
		return false;
	}

	cel_value_t arg;
	if (!eval_node(call->args[0], ctx, &arg)) {
		return false;
	}

	char buf[64];
	switch (arg.type) {
	case CEL_TYPE_STRING:
		*result = arg;
		return true;
	case CEL_TYPE_INT:
		snprintf(buf, sizeof(buf), "%ld", (long)arg.value.int_value);
		*result = cel_value_string(buf);
		return true;
	case CEL_TYPE_UINT:
		snprintf(buf, sizeof(buf), "%lu", (unsigned long)arg.value.uint_value);
		*result = cel_value_string(buf);
		return true;
	case CEL_TYPE_DOUBLE:
		snprintf(buf, sizeof(buf), "%g", arg.value.double_value);
		*result = cel_value_string(buf);
		return true;
	case CEL_TYPE_BOOL:
		*result = cel_value_string(arg.value.bool_value ? "true" : "false");
		return true;
	default:
		set_error(ctx, "string() cannot convert this type");
		return false;
	}
}

/**
 * @brief 内置 type() 函数
 */
static bool builtin_type(const cel_ast_call_t *call, cel_context_t *ctx,
			  cel_value_t *result)
{
	if (call->arg_count != 1) {
		set_error(ctx, "type() requires exactly 1 argument");
		return false;
	}

	cel_value_t arg;
	if (!eval_node(call->args[0], ctx, &arg)) {
		return false;
	}

	const char *type_name;
	switch (arg.type) {
	case CEL_TYPE_NULL:
		type_name = "null_type";
		break;
	case CEL_TYPE_BOOL:
		type_name = "bool";
		break;
	case CEL_TYPE_INT:
		type_name = "int";
		break;
	case CEL_TYPE_UINT:
		type_name = "uint";
		break;
	case CEL_TYPE_DOUBLE:
		type_name = "double";
		break;
	case CEL_TYPE_STRING:
		type_name = "string";
		break;
	case CEL_TYPE_BYTES:
		type_name = "bytes";
		break;
	case CEL_TYPE_LIST:
		type_name = "list";
		break;
	case CEL_TYPE_MAP:
		type_name = "map";
		break;
	case CEL_TYPE_TIMESTAMP:
		type_name = "google.protobuf.Timestamp";
		break;
	case CEL_TYPE_DURATION:
		type_name = "google.protobuf.Duration";
		break;
	default:
		type_name = "unknown";
		break;
	}

	*result = cel_value_string(type_name);
	return true;
}

/* ========== 时间戳方法 ========== */

/**
 * @brief 从 Unix 时间戳获取 tm 结构
 */
static bool timestamp_to_tm(int64_t seconds, int16_t offset_minutes, struct tm *tm_out)
{
	time_t t = (time_t)seconds;
	/* 应用时区偏移 */
	t += offset_minutes * 60;

	struct tm *result;
#ifdef _WIN32
	result = gmtime(&t);
	if (result) {
		*tm_out = *result;
	}
#else
	result = gmtime_r(&t, tm_out);
#endif
	return result != NULL;
}

/**
 * @brief timestamp.getFullYear() - 获取年份
 */
static bool builtin_getFullYear(const cel_ast_call_t *call, cel_context_t *ctx,
				 cel_value_t *result)
{
	/* 只支持方法调用: ts.getFullYear() */
	if (!call->target || call->arg_count != 0) {
		set_error(ctx, "getFullYear() requires no arguments");
		return false;
	}

	cel_value_t ts;
	if (!eval_node(call->target, ctx, &ts)) {
		return false;
	}

	if (ts.type != CEL_TYPE_TIMESTAMP) {
		set_error(ctx, "getFullYear() requires timestamp");
		return false;
	}

	struct tm tm;
	if (!timestamp_to_tm(ts.value.timestamp_value.seconds,
			     ts.value.timestamp_value.offset_minutes, &tm)) {
		set_error(ctx, "Failed to convert timestamp");
		return false;
	}

	*result = cel_value_int(tm.tm_year + 1900);
	return true;
}

/**
 * @brief timestamp.getMonth() - 获取月份 (0-11)
 */
static bool builtin_getMonth(const cel_ast_call_t *call, cel_context_t *ctx,
			      cel_value_t *result)
{
	if (!call->target || call->arg_count != 0) {
		set_error(ctx, "getMonth() requires no arguments");
		return false;
	}

	cel_value_t ts;
	if (!eval_node(call->target, ctx, &ts)) {
		return false;
	}

	if (ts.type != CEL_TYPE_TIMESTAMP) {
		set_error(ctx, "getMonth() requires timestamp");
		return false;
	}

	struct tm tm;
	if (!timestamp_to_tm(ts.value.timestamp_value.seconds,
			     ts.value.timestamp_value.offset_minutes, &tm)) {
		set_error(ctx, "Failed to convert timestamp");
		return false;
	}

	*result = cel_value_int(tm.tm_mon);
	return true;
}

/**
 * @brief timestamp.getDayOfMonth() - 获取日期 (1-31)
 */
static bool builtin_getDayOfMonth(const cel_ast_call_t *call, cel_context_t *ctx,
				   cel_value_t *result)
{
	if (!call->target || call->arg_count != 0) {
		set_error(ctx, "getDayOfMonth() requires no arguments");
		return false;
	}

	cel_value_t ts;
	if (!eval_node(call->target, ctx, &ts)) {
		return false;
	}

	if (ts.type != CEL_TYPE_TIMESTAMP) {
		set_error(ctx, "getDayOfMonth() requires timestamp");
		return false;
	}

	struct tm tm;
	if (!timestamp_to_tm(ts.value.timestamp_value.seconds,
			     ts.value.timestamp_value.offset_minutes, &tm)) {
		set_error(ctx, "Failed to convert timestamp");
		return false;
	}

	*result = cel_value_int(tm.tm_mday);
	return true;
}

/**
 * @brief timestamp.getDayOfWeek() - 获取星期几 (0=Sunday, 6=Saturday)
 */
static bool builtin_getDayOfWeek(const cel_ast_call_t *call, cel_context_t *ctx,
				  cel_value_t *result)
{
	if (!call->target || call->arg_count != 0) {
		set_error(ctx, "getDayOfWeek() requires no arguments");
		return false;
	}

	cel_value_t ts;
	if (!eval_node(call->target, ctx, &ts)) {
		return false;
	}

	if (ts.type != CEL_TYPE_TIMESTAMP) {
		set_error(ctx, "getDayOfWeek() requires timestamp");
		return false;
	}

	struct tm tm;
	if (!timestamp_to_tm(ts.value.timestamp_value.seconds,
			     ts.value.timestamp_value.offset_minutes, &tm)) {
		set_error(ctx, "Failed to convert timestamp");
		return false;
	}

	*result = cel_value_int(tm.tm_wday);
	return true;
}

/**
 * @brief timestamp.getDayOfYear() - 获取年中第几天 (0-365)
 */
static bool builtin_getDayOfYear(const cel_ast_call_t *call, cel_context_t *ctx,
				  cel_value_t *result)
{
	if (!call->target || call->arg_count != 0) {
		set_error(ctx, "getDayOfYear() requires no arguments");
		return false;
	}

	cel_value_t ts;
	if (!eval_node(call->target, ctx, &ts)) {
		return false;
	}

	if (ts.type != CEL_TYPE_TIMESTAMP) {
		set_error(ctx, "getDayOfYear() requires timestamp");
		return false;
	}

	struct tm tm;
	if (!timestamp_to_tm(ts.value.timestamp_value.seconds,
			     ts.value.timestamp_value.offset_minutes, &tm)) {
		set_error(ctx, "Failed to convert timestamp");
		return false;
	}

	*result = cel_value_int(tm.tm_yday);
	return true;
}

/**
 * @brief timestamp.getHours() 或 duration.getHours()
 */
static bool builtin_getHours(const cel_ast_call_t *call, cel_context_t *ctx,
			      cel_value_t *result)
{
	if (!call->target || call->arg_count != 0) {
		set_error(ctx, "getHours() requires no arguments");
		return false;
	}

	cel_value_t val;
	if (!eval_node(call->target, ctx, &val)) {
		return false;
	}

	if (val.type == CEL_TYPE_TIMESTAMP) {
		struct tm tm;
		if (!timestamp_to_tm(val.value.timestamp_value.seconds,
				     val.value.timestamp_value.offset_minutes, &tm)) {
			set_error(ctx, "Failed to convert timestamp");
			return false;
		}
		*result = cel_value_int(tm.tm_hour);
		return true;
	} else if (val.type == CEL_TYPE_DURATION) {
		/* duration.getHours() 返回总小时数 */
		int64_t total_hours = val.value.duration_value.seconds / 3600;
		*result = cel_value_int(total_hours);
		return true;
	} else {
		set_error(ctx, "getHours() requires timestamp or duration");
		return false;
	}
}

/**
 * @brief timestamp.getMinutes() 或 duration.getMinutes()
 */
static bool builtin_getMinutes(const cel_ast_call_t *call, cel_context_t *ctx,
				cel_value_t *result)
{
	if (!call->target || call->arg_count != 0) {
		set_error(ctx, "getMinutes() requires no arguments");
		return false;
	}

	cel_value_t val;
	if (!eval_node(call->target, ctx, &val)) {
		return false;
	}

	if (val.type == CEL_TYPE_TIMESTAMP) {
		struct tm tm;
		if (!timestamp_to_tm(val.value.timestamp_value.seconds,
				     val.value.timestamp_value.offset_minutes, &tm)) {
			set_error(ctx, "Failed to convert timestamp");
			return false;
		}
		*result = cel_value_int(tm.tm_min);
		return true;
	} else if (val.type == CEL_TYPE_DURATION) {
		/* duration.getMinutes() 返回总分钟数 */
		int64_t total_minutes = val.value.duration_value.seconds / 60;
		*result = cel_value_int(total_minutes);
		return true;
	} else {
		set_error(ctx, "getMinutes() requires timestamp or duration");
		return false;
	}
}

/**
 * @brief timestamp.getSeconds() 或 duration.getSeconds()
 */
static bool builtin_getSeconds(const cel_ast_call_t *call, cel_context_t *ctx,
				cel_value_t *result)
{
	if (!call->target || call->arg_count != 0) {
		set_error(ctx, "getSeconds() requires no arguments");
		return false;
	}

	cel_value_t val;
	if (!eval_node(call->target, ctx, &val)) {
		return false;
	}

	if (val.type == CEL_TYPE_TIMESTAMP) {
		struct tm tm;
		if (!timestamp_to_tm(val.value.timestamp_value.seconds,
				     val.value.timestamp_value.offset_minutes, &tm)) {
			set_error(ctx, "Failed to convert timestamp");
			return false;
		}
		*result = cel_value_int(tm.tm_sec);
		return true;
	} else if (val.type == CEL_TYPE_DURATION) {
		/* duration.getSeconds() 返回总秒数 */
		*result = cel_value_int(val.value.duration_value.seconds);
		return true;
	} else {
		set_error(ctx, "getSeconds() requires timestamp or duration");
		return false;
	}
}

/**
 * @brief timestamp.getMilliseconds() 或 duration.getMilliseconds()
 */
static bool builtin_getMilliseconds(const cel_ast_call_t *call, cel_context_t *ctx,
				     cel_value_t *result)
{
	if (!call->target || call->arg_count != 0) {
		set_error(ctx, "getMilliseconds() requires no arguments");
		return false;
	}

	cel_value_t val;
	if (!eval_node(call->target, ctx, &val)) {
		return false;
	}

	if (val.type == CEL_TYPE_TIMESTAMP) {
		/* timestamp 的毫秒部分 */
		int64_t ms = val.value.timestamp_value.nanoseconds / 1000000;
		*result = cel_value_int(ms);
		return true;
	} else if (val.type == CEL_TYPE_DURATION) {
		/* duration 的总毫秒数 */
		int64_t total_ms = val.value.duration_value.seconds * 1000 +
				   val.value.duration_value.nanoseconds / 1000000;
		*result = cel_value_int(total_ms);
		return true;
	} else {
		set_error(ctx, "getMilliseconds() requires timestamp or duration");
		return false;
	}
}

/**
 * @brief timestamp() 函数 - 从 RFC3339 字符串解析时间戳
 */
static bool builtin_timestamp(const cel_ast_call_t *call, cel_context_t *ctx,
			       cel_value_t *result)
{
	/* timestamp(string) 或 timestamp(int) */
	if (call->target || call->arg_count != 1) {
		set_error(ctx, "timestamp() requires exactly 1 argument");
		return false;
	}

	cel_value_t arg;
	if (!eval_node(call->args[0], ctx, &arg)) {
		return false;
	}

	if (arg.type == CEL_TYPE_INT) {
		/* 直接从 Unix 时间戳创建 */
		*result = cel_value_timestamp(arg.value.int_value, 0, 0);
		return true;
	} else if (arg.type == CEL_TYPE_STRING) {
		/* 解析 RFC3339 格式: "2021-08-15T10:30:00Z" */
		const char *s = arg.value.string_value->data;
		int year, month, day, hour, min, sec;
		char tz;

		int parsed = sscanf(s, "%d-%d-%dT%d:%d:%d%c",
				    &year, &month, &day, &hour, &min, &sec, &tz);

		if (parsed < 6) {
			set_error(ctx, "Invalid RFC3339 timestamp format");
			return false;
		}

		/* 转换为 Unix 时间戳 */
		struct tm tm = {0};
		tm.tm_year = year - 1900;
		tm.tm_mon = month - 1;
		tm.tm_mday = day;
		tm.tm_hour = hour;
		tm.tm_min = min;
		tm.tm_sec = sec;

		time_t t;
#ifdef _WIN32
		t = _mkgmtime(&tm);
#else
		t = timegm(&tm);
#endif
		if (t == -1) {
			set_error(ctx, "Failed to convert timestamp");
			return false;
		}

		*result = cel_value_timestamp((int64_t)t, 0, 0);
		return true;
	} else {
		set_error(ctx, "timestamp() requires int or string argument");
		return false;
	}
}

/**
 * @brief duration() 函数 - 从字符串解析时长
 */
static bool builtin_duration(const cel_ast_call_t *call, cel_context_t *ctx,
			      cel_value_t *result)
{
	/* duration(string) 格式: "1h30m45s" 或 "3600s" */
	if (call->target || call->arg_count != 1) {
		set_error(ctx, "duration() requires exactly 1 argument");
		return false;
	}

	cel_value_t arg;
	if (!eval_node(call->args[0], ctx, &arg)) {
		return false;
	}

	if (arg.type != CEL_TYPE_STRING) {
		set_error(ctx, "duration() requires string argument");
		return false;
	}

	const char *s = arg.value.string_value->data;
	int64_t total_seconds = 0;
	int64_t current_num = 0;
	bool negative = false;

	if (*s == '-') {
		negative = true;
		s++;
	}

	while (*s) {
		if (*s >= '0' && *s <= '9') {
			current_num = current_num * 10 + (*s - '0');
		} else if (*s == 'h' || *s == 'H') {
			total_seconds += current_num * 3600;
			current_num = 0;
		} else if (*s == 'm' || *s == 'M') {
			total_seconds += current_num * 60;
			current_num = 0;
		} else if (*s == 's' || *s == 'S') {
			total_seconds += current_num;
			current_num = 0;
		} else {
			set_error(ctx, "Invalid duration format");
			return false;
		}
		s++;
	}

	/* 如果字符串末尾没有单位，假设为秒 */
	total_seconds += current_num;

	if (negative) {
		total_seconds = -total_seconds;
	}

	*result = cel_value_duration(total_seconds, 0);
	return true;
}

/* ========== 函数调用求值 ========== */

static bool eval_call(const cel_ast_call_t *call, cel_context_t *ctx,
		      cel_value_t *result)
{
	/* 分发到内置函数 */
	if (func_name_equals(call->function, call->function_length, "size")) {
		return builtin_size(call, ctx, result);
	}
	if (func_name_equals(call->function, call->function_length, "contains")) {
		return builtin_contains(call, ctx, result);
	}
	if (func_name_equals(call->function, call->function_length, "startsWith")) {
		return builtin_startsWith(call, ctx, result);
	}
	if (func_name_equals(call->function, call->function_length, "endsWith")) {
		return builtin_endsWith(call, ctx, result);
	}
#ifdef CEL_ENABLE_REGEX
	if (func_name_equals(call->function, call->function_length, "matches")) {
		return builtin_matches(call, ctx, result);
	}
#endif
	if (func_name_equals(call->function, call->function_length, "int")) {
		return builtin_int(call, ctx, result);
	}
	if (func_name_equals(call->function, call->function_length, "uint")) {
		return builtin_uint(call, ctx, result);
	}
	if (func_name_equals(call->function, call->function_length, "double")) {
		return builtin_double(call, ctx, result);
	}
	if (func_name_equals(call->function, call->function_length, "string")) {
		return builtin_string(call, ctx, result);
	}
	if (func_name_equals(call->function, call->function_length, "type")) {
		return builtin_type(call, ctx, result);
	}

	/* 时间函数 */
	if (func_name_equals(call->function, call->function_length, "timestamp")) {
		return builtin_timestamp(call, ctx, result);
	}
	if (func_name_equals(call->function, call->function_length, "duration")) {
		return builtin_duration(call, ctx, result);
	}

	/* 时间戳方法 */
	if (func_name_equals(call->function, call->function_length, "getFullYear")) {
		return builtin_getFullYear(call, ctx, result);
	}
	if (func_name_equals(call->function, call->function_length, "getMonth")) {
		return builtin_getMonth(call, ctx, result);
	}
	if (func_name_equals(call->function, call->function_length, "getDayOfMonth")) {
		return builtin_getDayOfMonth(call, ctx, result);
	}
	if (func_name_equals(call->function, call->function_length, "getDayOfWeek")) {
		return builtin_getDayOfWeek(call, ctx, result);
	}
	if (func_name_equals(call->function, call->function_length, "getDayOfYear")) {
		return builtin_getDayOfYear(call, ctx, result);
	}
	if (func_name_equals(call->function, call->function_length, "getHours")) {
		return builtin_getHours(call, ctx, result);
	}
	if (func_name_equals(call->function, call->function_length, "getMinutes")) {
		return builtin_getMinutes(call, ctx, result);
	}
	if (func_name_equals(call->function, call->function_length, "getSeconds")) {
		return builtin_getSeconds(call, ctx, result);
	}
	if (func_name_equals(call->function, call->function_length, "getMilliseconds")) {
		return builtin_getMilliseconds(call, ctx, result);
	}

	/* 查找上下文中注册的函数 */
	char *func_name = strndup(call->function, call->function_length);
	if (!func_name) {
		set_error(ctx, "Out of memory");
		return false;
	}

	cel_function_t *func = cel_context_get_function(ctx, func_name);
	if (func) {
		/* 求值所有参数 */
		cel_value_t *args = NULL;
		cel_value_t **arg_ptrs = NULL;
		if (call->arg_count > 0) {
			args = malloc(sizeof(cel_value_t) * call->arg_count);
			arg_ptrs = malloc(sizeof(cel_value_t *) * call->arg_count);
			if (!args || !arg_ptrs) {
				free(func_name);
				free(args);
				free(arg_ptrs);
				set_error(ctx, "Out of memory");
				return false;
			}

			for (size_t i = 0; i < call->arg_count; i++) {
				if (!eval_node(call->args[i], ctx, &args[i])) {
					free(func_name);
					free(args);
					free(arg_ptrs);
					return false;
				}
				arg_ptrs[i] = &args[i];
			}
		}

		/* 调用函数 */
		cel_func_context_t func_ctx = {
			.context = ctx,
			.func_name = func_name,
			.call_site = NULL
		};

		cel_result_t func_result = func->func(&func_ctx, arg_ptrs, call->arg_count);

		free(func_name);
		free(args);
		free(arg_ptrs);

		if (!func_result.is_ok) {
			if (func_result.error) {
				set_error(ctx, func_result.error->message);
				cel_error_destroy(func_result.error);
			} else {
				set_error(ctx, "Function call failed");
			}
			return false;
		}

		/* 从 void* 复制返回值 */
		if (func_result.value) {
			*result = *(cel_value_t *)func_result.value;
		} else {
			*result = cel_value_null();
		}
		return true;
	}

	char error_msg[256];
	snprintf(error_msg, sizeof(error_msg), "Unknown function: %s", func_name);
	free(func_name);
	set_error(ctx, error_msg);
	return false;
}

/* ========== 列表字面量求值 ========== */

static bool eval_list(const cel_ast_list_t *list, cel_context_t *ctx,
		      cel_value_t *result)
{
	cel_list_t *cel_list = cel_list_create(list->element_count);
	if (!cel_list) {
		set_error(ctx, "Failed to create list");
		return false;
	}

	for (size_t i = 0; i < list->element_count; i++) {
		cel_value_t element;
		if (!eval_node(list->elements[i], ctx, &element)) {
			cel_list_release(cel_list);
			return false;
		}

		if (!cel_list_append(cel_list, &element)) {
			cel_list_release(cel_list);
			set_error(ctx, "Failed to append to list");
			return false;
		}
	}

	result->type = CEL_TYPE_LIST;
	result->value.list_value = cel_list;
	return true;
}

/* ========== Map 字面量求值 ========== */

static bool eval_map(const cel_ast_map_t *map, cel_context_t *ctx,
		     cel_value_t *result)
{
	cel_map_t *cel_map = cel_map_create(map->entry_count > 0 ? map->entry_count : 16);
	if (!cel_map) {
		set_error(ctx, "Failed to create map");
		return false;
	}

	for (size_t i = 0; i < map->entry_count; i++) {
		cel_value_t key, value;

		if (!eval_node(map->entries[i].key, ctx, &key)) {
			cel_map_release(cel_map);
			return false;
		}

		if (!eval_node(map->entries[i].value, ctx, &value)) {
			cel_map_release(cel_map);
			return false;
		}

		if (!cel_map_put(cel_map, &key, &value)) {
			cel_map_release(cel_map);
			set_error(ctx, "Failed to set map entry");
			return false;
		}
	}

	result->type = CEL_TYPE_MAP;
	result->value.map_value = cel_map;
	return true;
}

/* ========== Comprehension 求值 ========== */

/**
 * @brief 对 Comprehension 表达式求值
 *
 * Comprehension 执行模型:
 * 1. 求值 iter_range，得到要迭代的集合（List 或 Map）
 * 2. 求值 accu_init，初始化累加器变量
 * 3. 对集合中的每个元素:
 *    a. 将元素绑定到 iter_var
 *    b. 求值 loop_cond，如果为 false 则中断循环
 *    c. 求值 loop_step，更新累加器
 * 4. 求值 result 表达式，返回最终结果
 *
 * @param comp Comprehension 节点
 * @param ctx 求值上下文
 * @param result 输出结果
 * @return true 成功，false 失败
 */

/**
 * @brief Comprehension 求值 (使用新的 cel_context API)
 *
 * Comprehension 语义:
 * 1. 求值 iter_range (列表或Map)
 * 2. 求值 accu_init (累加器初始值)
 * 3. 对集合中的每个元素:
 *    a. 将元素绑定到 iter_var
 *    b. 求值 loop_cond，如果为 false 则中断循环
 *    c. 求值 loop_step，更新累加器
 * 4. 求值 result 表达式，返回最终结果
 */
static bool eval_comprehension(const cel_ast_comprehension_t *comp,
				 cel_context_t *ctx, cel_value_t *result)
{
	if (!comp || !ctx || !result) {
		set_error(ctx, "Invalid arguments to eval_comprehension");
		return false;
	}

	/* 1. 求值迭代范围 */
	cel_value_t iter_range_val;
	if (!eval_node(comp->iter_range, ctx, &iter_range_val)) {
		return false;
	}

	/* 检查迭代范围类型 */
	if (iter_range_val.type != CEL_TYPE_LIST && iter_range_val.type != CEL_TYPE_MAP) {
		set_error(ctx, "Comprehension iter_range must be a list or map");
		cel_value_destroy(&iter_range_val);
		return false;
	}

	/* 2. 求值累加器初始值 */
	cel_value_t accu_val;
	if (!eval_node(comp->accu_init, ctx, &accu_val)) {
		cel_value_destroy(&iter_range_val);
		return false;
	}

	/* 创建子上下文用于管理累加器 */
	cel_context_t *loop_ctx = cel_context_create_child(ctx);
	if (!loop_ctx) {
		set_error(ctx, "Failed to create loop context");
		cel_value_destroy(&iter_range_val);
		cel_value_destroy(&accu_val);
		return false;
	}

	/* 构造累加器变量名 */
	char *accu_name = strndup(comp->accu_var, comp->accu_var_length);
	if (!accu_name) {
		set_error(ctx, "Out of memory");
		cel_context_destroy(loop_ctx);
		cel_value_destroy(&iter_range_val);
		cel_value_destroy(&accu_val);
		return false;
	}

	/* 添加累加器到子上下文 */
	if (cel_context_add_variable(loop_ctx, accu_name, &accu_val) != CEL_OK) {
		set_error(ctx, "Failed to bind accumulator variable");
		free(accu_name);
		cel_context_destroy(loop_ctx);
		cel_value_destroy(&iter_range_val);
		cel_value_destroy(&accu_val);
		return false;
	}

	bool success = false;

	/* 3. 迭代处理 */
	if (iter_range_val.type == CEL_TYPE_LIST) {
		/* List 迭代 */
		cel_list_t *list = iter_range_val.value.list_value;
		size_t list_size = cel_list_size(list);

		/* 构造循环变量名 */
		char *iter_name = strndup(comp->iter_var, comp->iter_var_length);
		if (!iter_name) {
			set_error(ctx, "Out of memory");
			goto cleanup;
		}

		for (size_t i = 0; i < list_size; i++) {
			/* 获取列表元素 */
			cel_value_t *elem_ptr = cel_list_get(list, i);
			if (!elem_ptr) {
				set_error(ctx, "Failed to get list element");
				free(iter_name);
				goto cleanup;
			}

			/* 为每次迭代创建子上下文 */
			cel_context_t *iter_ctx = cel_context_create_child(loop_ctx);
			if (!iter_ctx) {
				set_error(ctx, "Failed to create iteration context");
				free(iter_name);
				goto cleanup;
			}

			/* 绑定循环变量 */
			if (cel_context_add_variable(iter_ctx, iter_name, elem_ptr) != CEL_OK) {
				set_error(ctx, "Failed to bind iteration variable");
				cel_context_destroy(iter_ctx);
				free(iter_name);
				goto cleanup;
			}

			/* 检查循环条件 */
			cel_value_t cond_val;
			if (!eval_node(comp->loop_cond, iter_ctx, &cond_val)) {
				cel_context_destroy(iter_ctx);
				free(iter_name);
				goto cleanup;
			}

			/* 条件必须是布尔值 */
			if (cond_val.type != CEL_TYPE_BOOL) {
				set_error(ctx, "Loop condition must be boolean");
				cel_context_destroy(iter_ctx);
				free(iter_name);
				goto cleanup;
			}

			/* 如果条件为 false，中断循环 */
			if (!cond_val.value.bool_value) {
				cel_context_destroy(iter_ctx);
				break;
			}

			/* 求值循环步骤，更新累加器 */
			cel_value_t new_accu_val;
			if (!eval_node(comp->loop_step, iter_ctx, &new_accu_val)) {
				cel_context_destroy(iter_ctx);
				free(iter_name);
				goto cleanup;
			}

			/* 销毁迭代上下文 */
			cel_context_destroy(iter_ctx);

			/* 更新累加器：移除旧值，添加新值 */
			cel_context_remove_variable(loop_ctx, accu_name);
			if (cel_context_add_variable(loop_ctx, accu_name, &new_accu_val) != CEL_OK) {
				set_error(ctx, "Failed to update accumulator");
				cel_value_destroy(&new_accu_val);
				free(iter_name);
				goto cleanup;
			}
		}

		free(iter_name);

	} else {
		/* TODO: Map 迭代（目前只支持 List） */
		set_error(ctx, "Map comprehension not yet implemented");
		goto cleanup;
	}

	/* 4. 返回结果 */
	if (comp->result) {
		/* 求值结果表达式 */
		if (!eval_node(comp->result, loop_ctx, result)) {
			goto cleanup;
		}
	} else {
		/* 没有结果表达式，返回累加器的值 */
		cel_value_t *final_accu = cel_context_get_variable(loop_ctx, accu_name);
		if (final_accu) {
			*result = *final_accu;
		} else {
			set_error(ctx, "Failed to get final accumulator value");
			goto cleanup;
		}
	}

	success = true;

cleanup:
	free(accu_name);
	cel_context_destroy(loop_ctx);
	cel_value_destroy(&iter_range_val);

	return success;
}

/* ========== 错误处理 ========== */

static void set_error(cel_context_t *ctx, const char *message)
{
	/* TODO: Task 4.2 - 实现新的错误处理机制 */
	(void)ctx;
	fprintf(stderr, "CEL Error: %s\n", message);
}
