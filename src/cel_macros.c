/**
 * @file cel_macros.c
 * @brief CEL 宏展开器实现
 */

#include "cel/cel_macros.h"
#include "cel/cel_memory.h"
#include <stdlib.h>
#include <string.h>

/* ========== 宏检测 ========== */

cel_macro_type_e cel_macro_detect(const char *func_name, bool has_target,
				    size_t arg_count)
{
	if (!func_name) {
		return CEL_MACRO_UNKNOWN;
	}

	size_t name_len = strlen(func_name);

	/* has() - 不需要 target, 1 个参数 */
	if (name_len == 3 && memcmp(func_name, "has", 3) == 0) {
		if (!has_target && arg_count == 1) {
			return CEL_MACRO_HAS;
		}
	}

	/* all() - 需要 target, 2 个参数 */
	if (name_len == 3 && memcmp(func_name, "all", 3) == 0) {
		if (has_target && arg_count == 2) {
			return CEL_MACRO_ALL;
		}
	}

	/* exists() - 需要 target, 2 个参数 */
	if (name_len == 6 && memcmp(func_name, "exists", 6) == 0) {
		if (has_target && arg_count == 2) {
			return CEL_MACRO_EXISTS;
		}
	}

	/* exists_one() 或 existsOne() - 需要 target, 2 个参数 */
	if ((name_len == 10 && memcmp(func_name, "exists_one", 10) == 0) ||
	    (name_len == 9 && memcmp(func_name, "existsOne", 9) == 0)) {
		if (has_target && arg_count == 2) {
			return CEL_MACRO_EXISTS_ONE;
		}
	}

	/* map() - 需要 target, 2 或 3 个参数 */
	if (name_len == 3 && memcmp(func_name, "map", 3) == 0) {
		if (has_target && (arg_count == 2 || arg_count == 3)) {
			return CEL_MACRO_MAP;
		}
	}

	/* filter() - 需要 target, 2 个参数 */
	if (name_len == 6 && memcmp(func_name, "filter", 6) == 0) {
		if (has_target && arg_count == 2) {
			return CEL_MACRO_FILTER;
		}
	}

	return CEL_MACRO_UNKNOWN;
}

/* ========== 辅助器管理 ========== */

cel_macro_helper_t *cel_macro_helper_create(arena_t *arena,
					      uint64_t start_id)
{
	if (!arena) {
		return NULL;
	}

	cel_macro_helper_t *helper = malloc(sizeof(cel_macro_helper_t));
	if (!helper) {
		return NULL;
	}

	helper->arena = arena;
	helper->next_id = start_id;

	return helper;
}

void cel_macro_helper_destroy(cel_macro_helper_t *helper)
{
	if (helper) {
		free(helper);
	}
}

/* ========== 辅助函数 ========== */

/**
 * @brief 从标识符节点提取变量名
 */
static cel_error_code_e extract_ident(cel_ast_node_t *node,
					const char **var_name, size_t *var_len)
{
	if (!node || node->type != CEL_AST_IDENT) {
		return CEL_ERROR_INVALID_ARGUMENT;
	}

	*var_name = node->as.ident.name;
	*var_len = node->as.ident.length;
	return CEL_OK;
}

/**
 * @brief 创建累加器标识符节点
 */
cel_ast_node_t *cel_macro_create_accu_ident(cel_macro_helper_t *helper)
{
	(void)helper; /* Unused for now */
	cel_token_location_t loc = {0};
	return cel_ast_create_ident("@result", 7, loc);
}

/**
 * @brief 创建布尔字面量节点
 */
cel_ast_node_t *cel_macro_create_bool_literal(cel_macro_helper_t *helper,
						bool value)
{
	(void)helper; /* Unused for now */
	cel_token_location_t loc = {0};
	cel_value_t val = cel_value_bool(value);
	return cel_ast_create_literal(val, loc);
}

/**
 * @brief 创建整数字面量节点
 */
cel_ast_node_t *cel_macro_create_int_literal(cel_macro_helper_t *helper,
					       int64_t value)
{
	(void)helper; /* Unused for now */
	cel_token_location_t loc = {0};
	cel_value_t val = cel_value_int(value);
	return cel_ast_create_literal(val, loc);
}

/**
 * @brief 创建空列表字面量节点
 */
cel_ast_node_t *cel_macro_create_empty_list(cel_macro_helper_t *helper)
{
	(void)helper; /* Unused for now */
	cel_token_location_t loc = {0};
	return cel_ast_create_list(NULL, 0, loc);
}

/* ========== 宏展开实现 ========== */

/**
 * @brief 展开 all() 宏
 *
 * list.all(x, predicate) =>
 *   Comprehension(
 *     iter_var: x,
 *     iter_range: list,
 *     accu_var: @result,
 *     accu_init: true,
 *     loop_cond: @result,
 *     loop_step: @result && predicate,
 *     result: @result
 *   )
 */
cel_error_code_e cel_macro_expand_all(cel_macro_helper_t *helper,
					cel_ast_node_t *target,
					cel_ast_node_t **args, size_t arg_count,
					cel_ast_node_t **result)
{
	if (!helper || !target || !args || arg_count != 2 || !result) {
		return CEL_ERROR_INVALID_ARGUMENT;
	}

	/* 提取循环变量名 */
	const char *iter_var = NULL;
	size_t iter_var_len = 0;
	cel_error_code_e err = extract_ident(args[0], &iter_var, &iter_var_len);
	if (err != CEL_OK) {
		return err;
	}

	cel_ast_node_t *predicate = args[1];
	cel_token_location_t loc = {0};

	/* 创建 Comprehension 节点 */
	*result = cel_ast_create_comprehension(
		iter_var, iter_var_len,  /* iter_var */
		NULL, 0,                  /* iter_var2 (unused) */
		target,                   /* iter_range */
		"@result", 7,             /* accu_var */
		cel_macro_create_bool_literal(helper, true), /* accu_init: true */
		cel_macro_create_accu_ident(helper),         /* loop_cond: @result */
		cel_ast_create_binary(CEL_BINARY_AND,
				       cel_macro_create_accu_ident(helper),
				       predicate, loc), /* loop_step: @result && predicate */
		cel_macro_create_accu_ident(helper),  /* result: @result */
		loc
	);

	return (*result) ? CEL_OK : CEL_ERROR_OUT_OF_MEMORY;
}

/**
 * @brief 展开 exists() 宏
 *
 * list.exists(x, predicate) =>
 *   Comprehension(
 *     iter_var: x,
 *     iter_range: list,
 *     accu_var: @result,
 *     accu_init: false,
 *     loop_cond: !@result,
 *     loop_step: @result || predicate,
 *     result: @result
 *   )
 */
cel_error_code_e cel_macro_expand_exists(cel_macro_helper_t *helper,
					   cel_ast_node_t *target,
					   cel_ast_node_t **args,
					   size_t arg_count,
					   cel_ast_node_t **result)
{
	if (!helper || !target || !args || arg_count != 2 || !result) {
		return CEL_ERROR_INVALID_ARGUMENT;
	}

	const char *iter_var = NULL;
	size_t iter_var_len = 0;
	cel_error_code_e err = extract_ident(args[0], &iter_var, &iter_var_len);
	if (err != CEL_OK) {
		return err;
	}

	cel_ast_node_t *predicate = args[1];
	cel_token_location_t loc = {0};

	*result = cel_ast_create_comprehension(
		iter_var, iter_var_len,
		NULL, 0,
		target,
		"@result", 7,
		cel_macro_create_bool_literal(helper, false), /* accu_init: false */
		cel_ast_create_unary(CEL_UNARY_NOT,
				      cel_macro_create_accu_ident(helper),
				      loc), /* loop_cond: !@result */
		cel_ast_create_binary(CEL_BINARY_OR,
				       cel_macro_create_accu_ident(helper),
				       predicate, loc), /* loop_step: @result || predicate */
		cel_macro_create_accu_ident(helper),
		loc
	);

	return (*result) ? CEL_OK : CEL_ERROR_OUT_OF_MEMORY;
}

/**
 * @brief 展开 exists_one() 宏
 *
 * list.exists_one(x, predicate) =>
 *   Comprehension(
 *     iter_var: x,
 *     iter_range: list,
 *     accu_var: @result,
 *     accu_init: 0,
 *     loop_cond: true,
 *     loop_step: predicate ? (@result + 1) : @result,
 *     result: @result == 1
 *   )
 */
cel_error_code_e cel_macro_expand_exists_one(cel_macro_helper_t *helper,
					       cel_ast_node_t *target,
					       cel_ast_node_t **args,
					       size_t arg_count,
					       cel_ast_node_t **result)
{
	if (!helper || !target || !args || arg_count != 2 || !result) {
		return CEL_ERROR_INVALID_ARGUMENT;
	}

	const char *iter_var = NULL;
	size_t iter_var_len = 0;
	cel_error_code_e err = extract_ident(args[0], &iter_var, &iter_var_len);
	if (err != CEL_OK) {
		return err;
	}

	cel_ast_node_t *predicate = args[1];
	cel_token_location_t loc = {0};

	*result = cel_ast_create_comprehension(
		iter_var, iter_var_len,
		NULL, 0,
		target,
		"@result", 7,
		cel_macro_create_int_literal(helper, 0), /* accu_init: 0 */
		cel_macro_create_bool_literal(helper, true), /* loop_cond: true */
		cel_ast_create_ternary(
			predicate,
			cel_ast_create_binary(CEL_BINARY_ADD,
					       cel_macro_create_accu_ident(helper),
					       cel_macro_create_int_literal(helper, 1),
					       loc),
			cel_macro_create_accu_ident(helper),
			loc), /* loop_step: predicate ? (@result + 1) : @result */
		cel_ast_create_binary(CEL_BINARY_EQ,
				       cel_macro_create_accu_ident(helper),
				       cel_macro_create_int_literal(helper, 1),
				       loc), /* result: @result == 1 */
		loc
	);

	return (*result) ? CEL_OK : CEL_ERROR_OUT_OF_MEMORY;
}

/**
 * @brief 展开 map() 宏
 *
 * list.map(x, transform) =>
 *   Comprehension(
 *     iter_var: x,
 *     iter_range: list,
 *     accu_var: @result,
 *     accu_init: [],
 *     loop_cond: true,
 *     loop_step: @result + [transform],
 *     result: @result
 *   )
 */
cel_error_code_e cel_macro_expand_map(cel_macro_helper_t *helper,
					cel_ast_node_t *target,
					cel_ast_node_t **args, size_t arg_count,
					cel_ast_node_t **result)
{
	if (!helper || !target || !args || (arg_count != 2 && arg_count != 3) ||
	    !result) {
		return CEL_ERROR_INVALID_ARGUMENT;
	}

	const char *iter_var = NULL;
	size_t iter_var_len = 0;
	cel_error_code_e err = extract_ident(args[0], &iter_var, &iter_var_len);
	if (err != CEL_OK) {
		return err;
	}

	cel_token_location_t loc = {0};

	/* 2 参数形式: map(x, transform) */
	if (arg_count == 2) {
		cel_ast_node_t *transform = args[1];

		/* 创建 [transform] 列表 */
		cel_ast_node_t **list_elements = malloc(sizeof(cel_ast_node_t *));
		if (!list_elements) {
			return CEL_ERROR_OUT_OF_MEMORY;
		}
		list_elements[0] = transform;
		cel_ast_node_t *transform_list = cel_ast_create_list(list_elements, 1, loc);

		*result = cel_ast_create_comprehension(
			iter_var, iter_var_len,
			NULL, 0,
			target,
			"@result", 7,
			cel_macro_create_empty_list(helper), /* accu_init: [] */
			cel_macro_create_bool_literal(helper, true), /* loop_cond: true */
			cel_ast_create_binary(CEL_BINARY_ADD,
					       cel_macro_create_accu_ident(helper),
					       transform_list,
					       loc), /* loop_step: @result + [transform] */
			cel_macro_create_accu_ident(helper),
			loc
		);
	}
	/* 3 参数形式: map(x, filter, transform) */
	else {
		cel_ast_node_t *filter = args[1];
		cel_ast_node_t *transform = args[2];

		/* 创建 [transform] 列表 */
		cel_ast_node_t **list_elements = malloc(sizeof(cel_ast_node_t *));
		if (!list_elements) {
			return CEL_ERROR_OUT_OF_MEMORY;
		}
		list_elements[0] = transform;
		cel_ast_node_t *transform_list = cel_ast_create_list(list_elements, 1, loc);

		*result = cel_ast_create_comprehension(
			iter_var, iter_var_len,
			NULL, 0,
			target,
			"@result", 7,
			cel_macro_create_empty_list(helper),
			cel_macro_create_bool_literal(helper, true),
			cel_ast_create_ternary(
				filter,
				cel_ast_create_binary(CEL_BINARY_ADD,
						       cel_macro_create_accu_ident(helper),
						       transform_list,
						       loc),
				cel_macro_create_accu_ident(helper),
				loc), /* loop_step: filter ? (@result + [transform]) : @result */
			cel_macro_create_accu_ident(helper),
			loc
		);
	}

	return (*result) ? CEL_OK : CEL_ERROR_OUT_OF_MEMORY;
}

/**
 * @brief 展开 filter() 宏
 *
 * list.filter(x, predicate) =>
 *   Comprehension(
 *     iter_var: x,
 *     iter_range: list,
 *     accu_var: @result,
 *     accu_init: [],
 *     loop_cond: true,
 *     loop_step: predicate ? (@result + [x]) : @result,
 *     result: @result
 *   )
 */
cel_error_code_e cel_macro_expand_filter(cel_macro_helper_t *helper,
					   cel_ast_node_t *target,
					   cel_ast_node_t **args,
					   size_t arg_count,
					   cel_ast_node_t **result)
{
	if (!helper || !target || !args || arg_count != 2 || !result) {
		return CEL_ERROR_INVALID_ARGUMENT;
	}

	const char *iter_var = NULL;
	size_t iter_var_len = 0;
	cel_error_code_e err = extract_ident(args[0], &iter_var, &iter_var_len);
	if (err != CEL_OK) {
		return err;
	}

	cel_ast_node_t *predicate = args[1];
	cel_token_location_t loc = {0};

	/* 创建 [x] 列表 (循环变量本身) */
	cel_ast_node_t **list_elements = malloc(sizeof(cel_ast_node_t *));
	if (!list_elements) {
		return CEL_ERROR_OUT_OF_MEMORY;
	}
	list_elements[0] = cel_ast_create_ident(iter_var, iter_var_len, loc);
	cel_ast_node_t *x_list = cel_ast_create_list(list_elements, 1, loc);

	*result = cel_ast_create_comprehension(
		iter_var, iter_var_len,
		NULL, 0,
		target,
		"@result", 7,
		cel_macro_create_empty_list(helper), /* accu_init: [] */
		cel_macro_create_bool_literal(helper, true), /* loop_cond: true */
		cel_ast_create_ternary(
			predicate,
			cel_ast_create_binary(CEL_BINARY_ADD,
					       cel_macro_create_accu_ident(helper),
					       x_list,
					       loc),
			cel_macro_create_accu_ident(helper),
			loc), /* loop_step: predicate ? (@result + [x]) : @result */
		cel_macro_create_accu_ident(helper),
		loc
	);

	return (*result) ? CEL_OK : CEL_ERROR_OUT_OF_MEMORY;
}

/**
 * @brief 展开 has() 宏
 *
 * has(obj.field) => obj.field (with optional=true flag)
 * 注意: has() 实际上只是将字段访问标记为可选访问
 */
cel_error_code_e cel_macro_expand_has(cel_macro_helper_t *helper,
					cel_ast_node_t **args, size_t arg_count,
					cel_ast_node_t **result)
{
	if (!helper || !args || arg_count != 1 || !result) {
		return CEL_ERROR_INVALID_ARGUMENT;
	}

	/* has() 的参数应该是 SELECT 节点 */
	cel_ast_node_t *arg = args[0];
	if (arg->type != CEL_AST_SELECT) {
		return CEL_ERROR_INVALID_ARGUMENT;
	}

	/* 将 SELECT 节点标记为可选访问 */
	arg->as.select.optional = true;
	*result = arg;

	return CEL_OK;
}

/**
 * @brief 统一宏展开入口
 */
cel_error_code_e cel_macro_expand(cel_macro_helper_t *helper,
				    cel_macro_type_e macro_type,
				    cel_ast_node_t *target,
				    cel_ast_node_t **args, size_t arg_count,
				    cel_ast_node_t **result)
{
	if (!helper || !result) {
		return CEL_ERROR_INVALID_ARGUMENT;
	}

	switch (macro_type) {
	case CEL_MACRO_HAS:
		return cel_macro_expand_has(helper, args, arg_count, result);

	case CEL_MACRO_ALL:
		return cel_macro_expand_all(helper, target, args, arg_count, result);

	case CEL_MACRO_EXISTS:
		return cel_macro_expand_exists(helper, target, args, arg_count, result);

	case CEL_MACRO_EXISTS_ONE:
		return cel_macro_expand_exists_one(helper, target, args, arg_count, result);

	case CEL_MACRO_MAP:
		return cel_macro_expand_map(helper, target, args, arg_count, result);

	case CEL_MACRO_FILTER:
		return cel_macro_expand_filter(helper, target, args, arg_count, result);

	default:
		return CEL_ERROR_UNSUPPORTED;
	}
}
