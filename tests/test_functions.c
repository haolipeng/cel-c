/**
 * @file test_functions.c
 * @brief CEL 内置函数单元测试
 */

#include "cel/cel_eval.h"
#include "cel/cel_parser.h"
#include "cel/cel_context.h"
#include "unity.h"
#include <string.h>
#include <stdlib.h>

/* ========== Unity 设置 ========== */

static cel_context_t *ctx = NULL;

void setUp(void)
{
	ctx = cel_context_create();
	TEST_ASSERT_NOT_NULL(ctx);
}

void tearDown(void)
{
	if (ctx) {
		cel_context_destroy(ctx);
		ctx = NULL;
	}
}

/* ========== 辅助函数 ========== */

static cel_value_t eval_expression(const char *expr)
{
	cel_parse_result_t parse_result = cel_parse(expr);
	TEST_ASSERT_FALSE(parse_result.has_errors);
	TEST_ASSERT_NOT_NULL(parse_result.ast);

	cel_value_t result;
	bool success = cel_eval(parse_result.ast, ctx, &result);
	TEST_ASSERT_TRUE(success);

	cel_parse_result_destroy(&parse_result);
	return result;
}

/* ========== size() 函数测试 ========== */

void test_size_string(void)
{
	cel_value_t result = eval_expression("size(\"hello\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(5, result.value.int_value);
}

void test_size_empty_string(void)
{
	cel_value_t result = eval_expression("size(\"\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(0, result.value.int_value);
}

void test_size_list(void)
{
	cel_value_t result = eval_expression("size([1, 2, 3])");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(3, result.value.int_value);
	cel_value_destroy(&result);
}

void test_size_empty_list(void)
{
	cel_value_t result = eval_expression("size([])");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(0, result.value.int_value);
	cel_value_destroy(&result);
}

void test_size_map(void)
{
	cel_value_t result = eval_expression("size({\"a\": 1, \"b\": 2})");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(2, result.value.int_value);
	cel_value_destroy(&result);
}

/* ========== int() 类型转换测试 ========== */

void test_int_from_double(void)
{
	cel_value_t result = eval_expression("int(3.14)");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(3, result.value.int_value);
}

void test_int_from_string(void)
{
	cel_value_t result = eval_expression("int(\"42\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(42, result.value.int_value);
}

void test_int_from_negative_string(void)
{
	cel_value_t result = eval_expression("int(\"-123\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(-123, result.value.int_value);
}

/* ========== double() 类型转换测试 ========== */

void test_double_from_int(void)
{
	cel_value_t result = eval_expression("double(42)");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_DOUBLE, result.type);
	TEST_ASSERT_DOUBLE_WITHIN(0.001, 42.0, result.value.double_value);
}

void test_double_from_string(void)
{
	cel_value_t result = eval_expression("double(\"3.14\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_DOUBLE, result.type);
	TEST_ASSERT_DOUBLE_WITHIN(0.001, 3.14, result.value.double_value);
}

/* ========== string() 类型转换测试 ========== */

void test_string_from_int(void)
{
	cel_value_t result = eval_expression("string(42)");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_STRING, result.type);
	TEST_ASSERT_EQUAL_STRING("42", result.value.string_value->data);
	cel_value_destroy(&result);
}

void test_string_from_bool_true(void)
{
	cel_value_t result = eval_expression("string(true)");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_STRING, result.type);
	TEST_ASSERT_EQUAL_STRING("true", result.value.string_value->data);
	cel_value_destroy(&result);
}

void test_string_from_bool_false(void)
{
	cel_value_t result = eval_expression("string(false)");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_STRING, result.type);
	TEST_ASSERT_EQUAL_STRING("false", result.value.string_value->data);
	cel_value_destroy(&result);
}

/* ========== type() 函数测试 ========== */

void test_type_int(void)
{
	cel_value_t result = eval_expression("type(42)");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_STRING, result.type);
	TEST_ASSERT_EQUAL_STRING("int", result.value.string_value->data);
	cel_value_destroy(&result);
}

void test_type_string(void)
{
	cel_value_t result = eval_expression("type(\"hello\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_STRING, result.type);
	TEST_ASSERT_EQUAL_STRING("string", result.value.string_value->data);
	cel_value_destroy(&result);
}

void test_type_bool(void)
{
	cel_value_t result = eval_expression("type(true)");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_STRING, result.type);
	TEST_ASSERT_EQUAL_STRING("bool", result.value.string_value->data);
	cel_value_destroy(&result);
}

void test_type_list(void)
{
	cel_value_t result = eval_expression("type([1, 2, 3])");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_STRING, result.type);
	TEST_ASSERT_EQUAL_STRING("list", result.value.string_value->data);
	cel_value_destroy(&result);
}

void test_type_map(void)
{
	cel_value_t result = eval_expression("type({\"a\": 1})");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_STRING, result.type);
	TEST_ASSERT_EQUAL_STRING("map", result.value.string_value->data);
	cel_value_destroy(&result);
}

/* ========== 方法调用语法测试 ========== */

void test_method_size_string(void)
{
	/* 测试 str.size() 语法 */
	cel_value_t result = eval_expression("\"hello\".size()");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(5, result.value.int_value);
}

void test_method_size_list(void)
{
	/* 测试 list.size() 语法 */
	cel_value_t result = eval_expression("[1, 2, 3].size()");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(3, result.value.int_value);
	cel_value_destroy(&result);
}

void test_method_contains_string(void)
{
	/* 测试 str.contains(substr) 语法 */
	cel_value_t result = eval_expression("\"hello world\".contains(\"world\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);
}

void test_method_contains_string_false(void)
{
	cel_value_t result = eval_expression("\"hello\".contains(\"xyz\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_FALSE(result.value.bool_value);
}

void test_method_startsWith(void)
{
	cel_value_t result = eval_expression("\"hello world\".startsWith(\"hello\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);
}

void test_method_startsWith_false(void)
{
	cel_value_t result = eval_expression("\"hello\".startsWith(\"world\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_FALSE(result.value.bool_value);
}

void test_method_endsWith(void)
{
	cel_value_t result = eval_expression("\"hello world\".endsWith(\"world\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);
}

void test_method_endsWith_false(void)
{
	cel_value_t result = eval_expression("\"hello world\".endsWith(\"hello\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_FALSE(result.value.bool_value);
}

void test_method_contains_list(void)
{
	cel_value_t result = eval_expression("[1, 2, 3].contains(2)");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);
	cel_value_destroy(&result);
}

void test_method_contains_list_false(void)
{
	cel_value_t result = eval_expression("[1, 2, 3].contains(5)");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_FALSE(result.value.bool_value);
	cel_value_destroy(&result);
}

/* ========== Main 测试运行器 ========== */

int main(void)
{
	UNITY_BEGIN();

	/* size() 测试 */
	RUN_TEST(test_size_string);
	RUN_TEST(test_size_empty_string);
	RUN_TEST(test_size_list);
	RUN_TEST(test_size_empty_list);
	RUN_TEST(test_size_map);

	/* int() 测试 */
	RUN_TEST(test_int_from_double);
	RUN_TEST(test_int_from_string);
	RUN_TEST(test_int_from_negative_string);

	/* double() 测试 */
	RUN_TEST(test_double_from_int);
	RUN_TEST(test_double_from_string);

	/* string() 测试 */
	RUN_TEST(test_string_from_int);
	RUN_TEST(test_string_from_bool_true);
	RUN_TEST(test_string_from_bool_false);

	/* type() 测试 */
	RUN_TEST(test_type_int);
	RUN_TEST(test_type_string);
	RUN_TEST(test_type_bool);
	RUN_TEST(test_type_list);
	RUN_TEST(test_type_map);

	/* 方法调用语法测试 */
	RUN_TEST(test_method_size_string);
	RUN_TEST(test_method_size_list);
	RUN_TEST(test_method_contains_string);
	RUN_TEST(test_method_contains_string_false);
	RUN_TEST(test_method_startsWith);
	RUN_TEST(test_method_startsWith_false);
	RUN_TEST(test_method_endsWith);
	RUN_TEST(test_method_endsWith_false);
	RUN_TEST(test_method_contains_list);
	RUN_TEST(test_method_contains_list_false);

	return UNITY_END();
}
