/**
 * @file test_parser_integration.c
 * @brief CEL Parser 高层 API 集成测试
 *
 * 测试 cel_parse() 高层 API 的完整功能。
 */

#include "cel/cel_parser.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>

/* ========== Unity 回调 ========== */

void setUp(void)
{
	/* 每个测试前调用 */
}

void tearDown(void)
{
	/* 每个测试后调用 */
}

/* ========== 测试用例 ========== */

void test_parse_simple_literal(void)
{
	const char *source = "42";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_EQUAL_UINT(0, result.error_count);
	TEST_ASSERT_NULL(result.errors);
	TEST_ASSERT_NOT_NULL(result.ast);
	TEST_ASSERT_EQUAL(CEL_AST_LITERAL, result.ast->type);

	cel_parse_result_destroy(&result);
}

void test_parse_string_literal(void)
{
	const char *source = "\"hello world\"";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.ast);
	TEST_ASSERT_EQUAL(CEL_AST_LITERAL, result.ast->type);

	cel_parse_result_destroy(&result);
}

void test_parse_simple_arithmetic(void)
{
	const char *source = "1 + 2 * 3";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.ast);
	TEST_ASSERT_EQUAL(CEL_AST_BINARY, result.ast->type);

	cel_parse_result_destroy(&result);
}

void test_parse_comparison(void)
{
	const char *source = "x > 10";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.ast);
	TEST_ASSERT_EQUAL(CEL_AST_BINARY, result.ast->type);

	cel_parse_result_destroy(&result);
}

void test_parse_logical_and(void)
{
	const char *source = "true && false";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.ast);
	TEST_ASSERT_EQUAL(CEL_AST_BINARY, result.ast->type);

	cel_parse_result_destroy(&result);
}

void test_parse_ternary(void)
{
	const char *source = "x > 0 ? \"positive\" : \"non-positive\"";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.ast);
	TEST_ASSERT_EQUAL(CEL_AST_TERNARY, result.ast->type);

	cel_parse_result_destroy(&result);
}

void test_parse_field_access(void)
{
	const char *source = "obj.field";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.ast);
	TEST_ASSERT_EQUAL(CEL_AST_SELECT, result.ast->type);

	cel_parse_result_destroy(&result);
}

void test_parse_index_access(void)
{
	const char *source = "list[0]";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.ast);
	TEST_ASSERT_EQUAL(CEL_AST_INDEX, result.ast->type);

	cel_parse_result_destroy(&result);
}

void test_parse_function_call(void)
{
	const char *source = "size(list)";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.ast);
	TEST_ASSERT_EQUAL(CEL_AST_CALL, result.ast->type);

	cel_parse_result_destroy(&result);
}

void test_parse_list_literal(void)
{
	const char *source = "[1, 2, 3]";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.ast);
	TEST_ASSERT_EQUAL(CEL_AST_LIST, result.ast->type);

	cel_parse_result_destroy(&result);
}

void test_parse_map_literal(void)
{
	const char *source = "{\"key\": \"value\"}";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.ast);
	TEST_ASSERT_EQUAL(CEL_AST_MAP, result.ast->type);

	cel_parse_result_destroy(&result);
}

void test_parse_parentheses(void)
{
	const char *source = "(1 + 2) * 3";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.ast);

	cel_parse_result_destroy(&result);
}

void test_parse_unary_minus(void)
{
	const char *source = "-42";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.ast);
	TEST_ASSERT_EQUAL(CEL_AST_UNARY, result.ast->type);

	cel_parse_result_destroy(&result);
}

void test_parse_unary_not(void)
{
	const char *source = "!true";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.ast);
	TEST_ASSERT_EQUAL(CEL_AST_UNARY, result.ast->type);

	cel_parse_result_destroy(&result);
}

/* ========== 错误处理测试 ========== */

void test_parse_empty_string(void)
{
	const char *source = "";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_TRUE(result.has_errors);
	TEST_ASSERT_GREATER_THAN(0, result.error_count);
	TEST_ASSERT_NOT_NULL(result.errors);

	cel_parse_result_destroy(&result);
}

void test_parse_null_source(void)
{
	cel_parse_result_t result = cel_parse(NULL);

	TEST_ASSERT_TRUE(result.has_errors);
	TEST_ASSERT_EQUAL_UINT(1, result.error_count);
	TEST_ASSERT_NOT_NULL(result.errors);
	TEST_ASSERT_NOT_NULL(result.errors->message);

	cel_parse_result_destroy(&result);
}

void test_parse_invalid_syntax(void)
{
	const char *source = "1 + + 2";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_TRUE(result.has_errors);
	TEST_ASSERT_GREATER_THAN(0, result.error_count);

	cel_parse_result_destroy(&result);
}

void test_parse_unclosed_parenthesis(void)
{
	const char *source = "(1 + 2";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_TRUE(result.has_errors);
	TEST_ASSERT_GREATER_THAN(0, result.error_count);

	cel_parse_result_destroy(&result);
}

void test_parse_unclosed_bracket(void)
{
	const char *source = "[1, 2, 3";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_TRUE(result.has_errors);
	TEST_ASSERT_GREATER_THAN(0, result.error_count);

	cel_parse_result_destroy(&result);
}

/* ========== 错误格式化测试 ========== */

void test_parse_error_format(void)
{
	const char *source = "1 + + 2";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_TRUE(result.has_errors);

	char *formatted = cel_parse_result_format_errors(&result, source);
	TEST_ASSERT_NOT_NULL(formatted);
	TEST_ASSERT_GREATER_THAN(0, strlen(formatted));

	/* 打印格式化的错误 */
	printf("\nFormatted error:\n%s\n", formatted);

	free(formatted);
	cel_parse_result_destroy(&result);
}

/* ========== 复杂表达式测试 ========== */

void test_parse_complex_expression_1(void)
{
	const char *source = "(a + b) * c > d && e || f";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.ast);

	cel_parse_result_destroy(&result);
}

void test_parse_complex_expression_2(void)
{
	/* 方法调用语法: obj.method(args) */
	const char *source = "obj.field[0].method(arg1, arg2)";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.ast);
	TEST_ASSERT_EQUAL(CEL_AST_CALL, result.ast->type);
	/* 验证是方法调用 (有 target) */
	TEST_ASSERT_NOT_NULL(result.ast->as.call.target);

	cel_parse_result_destroy(&result);
}

void test_parse_method_call_simple(void)
{
	const char *source = "str.contains(\"hello\")";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.ast);
	TEST_ASSERT_EQUAL(CEL_AST_CALL, result.ast->type);
	TEST_ASSERT_NOT_NULL(result.ast->as.call.target);
	TEST_ASSERT_EQUAL_UINT(1, result.ast->as.call.arg_count);

	cel_parse_result_destroy(&result);
}

void test_parse_method_call_chained(void)
{
	/* 链式方法调用 */
	const char *source = "list.size()";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.ast);
	TEST_ASSERT_EQUAL(CEL_AST_CALL, result.ast->type);
	TEST_ASSERT_NOT_NULL(result.ast->as.call.target);
	TEST_ASSERT_EQUAL_UINT(0, result.ast->as.call.arg_count);

	cel_parse_result_destroy(&result);
}

void test_parse_complex_expression_3(void)
{
	const char *source = "x ? y : z ? a : b";
	cel_parse_result_t result = cel_parse(source);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.ast);

	cel_parse_result_destroy(&result);
}

void test_parse_with_max_recursion(void)
{
	const char *source = "1 + 2";
	cel_parse_result_t result = cel_parse_with_options(source, 50);

	TEST_ASSERT_FALSE(result.has_errors);
	TEST_ASSERT_NOT_NULL(result.ast);

	cel_parse_result_destroy(&result);
}

/* ========== 源位置工具测试 ========== */

void test_source_location_from_token(void)
{
	cel_lexer_t lexer;
	cel_lexer_init(&lexer, "123");

	cel_token_t token;
	cel_lexer_next_token(&lexer, &token);

	cel_source_location_t loc = cel_source_location_from_token(&token);

	TEST_ASSERT_EQUAL_UINT(1, loc.line);
	TEST_ASSERT_EQUAL_UINT(1, loc.column);
	TEST_ASSERT_EQUAL_UINT(0, loc.offset);
}

void test_source_range_from_token(void)
{
	cel_lexer_t lexer;
	cel_lexer_init(&lexer, "hello");

	cel_token_t token;
	cel_lexer_next_token(&lexer, &token);

	cel_source_range_t range = cel_source_range_from_token(&token);

	TEST_ASSERT_EQUAL_UINT(1, range.start.line);
	TEST_ASSERT_EQUAL_UINT(1, range.start.column);
	TEST_ASSERT_EQUAL_UINT(6, range.end.column);
}

/* ========== 主函数 ========== */

int main(void)
{
	UNITY_BEGIN();

	/* 基本字面量测试 */
	RUN_TEST(test_parse_simple_literal);
	RUN_TEST(test_parse_string_literal);

	/* 运算符测试 */
	RUN_TEST(test_parse_simple_arithmetic);
	RUN_TEST(test_parse_comparison);
	RUN_TEST(test_parse_logical_and);
	RUN_TEST(test_parse_ternary);
	RUN_TEST(test_parse_unary_minus);
	RUN_TEST(test_parse_unary_not);

	/* 访问测试 */
	RUN_TEST(test_parse_field_access);
	RUN_TEST(test_parse_index_access);
	RUN_TEST(test_parse_function_call);

	/* 容器字面量测试 */
	RUN_TEST(test_parse_list_literal);
	RUN_TEST(test_parse_map_literal);

	/* 其他测试 */
	RUN_TEST(test_parse_parentheses);

	/* 错误处理测试 */
	RUN_TEST(test_parse_empty_string);
	RUN_TEST(test_parse_null_source);
	RUN_TEST(test_parse_invalid_syntax);
	RUN_TEST(test_parse_unclosed_parenthesis);
	RUN_TEST(test_parse_unclosed_bracket);

	/* 错误格式化测试 */
	RUN_TEST(test_parse_error_format);

	/* 复杂表达式测试 */
	RUN_TEST(test_parse_complex_expression_1);
	RUN_TEST(test_parse_complex_expression_2);
	RUN_TEST(test_parse_complex_expression_3);

	/* 方法调用测试 */
	RUN_TEST(test_parse_method_call_simple);
	RUN_TEST(test_parse_method_call_chained);
	RUN_TEST(test_parse_with_max_recursion);

	/* 源位置工具测试 */
	RUN_TEST(test_source_location_from_token);
	RUN_TEST(test_source_range_from_token);

	return UNITY_END();
}
