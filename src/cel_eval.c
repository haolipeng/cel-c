/**
 * @file cel_eval.c
 * @brief CEL 求值器实现
 */

#include "cel/cel_eval.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void set_error(cel_context_t *ctx, const char *message);

/* ========== 上下文管理 ========== */

cel_context_t *cel_context_create(void)
{
	cel_context_t *ctx = malloc(sizeof(cel_context_t));
	if (!ctx) {
		return NULL;
	}

	ctx->bindings = NULL;
	ctx->binding_count = 0;
	ctx->binding_capacity = 0;
	ctx->error = NULL;

	return ctx;
}

void cel_context_destroy(cel_context_t *ctx)
{
	if (!ctx) {
		return;
	}

	/* 释放所有绑定的值 */
	for (size_t i = 0; i < ctx->binding_count; i++) {
		cel_value_destroy(&ctx->bindings[i].value);
	}

	free(ctx->bindings);

	if (ctx->error) {
		cel_error_destroy(ctx->error);
	}

	free(ctx);
}

bool cel_context_add_binding(cel_context_t *ctx, const char *name,
			      size_t name_length, cel_value_t value)
{
	if (!ctx || !name) {
		return false;
	}

	/* 扩展绑定数组 */
	if (ctx->binding_count >= ctx->binding_capacity) {
		size_t new_capacity = ctx->binding_capacity == 0 ?
					      4 :
					      ctx->binding_capacity * 2;
		cel_binding_t *new_bindings = realloc(
			ctx->bindings, new_capacity * sizeof(cel_binding_t));
		if (!new_bindings) {
			return false;
		}
		ctx->bindings = new_bindings;
		ctx->binding_capacity = new_capacity;
	}

	/* 添加绑定 */
	ctx->bindings[ctx->binding_count].name = name;
	ctx->bindings[ctx->binding_count].name_length = name_length;
	ctx->bindings[ctx->binding_count].value = value;
	ctx->binding_count++;

	return true;
}

bool cel_context_lookup(const cel_context_t *ctx, const char *name,
			 size_t name_length, cel_value_t *out)
{
	if (!ctx || !name || !out) {
		return false;
	}

	/* 线性查找 */
	for (size_t i = 0; i < ctx->binding_count; i++) {
		if (ctx->bindings[i].name_length == name_length &&
		    memcmp(ctx->bindings[i].name, name, name_length) == 0) {
			*out = ctx->bindings[i].value;
			return true;
		}
	}

	return false;
}

cel_error_t *cel_context_get_error(const cel_context_t *ctx)
{
	return ctx ? ctx->error : NULL;
}

/* ========== 求值主函数 ========== */

bool cel_eval(const cel_ast_node_t *ast, cel_context_t *ctx,
	      cel_value_t *result)
{
	if (!ast || !ctx || !result) {
		return false;
	}

	/* 清除之前的错误 */
	if (ctx->error) {
		cel_error_destroy(ctx->error);
		ctx->error = NULL;
	}

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
		if (!cel_context_lookup(ctx, node->as.ident.name,
					node->as.ident.length, result)) {
			char error_msg[256];
			snprintf(error_msg, sizeof(error_msg),
				 "Undefined variable: %.*s",
				 (int)node->as.ident.length,
				 node->as.ident.name);
			set_error(ctx, error_msg);
			return false;
		}
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

/* ========== 函数调用求值 ========== */

static bool eval_call(const cel_ast_call_t *call, cel_context_t *ctx,
		      cel_value_t *result)
{
	/* 暂时只支持内置函数 */
	if (call->function_length == 4 &&
	    memcmp(call->function, "size", 4) == 0) {
		/* size() 函数 */
		if (call->arg_count != 1) {
			set_error(ctx, "size() requires exactly 1 argument");
			return false;
		}

		cel_value_t arg;
		if (!eval_node(call->args[0], ctx, &arg)) {
			return false;
		}

		if (arg.type == CEL_TYPE_STRING) {
			*result = cel_value_int(
				(int64_t)cel_string_length(&arg));
			return true;
		} else if (arg.type == CEL_TYPE_LIST) {
			*result = cel_value_int((int64_t)arg.value.list_value->length);
			return true;
		} else if (arg.type == CEL_TYPE_MAP) {
			*result = cel_value_int((int64_t)arg.value.map_value->size);
			return true;
		} else {
			set_error(ctx, "size() requires string, list, or map");
			return false;
		}
	}

	char error_msg[256];
	snprintf(error_msg, sizeof(error_msg), "Unknown function: %.*s",
		 (int)call->function_length, call->function);
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

/* ========== 错误处理 ========== */

static void set_error(cel_context_t *ctx, const char *message)
{
	if (ctx->error) {
		cel_error_destroy(ctx->error);
	}
	ctx->error = cel_error_create(CEL_ERROR_INTERNAL, message);
}
