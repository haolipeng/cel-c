/**
 * @file test_macros.c
 * @brief CEL 宏展开器单元测试
 */

#include "cel/cel_macros.h"
#include "cel/cel_ast.h"
#include "cel/cel_memory.h"
#include "unity.h"
#include <string.h>

/* ========== Unity 设置 ========== */

static arena_t *arena = NULL;
static cel_macro_helper_t *helper = NULL;

void setUp(void)
{
	/* 每个测试前创建 Arena 和 Helper */
	arena = arena_create(4096);
	TEST_ASSERT_NOT_NULL(arena);

	helper = cel_macro_helper_create(arena, 1000);
	TEST_ASSERT_NOT_NULL(helper);
}

void tearDown(void)
{
	/* 每个测试后清理 */
	if (helper) {
		cel_macro_helper_destroy(helper);
		helper = NULL;
	}
	if (arena) {
		arena_destroy(arena);
		arena = NULL;
	}
}

/* ========== 辅助函数 ========== */

/**
 * @brief 创建标识符节点
 */
static cel_ast_node_t *create_ident(const char *name)
{
	cel_token_location_t loc = {0};
	return cel_ast_create_ident(name, strlen(name), loc);
}

/**
 * @brief 创建整数字面量节点
 */
static cel_ast_node_t *create_int(int64_t value)
{
	cel_token_location_t loc = {0};
	cel_value_t val = cel_value_int(value);
	return cel_ast_create_literal(val, loc);
}

/**
 * @brief 创建布尔字面量节点
 */
static cel_ast_node_t *create_bool(bool value)
{
	cel_token_location_t loc = {0};
	cel_value_t val = cel_value_bool(value);
	return cel_ast_create_literal(val, loc);
}

/**
 * @brief 创建二元运算节点
 */
static cel_ast_node_t *create_binary(cel_binary_op_e op, cel_ast_node_t *left,
				      cel_ast_node_t *right)
{
	cel_token_location_t loc = {0};
	return cel_ast_create_binary(op, left, right, loc);
}

/**
 * @brief 创建列表字面量节点
 */
static cel_ast_node_t *create_list(cel_ast_node_t **elements, size_t count)
{
	cel_token_location_t loc = {0};
	return cel_ast_create_list(elements, count, loc);
}

/* ========== 宏检测测试 ========== */

void test_macro_detect_all(void)
{
	cel_macro_type_e type = cel_macro_detect("all", true, 2);
	TEST_ASSERT_EQUAL_INT(CEL_MACRO_ALL, type);
}

void test_macro_detect_exists(void)
{
	cel_macro_type_e type = cel_macro_detect("exists", true, 2);
	TEST_ASSERT_EQUAL_INT(CEL_MACRO_EXISTS, type);
}

void test_macro_detect_exists_one(void)
{
	cel_macro_type_e type = cel_macro_detect("exists_one", true, 2);
	TEST_ASSERT_EQUAL_INT(CEL_MACRO_EXISTS_ONE, type);

	type = cel_macro_detect("existsOne", true, 2);
	TEST_ASSERT_EQUAL_INT(CEL_MACRO_EXISTS_ONE, type);
}

void test_macro_detect_map(void)
{
	cel_macro_type_e type = cel_macro_detect("map", true, 2);
	TEST_ASSERT_EQUAL_INT(CEL_MACRO_MAP, type);

	/* map 支持 3 参数形式 */
	type = cel_macro_detect("map", true, 3);
	TEST_ASSERT_EQUAL_INT(CEL_MACRO_MAP, type);
}

void test_macro_detect_filter(void)
{
	cel_macro_type_e type = cel_macro_detect("filter", true, 2);
	TEST_ASSERT_EQUAL_INT(CEL_MACRO_FILTER, type);
}

void test_macro_detect_has(void)
{
	cel_macro_type_e type = cel_macro_detect("has", false, 1);
	TEST_ASSERT_EQUAL_INT(CEL_MACRO_HAS, type);
}

void test_macro_detect_unknown(void)
{
	cel_macro_type_e type = cel_macro_detect("unknown_func", true, 2);
	TEST_ASSERT_EQUAL_INT(CEL_MACRO_UNKNOWN, type);

	/* 参数数量不匹配 */
	type = cel_macro_detect("all", true, 1);
	TEST_ASSERT_EQUAL_INT(CEL_MACRO_UNKNOWN, type);
}

/* ========== all() 宏展开测试 ========== */

void test_macro_expand_all_basic(void)
{
	/* 测试: [1, 2, 3].all(x, x > 0) */

	/* 创建目标列表 [1, 2, 3] */
	cel_ast_node_t **elements = malloc(3 * sizeof(cel_ast_node_t *));
	elements[0] = create_int(1);
	elements[1] = create_int(2);
	elements[2] = create_int(3);
	cel_ast_node_t *target = create_list(elements, 3);

	/* 创建参数: x (标识符), x > 0 (谓词) */
	cel_ast_node_t *args[2] = {
		create_ident("x"),
		create_binary(CEL_BINARY_GT, create_ident("x"), create_int(0))
	};

	/* 展开宏 */
	cel_ast_node_t *result = NULL;
	cel_error_code_e err = cel_macro_expand_all(helper, target, args, 2, &result);

	TEST_ASSERT_EQUAL_INT(CEL_OK, err);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_INT(CEL_AST_COMPREHENSION, result->type);

	/* 验证 Comprehension 结构 */
	cel_ast_comprehension_t *comp = &result->as.comprehension;

	/* 循环变量应该是 "x" */
	TEST_ASSERT_NOT_NULL(comp->iter_var);
	TEST_ASSERT_EQUAL_STRING("x", comp->iter_var);
	TEST_ASSERT_EQUAL_size_t(1, comp->iter_var_length);

	/* 累加器变量应该是 "@result" */
	TEST_ASSERT_NOT_NULL(comp->accu_var);
	TEST_ASSERT_EQUAL_STRING("@result", comp->accu_var);

	/* 迭代范围应该是传入的列表 */
	TEST_ASSERT_EQUAL_PTR(target, comp->iter_range);

	/* 累加器初始值应该是 true (布尔字面量) */
	TEST_ASSERT_NOT_NULL(comp->accu_init);
	TEST_ASSERT_EQUAL_INT(CEL_AST_LITERAL, comp->accu_init->type);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, comp->accu_init->as.literal.value.type);
	TEST_ASSERT_TRUE(comp->accu_init->as.literal.value.value.bool_value);

	/* 循环条件应该是 @result (标识符) */
	TEST_ASSERT_NOT_NULL(comp->loop_cond);
	TEST_ASSERT_EQUAL_INT(CEL_AST_IDENT, comp->loop_cond->type);

	/* 循环步骤应该是 @result && predicate */
	TEST_ASSERT_NOT_NULL(comp->loop_step);
	TEST_ASSERT_EQUAL_INT(CEL_AST_BINARY, comp->loop_step->type);
	TEST_ASSERT_EQUAL_INT(CEL_BINARY_AND, comp->loop_step->as.binary.op);

	/* 结果应该是 @result */
	TEST_ASSERT_NOT_NULL(comp->result);
	TEST_ASSERT_EQUAL_INT(CEL_AST_IDENT, comp->result->type);

	cel_ast_destroy(result);
}

/* ========== exists() 宏展开测试 ========== */

void test_macro_expand_exists_basic(void)
{
	/* 测试: [1, 2, 3].exists(x, x > 2) */

	cel_ast_node_t **elements = malloc(3 * sizeof(cel_ast_node_t *));
	elements[0] = create_int(1);
	elements[1] = create_int(2);
	elements[2] = create_int(3);
	cel_ast_node_t *target = create_list(elements, 3);

	cel_ast_node_t *args[2] = {
		create_ident("x"),
		create_binary(CEL_BINARY_GT, create_ident("x"), create_int(2))
	};

	cel_ast_node_t *result = NULL;
	cel_error_code_e err = cel_macro_expand_exists(helper, target, args, 2, &result);

	TEST_ASSERT_EQUAL_INT(CEL_OK, err);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_INT(CEL_AST_COMPREHENSION, result->type);

	cel_ast_comprehension_t *comp = &result->as.comprehension;

	/* 累加器初始值应该是 false */
	TEST_ASSERT_NOT_NULL(comp->accu_init);
	TEST_ASSERT_EQUAL_INT(CEL_AST_LITERAL, comp->accu_init->type);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, comp->accu_init->as.literal.value.type);
	TEST_ASSERT_FALSE(comp->accu_init->as.literal.value.value.bool_value);

	/* 循环条件应该是 !@result (一元运算) */
	TEST_ASSERT_NOT_NULL(comp->loop_cond);
	TEST_ASSERT_EQUAL_INT(CEL_AST_UNARY, comp->loop_cond->type);
	TEST_ASSERT_EQUAL_INT(CEL_UNARY_NOT, comp->loop_cond->as.unary.op);

	/* 循环步骤应该是 @result || predicate */
	TEST_ASSERT_NOT_NULL(comp->loop_step);
	TEST_ASSERT_EQUAL_INT(CEL_AST_BINARY, comp->loop_step->type);
	TEST_ASSERT_EQUAL_INT(CEL_BINARY_OR, comp->loop_step->as.binary.op);

	cel_ast_destroy(result);
}

/* ========== exists_one() 宏展开测试 ========== */

void test_macro_expand_exists_one_basic(void)
{
	/* 测试: [1, 2, 3].exists_one(x, x == 2) */

	cel_ast_node_t **elements = malloc(3 * sizeof(cel_ast_node_t *));
	elements[0] = create_int(1);
	elements[1] = create_int(2);
	elements[2] = create_int(3);
	cel_ast_node_t *target = create_list(elements, 3);

	cel_ast_node_t *args[2] = {
		create_ident("x"),
		create_binary(CEL_BINARY_EQ, create_ident("x"), create_int(2))
	};

	cel_ast_node_t *result = NULL;
	cel_error_code_e err = cel_macro_expand_exists_one(helper, target, args, 2, &result);

	TEST_ASSERT_EQUAL_INT(CEL_OK, err);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_INT(CEL_AST_COMPREHENSION, result->type);

	cel_ast_comprehension_t *comp = &result->as.comprehension;

	/* 累加器初始值应该是 0 */
	TEST_ASSERT_NOT_NULL(comp->accu_init);
	TEST_ASSERT_EQUAL_INT(CEL_AST_LITERAL, comp->accu_init->type);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, comp->accu_init->as.literal.value.type);
	TEST_ASSERT_EQUAL_INT64(0, comp->accu_init->as.literal.value.value.int_value);

	/* 循环条件应该是 true (不短路) */
	TEST_ASSERT_NOT_NULL(comp->loop_cond);
	TEST_ASSERT_EQUAL_INT(CEL_AST_LITERAL, comp->loop_cond->type);
	TEST_ASSERT_TRUE(comp->loop_cond->as.literal.value.value.bool_value);

	/* 循环步骤应该是 predicate ? (@result + 1) : @result */
	TEST_ASSERT_NOT_NULL(comp->loop_step);
	TEST_ASSERT_EQUAL_INT(CEL_AST_TERNARY, comp->loop_step->type);

	/* 结果应该是 @result == 1 */
	TEST_ASSERT_NOT_NULL(comp->result);
	TEST_ASSERT_EQUAL_INT(CEL_AST_BINARY, comp->result->type);
	TEST_ASSERT_EQUAL_INT(CEL_BINARY_EQ, comp->result->as.binary.op);

	cel_ast_destroy(result);
}

/* ========== map() 宏展开测试 ========== */

void test_macro_expand_map_basic(void)
{
	/* 测试: [1, 2, 3].map(x, x * 2) */

	cel_ast_node_t **elements = malloc(3 * sizeof(cel_ast_node_t *));
	elements[0] = create_int(1);
	elements[1] = create_int(2);
	elements[2] = create_int(3);
	cel_ast_node_t *target = create_list(elements, 3);

	cel_ast_node_t *args[2] = {
		create_ident("x"),
		create_binary(CEL_BINARY_MUL, create_ident("x"), create_int(2))
	};

	cel_ast_node_t *result = NULL;
	cel_error_code_e err = cel_macro_expand_map(helper, target, args, 2, &result);

	TEST_ASSERT_EQUAL_INT(CEL_OK, err);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_INT(CEL_AST_COMPREHENSION, result->type);

	cel_ast_comprehension_t *comp = &result->as.comprehension;

	/* 累加器初始值应该是空列表 [] */
	TEST_ASSERT_NOT_NULL(comp->accu_init);
	TEST_ASSERT_EQUAL_INT(CEL_AST_LIST, comp->accu_init->type);
	TEST_ASSERT_EQUAL_size_t(0, comp->accu_init->as.list.element_count);

	/* 循环条件应该是 true */
	TEST_ASSERT_NOT_NULL(comp->loop_cond);
	TEST_ASSERT_EQUAL_INT(CEL_AST_LITERAL, comp->loop_cond->type);
	TEST_ASSERT_TRUE(comp->loop_cond->as.literal.value.value.bool_value);

	/* 循环步骤应该是 @result + [transform] */
	TEST_ASSERT_NOT_NULL(comp->loop_step);
	TEST_ASSERT_EQUAL_INT(CEL_AST_BINARY, comp->loop_step->type);
	TEST_ASSERT_EQUAL_INT(CEL_BINARY_ADD, comp->loop_step->as.binary.op);

	cel_ast_destroy(result);
}

/* ========== filter() 宏展开测试 ========== */

void test_macro_expand_filter_basic(void)
{
	/* 测试: [1, 2, 3].filter(x, x > 1) */

	cel_ast_node_t **elements = malloc(3 * sizeof(cel_ast_node_t *));
	elements[0] = create_int(1);
	elements[1] = create_int(2);
	elements[2] = create_int(3);
	cel_ast_node_t *target = create_list(elements, 3);

	cel_ast_node_t *args[2] = {
		create_ident("x"),
		create_binary(CEL_BINARY_GT, create_ident("x"), create_int(1))
	};

	cel_ast_node_t *result = NULL;
	cel_error_code_e err = cel_macro_expand_filter(helper, target, args, 2, &result);

	TEST_ASSERT_EQUAL_INT(CEL_OK, err);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_INT(CEL_AST_COMPREHENSION, result->type);

	cel_ast_comprehension_t *comp = &result->as.comprehension;

	/* 累加器初始值应该是空列表 [] */
	TEST_ASSERT_NOT_NULL(comp->accu_init);
	TEST_ASSERT_EQUAL_INT(CEL_AST_LIST, comp->accu_init->type);

	/* 循环步骤应该是 predicate ? (@result + [x]) : @result */
	TEST_ASSERT_NOT_NULL(comp->loop_step);
	TEST_ASSERT_EQUAL_INT(CEL_AST_TERNARY, comp->loop_step->type);

	cel_ast_destroy(result);
}

/* ========== 错误处理测试 ========== */

void test_macro_expand_all_invalid_args(void)
{
	cel_ast_node_t *target = create_list(NULL, 0);
	cel_ast_node_t *result = NULL;

	/* 参数数量不足 */
	cel_ast_node_t *args[1] = { create_ident("x") };
	cel_error_code_e err = cel_macro_expand_all(helper, target, args, 1, &result);
	TEST_ASSERT_TRUE(err != CEL_OK);

	cel_ast_destroy(target);
}

void test_macro_expand_null_helper(void)
{
	cel_ast_node_t *target = create_list(NULL, 0);
	cel_ast_node_t *args[2] = { create_ident("x"), create_bool(true) };
	cel_ast_node_t *result = NULL;

	/* helper 为 NULL */
	cel_error_code_e err = cel_macro_expand_all(NULL, target, args, 2, &result);
	TEST_ASSERT_TRUE(err != CEL_OK);

	cel_ast_destroy(target);
}

/* ========== Main 测试运行器 ========== */

int main(void)
{
	UNITY_BEGIN();

	/* 宏检测测试 */
	RUN_TEST(test_macro_detect_all);
	RUN_TEST(test_macro_detect_exists);
	RUN_TEST(test_macro_detect_exists_one);
	RUN_TEST(test_macro_detect_map);
	RUN_TEST(test_macro_detect_filter);
	RUN_TEST(test_macro_detect_has);
	RUN_TEST(test_macro_detect_unknown);

	/* all() 宏展开测试 */
	RUN_TEST(test_macro_expand_all_basic);

	/* exists() 宏展开测试 */
	RUN_TEST(test_macro_expand_exists_basic);

	/* exists_one() 宏展开测试 */
	RUN_TEST(test_macro_expand_exists_one_basic);

	/* map() 宏展开测试 */
	RUN_TEST(test_macro_expand_map_basic);

	/* filter() 宏展开测试 */
	RUN_TEST(test_macro_expand_filter_basic);

	/* 错误处理测试 */
	RUN_TEST(test_macro_expand_all_invalid_args);
	RUN_TEST(test_macro_expand_null_helper);

	return UNITY_END();
}
