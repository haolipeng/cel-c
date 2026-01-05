/**
 * @file test_eval.c
 * @brief CEL Evaluator 单元测试
 */

#include "cel/cel_eval.h"
#include "cel/cel_parser.h"
#include "unity.h"
#include <string.h>

/* ========== Unity 设置 ========== */

void setUp(void)
{
}

void tearDown(void)
{
}

/* ========== 辅助函数 ========== */

static bool eval_expr(const char *source, cel_context_t *ctx,
		      cel_value_t *result)
{
	cel_lexer_t lexer;
	cel_parser_t parser;

	cel_lexer_init(&lexer, source);
	cel_parser_init(&parser, &lexer);

	cel_ast_node_t *ast = cel_parser_parse(&parser);
	if (!ast) {
		return false;
	}

	bool success = cel_eval(ast, ctx, result);
	cel_ast_destroy(ast);

	return success;
}

/* ========== 字面量求值测试 ========== */

void test_eval_int_literal(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("123", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(123, result.value.int_value);

	cel_context_destroy(ctx);
}

void test_eval_double_literal(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("3.14", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_DOUBLE, result.type);
	TEST_ASSERT_DOUBLE_WITHIN(0.001, 3.14, result.value.double_value);

	cel_context_destroy(ctx);
}

void test_eval_bool_literal(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("true", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);

	TEST_ASSERT_TRUE(eval_expr("false", ctx, &result));
	TEST_ASSERT_FALSE(result.value.bool_value);

	cel_context_destroy(ctx);
}

void test_eval_null_literal(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("null", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_NULL, result.type);

	cel_context_destroy(ctx);
}

/* ========== 变量查找测试 ========== */

void test_eval_variable_lookup(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	cel_context_add_binding(ctx, "x", 1, cel_value_int(42));

	TEST_ASSERT_TRUE(eval_expr("x", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(42, result.value.int_value);

	cel_context_destroy(ctx);
}

void test_eval_undefined_variable(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_FALSE(eval_expr("undefined_var", ctx, &result));
	TEST_ASSERT_NOT_NULL(cel_context_get_error(ctx));

	cel_context_destroy(ctx);
}

/* ========== 一元运算测试 ========== */

void test_eval_unary_neg_int(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("-123", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(-123, result.value.int_value);

	cel_context_destroy(ctx);
}

void test_eval_unary_neg_double(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("-3.14", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_DOUBLE, result.type);
	TEST_ASSERT_DOUBLE_WITHIN(0.001, -3.14, result.value.double_value);

	cel_context_destroy(ctx);
}

void test_eval_unary_not(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("!true", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_FALSE(result.value.bool_value);

	TEST_ASSERT_TRUE(eval_expr("!false", ctx, &result));
	TEST_ASSERT_TRUE(result.value.bool_value);

	cel_context_destroy(ctx);
}

/* ========== 算术运算测试 ========== */

void test_eval_add_int(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("1 + 2", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(3, result.value.int_value);

	cel_context_destroy(ctx);
}

void test_eval_sub_int(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("5 - 3", ctx, &result));
	TEST_ASSERT_EQUAL_INT64(2, result.value.int_value);

	cel_context_destroy(ctx);
}

void test_eval_mul_int(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("3 * 4", ctx, &result));
	TEST_ASSERT_EQUAL_INT64(12, result.value.int_value);

	cel_context_destroy(ctx);
}

void test_eval_div_int(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("10 / 2", ctx, &result));
	TEST_ASSERT_EQUAL_INT64(5, result.value.int_value);

	cel_context_destroy(ctx);
}

void test_eval_div_by_zero(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_FALSE(eval_expr("10 / 0", ctx, &result));
	TEST_ASSERT_NOT_NULL(cel_context_get_error(ctx));

	cel_context_destroy(ctx);
}

void test_eval_mod_int(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("10 % 3", ctx, &result));
	TEST_ASSERT_EQUAL_INT64(1, result.value.int_value);

	cel_context_destroy(ctx);
}

void test_eval_add_double(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("1.5 + 2.5", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_DOUBLE, result.type);
	TEST_ASSERT_DOUBLE_WITHIN(0.001, 4.0, result.value.double_value);

	cel_context_destroy(ctx);
}

void test_eval_mixed_arithmetic(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("1 + 2.5", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_DOUBLE, result.type);
	TEST_ASSERT_DOUBLE_WITHIN(0.001, 3.5, result.value.double_value);

	cel_context_destroy(ctx);
}

void test_eval_string_concat(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("\"hello\" + \" world\"", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_STRING, result.type);

	cel_context_destroy(ctx);
}

/* ========== 比较运算测试 ========== */

void test_eval_eq(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("1 == 1", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);

	TEST_ASSERT_TRUE(eval_expr("1 == 2", ctx, &result));
	TEST_ASSERT_FALSE(result.value.bool_value);

	cel_context_destroy(ctx);
}

void test_eval_ne(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("1 != 2", ctx, &result));
	TEST_ASSERT_TRUE(result.value.bool_value);

	cel_context_destroy(ctx);
}

void test_eval_lt(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("1 < 2", ctx, &result));
	TEST_ASSERT_TRUE(result.value.bool_value);

	TEST_ASSERT_TRUE(eval_expr("2 < 1", ctx, &result));
	TEST_ASSERT_FALSE(result.value.bool_value);

	cel_context_destroy(ctx);
}

void test_eval_le(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("1 <= 1", ctx, &result));
	TEST_ASSERT_TRUE(result.value.bool_value);

	cel_context_destroy(ctx);
}

void test_eval_gt(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("2 > 1", ctx, &result));
	TEST_ASSERT_TRUE(result.value.bool_value);

	cel_context_destroy(ctx);
}

void test_eval_ge(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("2 >= 2", ctx, &result));
	TEST_ASSERT_TRUE(result.value.bool_value);

	cel_context_destroy(ctx);
}

/* ========== 逻辑运算测试 ========== */

void test_eval_and(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("true && true", ctx, &result));
	TEST_ASSERT_TRUE(result.value.bool_value);

	TEST_ASSERT_TRUE(eval_expr("true && false", ctx, &result));
	TEST_ASSERT_FALSE(result.value.bool_value);

	cel_context_destroy(ctx);
}

void test_eval_or(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("false || true", ctx, &result));
	TEST_ASSERT_TRUE(result.value.bool_value);

	TEST_ASSERT_TRUE(eval_expr("false || false", ctx, &result));
	TEST_ASSERT_FALSE(result.value.bool_value);

	cel_context_destroy(ctx);
}

void test_eval_short_circuit_and(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	/* false && undefined_var 应该短路，不会报错 */
	TEST_ASSERT_TRUE(eval_expr("false && undefined_var", ctx, &result));
	TEST_ASSERT_FALSE(result.value.bool_value);

	cel_context_destroy(ctx);
}

void test_eval_short_circuit_or(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	/* true || undefined_var 应该短路，不会报错 */
	TEST_ASSERT_TRUE(eval_expr("true || undefined_var", ctx, &result));
	TEST_ASSERT_TRUE(result.value.bool_value);

	cel_context_destroy(ctx);
}

/* ========== 三元运算测试 ========== */

void test_eval_ternary_true(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("true ? 1 : 2", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(1, result.value.int_value);

	cel_context_destroy(ctx);
}

void test_eval_ternary_false(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("false ? 1 : 2", ctx, &result));
	TEST_ASSERT_EQUAL_INT64(2, result.value.int_value);

	cel_context_destroy(ctx);
}

/* ========== 列表测试 ========== */

void test_eval_empty_list(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("[]", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_LIST, result.type);
	TEST_ASSERT_EQUAL_size_t(0, result.value.list_value->size);

	cel_list_destroy(result.value.list_value);
	cel_context_destroy(ctx);
}

void test_eval_list_with_elements(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("[1, 2, 3]", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_LIST, result.type);
	TEST_ASSERT_EQUAL_size_t(3, result.value.list_value->size);

	cel_list_destroy(result.value.list_value);
	cel_context_destroy(ctx);
}

void test_eval_list_index(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("[1, 2, 3][1]", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(2, result.value.int_value);

	cel_context_destroy(ctx);
}

void test_eval_list_index_out_of_bounds(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_FALSE(eval_expr("[1, 2, 3][10]", ctx, &result));
	TEST_ASSERT_NOT_NULL(cel_context_get_error(ctx));

	cel_context_destroy(ctx);
}

/* ========== Map 测试 ========== */

void test_eval_empty_map(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("{}", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_MAP, result.type);
	TEST_ASSERT_EQUAL_size_t(0, result.value.map_value->size);

	cel_map_destroy(result.value.map_value);
	cel_context_destroy(ctx);
}

void test_eval_map_with_entries(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("{\"a\": 1, \"b\": 2}", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_MAP, result.type);
	TEST_ASSERT_EQUAL_size_t(2, result.value.map_value->size);

	cel_map_destroy(result.value.map_value);
	cel_context_destroy(ctx);
}

void test_eval_map_index(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("{\"a\": 1, \"b\": 2}[\"a\"]", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(1, result.value.int_value);

	cel_context_destroy(ctx);
}

void test_eval_field_access(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("{\"field\": 42}.field", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(42, result.value.int_value);

	cel_context_destroy(ctx);
}

/* ========== 函数调用测试 ========== */

void test_eval_size_string(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("size(\"hello\")", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(5, result.value.int_value);

	cel_context_destroy(ctx);
}

void test_eval_size_list(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("size([1, 2, 3])", ctx, &result));
	TEST_ASSERT_EQUAL_INT64(3, result.value.int_value);

	cel_context_destroy(ctx);
}

void test_eval_size_map(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("size({\"a\": 1, \"b\": 2})", ctx, &result));
	TEST_ASSERT_EQUAL_INT64(2, result.value.int_value);

	cel_context_destroy(ctx);
}

/* ========== in 运算符测试 ========== */

void test_eval_in_list(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("2 in [1, 2, 3]", ctx, &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);

	TEST_ASSERT_TRUE(eval_expr("5 in [1, 2, 3]", ctx, &result));
	TEST_ASSERT_FALSE(result.value.bool_value);

	cel_context_destroy(ctx);
}

void test_eval_in_map(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	TEST_ASSERT_TRUE(eval_expr("\"a\" in {\"a\": 1, \"b\": 2}", ctx, &result));
	TEST_ASSERT_TRUE(result.value.bool_value);

	TEST_ASSERT_TRUE(eval_expr("\"c\" in {\"a\": 1, \"b\": 2}", ctx, &result));
	TEST_ASSERT_FALSE(result.value.bool_value);

	cel_context_destroy(ctx);
}

/* ========== 复杂表达式测试 ========== */

void test_eval_complex_expression(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t result;

	cel_context_add_binding(ctx, "x", 1, cel_value_int(5));
	cel_context_add_binding(ctx, "y", 1, cel_value_int(10));

	TEST_ASSERT_TRUE(eval_expr("(x + y) * 2 > 20 ? true : false", ctx,
				   &result));
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);

	cel_context_destroy(ctx);
}

/* ========== Unity 主函数 ========== */

int main(void)
{
	UNITY_BEGIN();

	/* 字面量测试 */
	RUN_TEST(test_eval_int_literal);
	RUN_TEST(test_eval_double_literal);
	RUN_TEST(test_eval_bool_literal);
	RUN_TEST(test_eval_null_literal);

	/* 变量查找测试 */
	RUN_TEST(test_eval_variable_lookup);
	RUN_TEST(test_eval_undefined_variable);

	/* 一元运算测试 */
	RUN_TEST(test_eval_unary_neg_int);
	RUN_TEST(test_eval_unary_neg_double);
	RUN_TEST(test_eval_unary_not);

	/* 算术运算测试 */
	RUN_TEST(test_eval_add_int);
	RUN_TEST(test_eval_sub_int);
	RUN_TEST(test_eval_mul_int);
	RUN_TEST(test_eval_div_int);
	RUN_TEST(test_eval_div_by_zero);
	RUN_TEST(test_eval_mod_int);
	RUN_TEST(test_eval_add_double);
	RUN_TEST(test_eval_mixed_arithmetic);
	RUN_TEST(test_eval_string_concat);

	/* 比较运算测试 */
	RUN_TEST(test_eval_eq);
	RUN_TEST(test_eval_ne);
	RUN_TEST(test_eval_lt);
	RUN_TEST(test_eval_le);
	RUN_TEST(test_eval_gt);
	RUN_TEST(test_eval_ge);

	/* 逻辑运算测试 */
	RUN_TEST(test_eval_and);
	RUN_TEST(test_eval_or);
	RUN_TEST(test_eval_short_circuit_and);
	RUN_TEST(test_eval_short_circuit_or);

	/* 三元运算测试 */
	RUN_TEST(test_eval_ternary_true);
	RUN_TEST(test_eval_ternary_false);

	/* 列表测试 */
	RUN_TEST(test_eval_empty_list);
	RUN_TEST(test_eval_list_with_elements);
	RUN_TEST(test_eval_list_index);
	RUN_TEST(test_eval_list_index_out_of_bounds);

	/* Map 测试 */
	RUN_TEST(test_eval_empty_map);
	RUN_TEST(test_eval_map_with_entries);
	RUN_TEST(test_eval_map_index);
	RUN_TEST(test_eval_field_access);

	/* 函数调用测试 */
	RUN_TEST(test_eval_size_string);
	RUN_TEST(test_eval_size_list);
	RUN_TEST(test_eval_size_map);

	/* in 运算符测试 */
	RUN_TEST(test_eval_in_list);
	RUN_TEST(test_eval_in_map);

	/* 复杂表达式测试 */
	RUN_TEST(test_eval_complex_expression);

	return UNITY_END();
}
