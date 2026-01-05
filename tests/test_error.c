/**
 * @file test_error.c
 * @brief CEL-C 错误处理模块单元测试
 */

#include "cel/cel_error.h"
#include "unity.h"
#include <string.h>

/* ========== Unity 测试框架设置 ========== */

void setUp(void)
{
	/* 每个测试前执行 */
}

void tearDown(void)
{
	/* 每个测试后执行 */
}

/* ========== 错误对象测试 ========== */

void test_error_create_and_destroy(void)
{
	cel_error_t *error =
		cel_error_create(CEL_ERROR_SYNTAX, "Syntax error at line 42");

	TEST_ASSERT_NOT_NULL(error);
	TEST_ASSERT_EQUAL(CEL_ERROR_SYNTAX, error->code);
	TEST_ASSERT_NOT_NULL(error->message);
	TEST_ASSERT_EQUAL_STRING("Syntax error at line 42", error->message);

	cel_error_destroy(error);
}

void test_error_create_with_null_message(void)
{
	cel_error_t *error = cel_error_create(CEL_ERROR_UNKNOWN, NULL);

	TEST_ASSERT_NOT_NULL(error);
	TEST_ASSERT_EQUAL(CEL_ERROR_UNKNOWN, error->code);
	TEST_ASSERT_NULL(error->message);

	cel_error_destroy(error);
}

void test_error_destroy_null(void)
{
	/* 不应该崩溃 */
	cel_error_destroy(NULL);
}

void test_error_code_string(void)
{
	TEST_ASSERT_EQUAL_STRING("CEL_OK", cel_error_code_string(CEL_OK));
	TEST_ASSERT_EQUAL_STRING("CEL_ERROR_SYNTAX",
				  cel_error_code_string(CEL_ERROR_SYNTAX));
	TEST_ASSERT_EQUAL_STRING("CEL_ERROR_PARSE",
				  cel_error_code_string(CEL_ERROR_PARSE));
	TEST_ASSERT_EQUAL_STRING(
		"CEL_ERROR_TYPE_MISMATCH",
		cel_error_code_string(CEL_ERROR_TYPE_MISMATCH));
	TEST_ASSERT_EQUAL_STRING(
		"CEL_ERROR_DIVISION_BY_ZERO",
		cel_error_code_string(CEL_ERROR_DIVISION_BY_ZERO));
	TEST_ASSERT_EQUAL_STRING(
		"CEL_ERROR_NULL_POINTER",
		cel_error_code_string(CEL_ERROR_NULL_POINTER));
	TEST_ASSERT_EQUAL_STRING(
		"CEL_ERROR_OUT_OF_MEMORY",
		cel_error_code_string(CEL_ERROR_OUT_OF_MEMORY));
}

/* ========== Result 类型测试 ========== */

void test_ok_result(void)
{
	int value = 42;
	cel_result_t result = cel_ok_result(&value);

	TEST_ASSERT_TRUE(result.is_ok);
	TEST_ASSERT_EQUAL_PTR(&value, result.value);
	TEST_ASSERT_NULL(result.error);

	/* 销毁 Result (不会释放 value) */
	cel_result_destroy(&result);
}

void test_error_result(void)
{
	cel_error_t *error =
		cel_error_create(CEL_ERROR_INVALID_ARGUMENT, "Invalid input");
	cel_result_t result = cel_error_result(error);

	TEST_ASSERT_FALSE(result.is_ok);
	TEST_ASSERT_NULL(result.value);
	TEST_ASSERT_NOT_NULL(result.error);
	TEST_ASSERT_EQUAL(CEL_ERROR_INVALID_ARGUMENT, result.error->code);
	TEST_ASSERT_EQUAL_STRING("Invalid input", result.error->message);

	/* 销毁 Result (会释放 error) */
	cel_result_destroy(&result);
}

void test_result_destroy_null(void)
{
	/* 不应该崩溃 */
	cel_result_destroy(NULL);
}

/* ========== 错误传播宏测试 ========== */

/* 模拟返回 Result 的函数 */
static cel_result_t divide(int a, int b)
{
	if (b == 0) {
		CEL_RETURN_ERROR(CEL_ERROR_DIVISION_BY_ZERO,
				 "Cannot divide by zero");
	}

	int *result = (int *)malloc(sizeof(int));
	*result = a / b;
	return cel_ok_result(result);
}

void test_macro_cel_return_error(void)
{
	cel_result_t result = divide(10, 0);

	TEST_ASSERT_FALSE(result.is_ok);
	TEST_ASSERT_NULL(result.value);
	TEST_ASSERT_NOT_NULL(result.error);
	TEST_ASSERT_EQUAL(CEL_ERROR_DIVISION_BY_ZERO, result.error->code);
	TEST_ASSERT_EQUAL_STRING("Cannot divide by zero",
				  result.error->message);

	cel_result_destroy(&result);
}

void test_macro_cel_return_error_success(void)
{
	cel_result_t result = divide(10, 2);

	TEST_ASSERT_TRUE(result.is_ok);
	TEST_ASSERT_NOT_NULL(result.value);
	TEST_ASSERT_NULL(result.error);
	TEST_ASSERT_EQUAL(5, *(int *)result.value);

	free(result.value);
}

/* ========== 所有错误码测试 ========== */

void test_all_error_codes(void)
{
	/* 确保所有错误码都有字符串表示 */
	cel_error_code_e codes[] = {
		CEL_OK,
		CEL_ERROR_SYNTAX,
		CEL_ERROR_PARSE,
		CEL_ERROR_TYPE_MISMATCH,
		CEL_ERROR_UNKNOWN_IDENTIFIER,
		CEL_ERROR_DIVISION_BY_ZERO,
		CEL_ERROR_OUT_OF_RANGE,
		CEL_ERROR_OVERFLOW,
		CEL_ERROR_NULL_POINTER,
		CEL_ERROR_INVALID_ARGUMENT,
		CEL_ERROR_OUT_OF_MEMORY,
		CEL_ERROR_NOT_FOUND,
		CEL_ERROR_ALREADY_EXISTS,
		CEL_ERROR_UNSUPPORTED,
		CEL_ERROR_INTERNAL,
		CEL_ERROR_UNKNOWN
	};

	for (size_t i = 0; i < sizeof(codes) / sizeof(codes[0]); i++) {
		const char *str = cel_error_code_string(codes[i]);
		TEST_ASSERT_NOT_NULL(str);
		TEST_ASSERT_TRUE(strlen(str) > 0);
		TEST_ASSERT_NOT_EQUAL_STRING("UNKNOWN_ERROR_CODE", str);
	}
}

/* ========== Unity 主函数 ========== */

int main(void)
{
	UNITY_BEGIN();

	/* 错误对象测试 */
	RUN_TEST(test_error_create_and_destroy);
	RUN_TEST(test_error_create_with_null_message);
	RUN_TEST(test_error_destroy_null);
	RUN_TEST(test_error_code_string);

	/* Result 类型测试 */
	RUN_TEST(test_ok_result);
	RUN_TEST(test_error_result);
	RUN_TEST(test_result_destroy_null);

	/* 宏测试 */
	RUN_TEST(test_macro_cel_return_error);
	RUN_TEST(test_macro_cel_return_error_success);

	/* 完整性测试 */
	RUN_TEST(test_all_error_codes);

	return UNITY_END();
}
