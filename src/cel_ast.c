/**
 * @file cel_ast.c
 * @brief CEL AST 实现
 */

#include "cel/cel_ast.h"
#include <stdlib.h>
#include <string.h>

/* ========== AST 辅助函数 ========== */

const char *cel_ast_node_type_name(cel_ast_node_type_e type)
{
	switch (type) {
	case CEL_AST_LITERAL:
		return "LITERAL";
	case CEL_AST_IDENT:
		return "IDENT";
	case CEL_AST_UNARY:
		return "UNARY";
	case CEL_AST_BINARY:
		return "BINARY";
	case CEL_AST_TERNARY:
		return "TERNARY";
	case CEL_AST_SELECT:
		return "SELECT";
	case CEL_AST_INDEX:
		return "INDEX";
	case CEL_AST_CALL:
		return "CALL";
	case CEL_AST_LIST:
		return "LIST";
	case CEL_AST_MAP:
		return "MAP";
	case CEL_AST_STRUCT:
		return "STRUCT";
	default:
		return "<unknown>";
	}
}

const char *cel_unary_op_name(cel_unary_op_e op)
{
	switch (op) {
	case CEL_UNARY_NEG:
		return "-";
	case CEL_UNARY_NOT:
		return "!";
	default:
		return "<unknown>";
	}
}

const char *cel_binary_op_name(cel_binary_op_e op)
{
	switch (op) {
	case CEL_BINARY_ADD:
		return "+";
	case CEL_BINARY_SUB:
		return "-";
	case CEL_BINARY_MUL:
		return "*";
	case CEL_BINARY_DIV:
		return "/";
	case CEL_BINARY_MOD:
		return "%";
	case CEL_BINARY_EQ:
		return "==";
	case CEL_BINARY_NE:
		return "!=";
	case CEL_BINARY_LT:
		return "<";
	case CEL_BINARY_LE:
		return "<=";
	case CEL_BINARY_GT:
		return ">";
	case CEL_BINARY_GE:
		return ">=";
	case CEL_BINARY_AND:
		return "&&";
	case CEL_BINARY_OR:
		return "||";
	case CEL_BINARY_IN:
		return "in";
	default:
		return "<unknown>";
	}
}

/* ========== AST 创建函数 ========== */

cel_ast_node_t *cel_ast_create_literal(cel_value_t value,
					cel_token_location_t loc)
{
	cel_ast_node_t *node = malloc(sizeof(cel_ast_node_t));
	if (!node) {
		return NULL;
	}

	node->type = CEL_AST_LITERAL;
	node->loc = loc;
	node->as.literal.value = value;

	return node;
}

cel_ast_node_t *cel_ast_create_ident(const char *name, size_t length,
				      cel_token_location_t loc)
{
	cel_ast_node_t *node = malloc(sizeof(cel_ast_node_t));
	if (!node) {
		return NULL;
	}

	node->type = CEL_AST_IDENT;
	node->loc = loc;
	node->as.ident.name = name;
	node->as.ident.length = length;

	return node;
}

cel_ast_node_t *cel_ast_create_unary(cel_unary_op_e op,
				      cel_ast_node_t *operand,
				      cel_token_location_t loc)
{
	cel_ast_node_t *node = malloc(sizeof(cel_ast_node_t));
	if (!node) {
		return NULL;
	}

	node->type = CEL_AST_UNARY;
	node->loc = loc;
	node->as.unary.op = op;
	node->as.unary.operand = operand;

	return node;
}

cel_ast_node_t *cel_ast_create_binary(cel_binary_op_e op,
				       cel_ast_node_t *left,
				       cel_ast_node_t *right,
				       cel_token_location_t loc)
{
	cel_ast_node_t *node = malloc(sizeof(cel_ast_node_t));
	if (!node) {
		return NULL;
	}

	node->type = CEL_AST_BINARY;
	node->loc = loc;
	node->as.binary.op = op;
	node->as.binary.left = left;
	node->as.binary.right = right;

	return node;
}

cel_ast_node_t *cel_ast_create_ternary(cel_ast_node_t *condition,
					cel_ast_node_t *if_true,
					cel_ast_node_t *if_false,
					cel_token_location_t loc)
{
	cel_ast_node_t *node = malloc(sizeof(cel_ast_node_t));
	if (!node) {
		return NULL;
	}

	node->type = CEL_AST_TERNARY;
	node->loc = loc;
	node->as.ternary.condition = condition;
	node->as.ternary.if_true = if_true;
	node->as.ternary.if_false = if_false;

	return node;
}

cel_ast_node_t *cel_ast_create_select(cel_ast_node_t *operand,
				       const char *field, size_t field_length,
				       bool optional, cel_token_location_t loc)
{
	cel_ast_node_t *node = malloc(sizeof(cel_ast_node_t));
	if (!node) {
		return NULL;
	}

	node->type = CEL_AST_SELECT;
	node->loc = loc;
	node->as.select.operand = operand;
	node->as.select.field = field;
	node->as.select.field_length = field_length;
	node->as.select.optional = optional;

	return node;
}

cel_ast_node_t *cel_ast_create_index(cel_ast_node_t *operand,
				      cel_ast_node_t *index, bool optional,
				      cel_token_location_t loc)
{
	cel_ast_node_t *node = malloc(sizeof(cel_ast_node_t));
	if (!node) {
		return NULL;
	}

	node->type = CEL_AST_INDEX;
	node->loc = loc;
	node->as.index.operand = operand;
	node->as.index.index = index;
	node->as.index.optional = optional;

	return node;
}

cel_ast_node_t *cel_ast_create_call(const char *function,
				     size_t function_length,
				     cel_ast_node_t *target,
				     cel_ast_node_t **args, size_t arg_count,
				     cel_token_location_t loc)
{
	cel_ast_node_t *node = malloc(sizeof(cel_ast_node_t));
	if (!node) {
		return NULL;
	}

	node->type = CEL_AST_CALL;
	node->loc = loc;
	node->as.call.function = function;
	node->as.call.function_length = function_length;
	node->as.call.target = target;
	node->as.call.args = args;
	node->as.call.arg_count = arg_count;

	return node;
}

cel_ast_node_t *cel_ast_create_list(cel_ast_node_t **elements,
				     size_t element_count,
				     cel_token_location_t loc)
{
	cel_ast_node_t *node = malloc(sizeof(cel_ast_node_t));
	if (!node) {
		return NULL;
	}

	node->type = CEL_AST_LIST;
	node->loc = loc;
	node->as.list.elements = elements;
	node->as.list.element_count = element_count;

	return node;
}

cel_ast_node_t *cel_ast_create_map(cel_ast_map_entry_t *entries,
				    size_t entry_count,
				    cel_token_location_t loc)
{
	cel_ast_node_t *node = malloc(sizeof(cel_ast_node_t));
	if (!node) {
		return NULL;
	}

	node->type = CEL_AST_MAP;
	node->loc = loc;
	node->as.map.entries = entries;
	node->as.map.entry_count = entry_count;

	return node;
}

cel_ast_node_t *cel_ast_create_struct(const char *type_name,
				       size_t type_name_length,
				       cel_ast_struct_field_t *fields,
				       size_t field_count,
				       cel_token_location_t loc)
{
	cel_ast_node_t *node = malloc(sizeof(cel_ast_node_t));
	if (!node) {
		return NULL;
	}

	node->type = CEL_AST_STRUCT;
	node->loc = loc;
	node->as.struct_lit.type_name = type_name;
	node->as.struct_lit.type_name_length = type_name_length;
	node->as.struct_lit.fields = fields;
	node->as.struct_lit.field_count = field_count;

	return node;
}

/* ========== AST 销毁函数 ========== */

void cel_ast_destroy(cel_ast_node_t *node)
{
	if (!node) {
		return;
	}

	switch (node->type) {
	case CEL_AST_LITERAL:
		cel_value_destroy(&node->as.literal.value);
		break;

	case CEL_AST_IDENT:
		/* 标识符名称指向源代码，不需要释放 */
		break;

	case CEL_AST_UNARY:
		cel_ast_destroy(node->as.unary.operand);
		break;

	case CEL_AST_BINARY:
		cel_ast_destroy(node->as.binary.left);
		cel_ast_destroy(node->as.binary.right);
		break;

	case CEL_AST_TERNARY:
		cel_ast_destroy(node->as.ternary.condition);
		cel_ast_destroy(node->as.ternary.if_true);
		cel_ast_destroy(node->as.ternary.if_false);
		break;

	case CEL_AST_SELECT:
		cel_ast_destroy(node->as.select.operand);
		/* 字段名指向源代码，不需要释放 */
		break;

	case CEL_AST_INDEX:
		cel_ast_destroy(node->as.index.operand);
		cel_ast_destroy(node->as.index.index);
		break;

	case CEL_AST_CALL:
		cel_ast_destroy(node->as.call.target);
		for (size_t i = 0; i < node->as.call.arg_count; i++) {
			cel_ast_destroy(node->as.call.args[i]);
		}
		free(node->as.call.args);
		/* 函数名指向源代码，不需要释放 */
		break;

	case CEL_AST_LIST:
		for (size_t i = 0; i < node->as.list.element_count; i++) {
			cel_ast_destroy(node->as.list.elements[i]);
		}
		free(node->as.list.elements);
		break;

	case CEL_AST_MAP:
		for (size_t i = 0; i < node->as.map.entry_count; i++) {
			cel_ast_destroy(node->as.map.entries[i].key);
			cel_ast_destroy(node->as.map.entries[i].value);
		}
		free(node->as.map.entries);
		break;

	case CEL_AST_STRUCT:
		for (size_t i = 0; i < node->as.struct_lit.field_count; i++) {
			cel_ast_destroy(node->as.struct_lit.fields[i].value);
			/* 字段名指向源代码，不需要释放 */
		}
		free(node->as.struct_lit.fields);
		/* 类型名指向源代码，不需要释放 */
		break;
	}

	free(node);
}
