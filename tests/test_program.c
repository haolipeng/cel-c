/**
 * @file test_program.c
 * @brief CEL Program API 单元测试
 */

#include "cel/cel_program.h"
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

/* ========== cel_compile 测试 ========== */

void test_compile_simple(void)
{
	cel_compile_result_t result = cel_compile("1 + 2");

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.program);
	TEST_ASSERT_NOT_NULL(result.program->ast);
	TEST_ASSERT_NOT_NULL(result.program->source);
	TEST_ASSERT_EQUAL_STRING("1 + 2", result.program->source);

	cel_compile_result_destroy(&result);
}

void test_compile_syntax_error(void)
{
	cel_compile_result_t result = cel_compile("1 + + 2");

	TEST_ASSERT_TRUE(result.has_errors);
	TEST_ASSERT_NULL(result.program);
	TEST_ASSERT_GREATER_THAN(0, result.error_count);

	cel_compile_result_destroy(&result);
}

void test_compile_null_source(void)
{
	cel_compile_result_t result = cel_compile(NULL);

	TEST_ASSERT_TRUE(result.has_errors);
	TEST_ASSERT_NULL(result.program);

	cel_compile_result_destroy(&result);
}

/* ========== cel_execute 测试 ========== */

void test_execute_simple(void)
{
	cel_compile_result_t compile = cel_compile("1 + 2");
	TEST_ASSERT_FALSE(compile.has_errors);

	cel_execute_result_t result = cel_execute(compile.program, ctx);

	TEST_ASSERT_TRUE(result.success);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.value.type);
	TEST_ASSERT_EQUAL_INT64(3, result.value.value.int_value);

	cel_execute_result_destroy(&result);
	cel_compile_result_destroy(&compile);
}

void test_execute_with_variable(void)
{
	/* 添加变量到上下文 */
	cel_value_t x = cel_value_int(10);
	cel_context_add_variable(ctx, "x", &x);

	cel_compile_result_t compile = cel_compile("x * 2");
	TEST_ASSERT_FALSE(compile.has_errors);

	cel_execute_result_t result = cel_execute(compile.program, ctx);

	TEST_ASSERT_TRUE(result.success);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.value.type);
	TEST_ASSERT_EQUAL_INT64(20, result.value.value.int_value);

	cel_execute_result_destroy(&result);
	cel_compile_result_destroy(&compile);
}

void test_execute_null_program(void)
{
	cel_execute_result_t result = cel_execute(NULL, ctx);

	TEST_ASSERT_FALSE(result.success);
	TEST_ASSERT_NOT_NULL(result.error);

	cel_execute_result_destroy(&result);
}

void test_execute_null_context(void)
{
	cel_compile_result_t compile = cel_compile("1 + 2");
	TEST_ASSERT_FALSE(compile.has_errors);

	cel_execute_result_t result = cel_execute(compile.program, NULL);

	TEST_ASSERT_FALSE(result.success);
	TEST_ASSERT_NOT_NULL(result.error);

	cel_execute_result_destroy(&result);
	cel_compile_result_destroy(&compile);
}

/* ========== cel_eval_expression 测试 ========== */

void test_eval_expression_simple(void)
{
	cel_execute_result_t result = cel_eval_expression("42", ctx);

	TEST_ASSERT_TRUE(result.success);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.value.type);
	TEST_ASSERT_EQUAL_INT64(42, result.value.value.int_value);

	cel_execute_result_destroy(&result);
}

void test_eval_expression_with_variable(void)
{
	cel_value_t age = cel_value_int(25);
	cel_context_add_variable(ctx, "age", &age);

	cel_execute_result_t result = cel_eval_expression("age >= 18", ctx);

	TEST_ASSERT_TRUE(result.success);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.value.type);
	TEST_ASSERT_TRUE(result.value.value.bool_value);

	cel_execute_result_destroy(&result);
}

void test_eval_expression_syntax_error(void)
{
	cel_execute_result_t result = cel_eval_expression("1 + + 2", ctx);

	TEST_ASSERT_FALSE(result.success);
	TEST_ASSERT_NOT_NULL(result.error);

	cel_execute_result_destroy(&result);
}

/* ========== cel_check_syntax 测试 ========== */

void test_check_syntax_valid(void)
{
	TEST_ASSERT_TRUE(cel_check_syntax("1 + 2"));
	TEST_ASSERT_TRUE(cel_check_syntax("x > 10 && y < 20"));
	TEST_ASSERT_TRUE(cel_check_syntax("[1, 2, 3]"));
	TEST_ASSERT_TRUE(cel_check_syntax("{\"a\": 1}"));
}

void test_check_syntax_invalid(void)
{
	TEST_ASSERT_FALSE(cel_check_syntax("1 + + 2"));
	TEST_ASSERT_FALSE(cel_check_syntax("(1 + 2"));
	TEST_ASSERT_FALSE(cel_check_syntax("[1, 2, "));
	TEST_ASSERT_FALSE(cel_check_syntax(NULL));
}

/* ========== cel_program_get_source 测试 ========== */

void test_program_get_source(void)
{
	cel_compile_result_t compile = cel_compile("x + y");
	TEST_ASSERT_FALSE(compile.has_errors);

	const char *source = cel_program_get_source(compile.program);
	TEST_ASSERT_NOT_NULL(source);
	TEST_ASSERT_EQUAL_STRING("x + y", source);

	cel_compile_result_destroy(&compile);
}

void test_program_get_source_null(void)
{
	const char *source = cel_program_get_source(NULL);
	TEST_ASSERT_NULL(source);
}

/* ========== 复用程序测试 ========== */

void test_program_reuse(void)
{
	/* 编译一次，执行多次 */
	cel_compile_result_t compile = cel_compile("x * x");
	TEST_ASSERT_FALSE(compile.has_errors);

	/* 第一次执行 */
	cel_value_t x1 = cel_value_int(5);
	cel_context_add_variable(ctx, "x", &x1);
	cel_execute_result_t result1 = cel_execute(compile.program, ctx);
	TEST_ASSERT_TRUE(result1.success);
	TEST_ASSERT_EQUAL_INT64(25, result1.value.value.int_value);
	cel_execute_result_destroy(&result1);

	/* 第二次执行（不同的值） */
	cel_context_remove_variable(ctx, "x");
	cel_value_t x2 = cel_value_int(7);
	cel_context_add_variable(ctx, "x", &x2);
	cel_execute_result_t result2 = cel_execute(compile.program, ctx);
	TEST_ASSERT_TRUE(result2.success);
	TEST_ASSERT_EQUAL_INT64(49, result2.value.value.int_value);
	cel_execute_result_destroy(&result2);

	cel_compile_result_destroy(&compile);
}

/* ========== Main 测试运行器 ========== */

int main(void)
{
	UNITY_BEGIN();

	/* cel_compile 测试 */
	RUN_TEST(test_compile_simple);
	RUN_TEST(test_compile_syntax_error);
	RUN_TEST(test_compile_null_source);

	/* cel_execute 测试 */
	RUN_TEST(test_execute_simple);
	RUN_TEST(test_execute_with_variable);
	RUN_TEST(test_execute_null_program);
	RUN_TEST(test_execute_null_context);

	/* cel_eval_expression 测试 */
	RUN_TEST(test_eval_expression_simple);
	RUN_TEST(test_eval_expression_with_variable);
	RUN_TEST(test_eval_expression_syntax_error);

	/* cel_check_syntax 测试 */
	RUN_TEST(test_check_syntax_valid);
	RUN_TEST(test_check_syntax_invalid);

	/* cel_program_get_source 测试 */
	RUN_TEST(test_program_get_source);
	RUN_TEST(test_program_get_source_null);

	/* 复用测试 */
	RUN_TEST(test_program_reuse);

	return UNITY_END();
}
