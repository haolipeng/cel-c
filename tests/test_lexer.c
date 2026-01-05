/**
 * @file test_lexer.c
 * @brief CEL 词法分析器单元测试
 */

#include "cel/cel_lexer.h"
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

/* ========== 辅助函数 ========== */

static void assert_token(cel_token_t *token, cel_token_type_e expected_type)
{
	TEST_ASSERT_EQUAL_INT(expected_type, token->type);
}

static void assert_int_token(cel_token_t *token, int64_t expected_value)
{
	TEST_ASSERT_EQUAL_INT(CEL_TOKEN_INT, token->type);
	TEST_ASSERT_EQUAL_INT64(expected_value, token->value.int_value);
}

static void assert_uint_token(cel_token_t *token, uint64_t expected_value)
{
	TEST_ASSERT_EQUAL_INT(CEL_TOKEN_UINT, token->type);
	TEST_ASSERT_EQUAL_UINT64(expected_value, token->value.uint_value);
}

static void assert_double_token(cel_token_t *token, double expected_value)
{
	TEST_ASSERT_EQUAL_INT(CEL_TOKEN_DOUBLE, token->type);
	TEST_ASSERT_DOUBLE_WITHIN(0.00001, expected_value,
				  token->value.double_value);
}

static void assert_string_token(cel_token_t *token, const char *expected_str)
{
	TEST_ASSERT_EQUAL_INT(CEL_TOKEN_STRING, token->type);
	size_t expected_len = strlen(expected_str);
	TEST_ASSERT_EQUAL_size_t(expected_len, token->value.str_length);
	TEST_ASSERT_EQUAL_MEMORY(expected_str, token->value.str_value,
				 expected_len);
}

static void assert_identifier_token(cel_token_t *token,
				     const char *expected_name)
{
	TEST_ASSERT_EQUAL_INT(CEL_TOKEN_IDENTIFIER, token->type);
	size_t expected_len = strlen(expected_name);
	TEST_ASSERT_EQUAL_size_t(expected_len, token->value.str_length);
	TEST_ASSERT_EQUAL_MEMORY(expected_name, token->value.str_value,
				 expected_len);
}

/* ========== 整数字面量测试 ========== */

void test_int_decimal(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "123");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_int_token(&token, 123);
}

void test_int_negative(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	/* 负号是单独的 Token */
	cel_lexer_init(&lexer, "-456");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_MINUS);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_int_token(&token, 456);
}

void test_int_hex(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "0x1A");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_int_token(&token, 26);
}

void test_int_hex_uppercase(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "0xFF");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_int_token(&token, 255);
}

void test_uint_literal(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "123u");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_uint_token(&token, 123);
}

void test_uint_hex(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "0xFFu");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_uint_token(&token, 255);
}

/* ========== 浮点数字面量测试 ========== */

void test_double_simple(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "3.14");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_double_token(&token, 3.14);
}

void test_double_no_integer_part(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, ".5");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_double_token(&token, 0.5);
}

void test_double_scientific(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "1.23e10");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_double_token(&token, 1.23e10);
}

void test_double_scientific_negative_exp(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "5e-3");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_double_token(&token, 5e-3);
}

/* ========== 字符串字面量测试 ========== */

void test_string_simple(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "\"hello\"");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_string_token(&token, "hello");
}

void test_string_empty(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "\"\"");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_string_token(&token, "");
}

void test_string_with_escape(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "\"hello\\nworld\"");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	TEST_ASSERT_EQUAL_INT(CEL_TOKEN_STRING, token.type);
	/* 转义序列保持原样 (解析阶段处理) */
	TEST_ASSERT_EQUAL_size_t(12, token.value.str_length);
}

void test_string_unterminated(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "\"hello");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	TEST_ASSERT_EQUAL_INT(CEL_TOKEN_ERROR, token.type);
}

/* ========== 字节字面量测试 ========== */

void test_bytes_simple(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "b\"hello\"");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	TEST_ASSERT_EQUAL_INT(CEL_TOKEN_BYTES, token.type);
	TEST_ASSERT_EQUAL_size_t(5, token.value.str_length);
	TEST_ASSERT_EQUAL_MEMORY("hello", token.value.str_value, 5);
}

void test_bytes_empty(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "b\"\"");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	TEST_ASSERT_EQUAL_INT(CEL_TOKEN_BYTES, token.type);
	TEST_ASSERT_EQUAL_size_t(0, token.value.str_length);
}

/* ========== 布尔值和 null 测试 ========== */

void test_true_keyword(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "true");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_TRUE);
}

void test_false_keyword(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "false");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_FALSE);
}

void test_null_keyword(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "null");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_NULL);
}

/* ========== 标识符测试 ========== */

void test_identifier_simple(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "foo");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_identifier_token(&token, "foo");
}

void test_identifier_with_underscore(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "foo_bar");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_identifier_token(&token, "foo_bar");
}

void test_identifier_with_digits(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "var123");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_identifier_token(&token, "var123");
}

/* ========== 运算符测试 ========== */

void test_arithmetic_operators(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "+ - * / %");

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_PLUS);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_MINUS);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_STAR);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_SLASH);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_PERCENT);
}

void test_comparison_operators(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "== != < <= > >=");

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_EQUAL_EQUAL);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_BANG_EQUAL);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_LESS);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_LESS_EQUAL);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_GREATER);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_GREATER_EQUAL);
}

void test_logical_operators(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "&& || !");

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_AND_AND);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_OR_OR);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_BANG);
}

void test_ternary_operator(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "? :");

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_QUESTION);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_COLON);
}

void test_field_access_operators(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, ". .?");

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_DOT);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_DOT_QUESTION);
}

void test_bracket_operators(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "[ ] [?");

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_LBRACKET);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_RBRACKET);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_LBRACKET_QUESTION);
}

void test_parentheses_and_braces(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "( ) { } ,");

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_LPAREN);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_RPAREN);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_LBRACE);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_RBRACE);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_COMMA);
}

/* ========== 空白字符和注释测试 ========== */

void test_whitespace_skipping(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "  \t\n  123  ");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_int_token(&token, 123);
}

void test_line_comment(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "123 // this is a comment\n456");

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_int_token(&token, 123);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_int_token(&token, 456);
}

void test_comment_at_end(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "123 // comment");

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_int_token(&token, 123);

	TEST_ASSERT_FALSE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_EOF);
}

/* ========== 复杂表达式测试 ========== */

void test_simple_expression(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "1 + 2");

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_int_token(&token, 1);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_PLUS);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_int_token(&token, 2);

	TEST_ASSERT_FALSE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_EOF);
}

void test_field_access_expression(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "obj.field");

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_identifier_token(&token, "obj");

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_DOT);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_identifier_token(&token, "field");
}

void test_function_call_expression(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "func(1, 2)");

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_identifier_token(&token, "func");

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_LPAREN);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_int_token(&token, 1);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_COMMA);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_int_token(&token, 2);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_RPAREN);
}

void test_ternary_expression(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "x > 0 ? 1 : -1");

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_identifier_token(&token, "x");

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_GREATER);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_int_token(&token, 0);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_QUESTION);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_int_token(&token, 1);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_COLON);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_MINUS);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_int_token(&token, 1);
}

/* ========== 错误处理测试 ========== */

void test_error_unexpected_character(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "@");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	TEST_ASSERT_EQUAL_INT(CEL_TOKEN_ERROR, token.type);
}

void test_error_single_ampersand(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "&");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	TEST_ASSERT_EQUAL_INT(CEL_TOKEN_ERROR, token.type);
}

void test_error_single_pipe(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "|");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	TEST_ASSERT_EQUAL_INT(CEL_TOKEN_ERROR, token.type);
}

/* ========== 位置跟踪测试 ========== */

void test_location_tracking_simple(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "123");
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));

	TEST_ASSERT_EQUAL_size_t(1, token.loc.line);
	TEST_ASSERT_EQUAL_size_t(1, token.loc.column);
	TEST_ASSERT_EQUAL_size_t(0, token.loc.offset);
	TEST_ASSERT_EQUAL_size_t(3, token.loc.length);
}

void test_location_tracking_multiline(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "123\n456");

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	TEST_ASSERT_EQUAL_size_t(1, token.loc.line);

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	TEST_ASSERT_EQUAL_size_t(2, token.loc.line);
	TEST_ASSERT_EQUAL_size_t(1, token.loc.column);
}

/* ========== EOF 测试 ========== */

void test_eof(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "");
	TEST_ASSERT_FALSE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_EOF);
}

void test_eof_after_tokens(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "123");

	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_int_token(&token, 123);

	TEST_ASSERT_FALSE(cel_lexer_next_token(&lexer, &token));
	assert_token(&token, CEL_TOKEN_EOF);
}

/* ========== Peek 测试 ========== */

void test_peek_token(void)
{
	cel_lexer_t lexer;
	cel_token_t token;

	cel_lexer_init(&lexer, "123 456");

	/* Peek 第一个 Token */
	TEST_ASSERT_TRUE(cel_lexer_peek_token(&lexer, &token));
	assert_int_token(&token, 123);

	/* 再次 Peek 应该返回相同的 Token */
	TEST_ASSERT_TRUE(cel_lexer_peek_token(&lexer, &token));
	assert_int_token(&token, 123);

	/* 正常扫描 */
	TEST_ASSERT_TRUE(cel_lexer_next_token(&lexer, &token));
	assert_int_token(&token, 123);

	/* Peek 第二个 Token */
	TEST_ASSERT_TRUE(cel_lexer_peek_token(&lexer, &token));
	assert_int_token(&token, 456);
}

/* ========== Unity 主函数 ========== */

int main(void)
{
	UNITY_BEGIN();

	/* 整数字面量测试 */
	RUN_TEST(test_int_decimal);
	RUN_TEST(test_int_negative);
	RUN_TEST(test_int_hex);
	RUN_TEST(test_int_hex_uppercase);
	RUN_TEST(test_uint_literal);
	RUN_TEST(test_uint_hex);

	/* 浮点数字面量测试 */
	RUN_TEST(test_double_simple);
	RUN_TEST(test_double_no_integer_part);
	RUN_TEST(test_double_scientific);
	RUN_TEST(test_double_scientific_negative_exp);

	/* 字符串字面量测试 */
	RUN_TEST(test_string_simple);
	RUN_TEST(test_string_empty);
	RUN_TEST(test_string_with_escape);
	RUN_TEST(test_string_unterminated);

	/* 字节字面量测试 */
	RUN_TEST(test_bytes_simple);
	RUN_TEST(test_bytes_empty);

	/* 布尔值和 null 测试 */
	RUN_TEST(test_true_keyword);
	RUN_TEST(test_false_keyword);
	RUN_TEST(test_null_keyword);

	/* 标识符测试 */
	RUN_TEST(test_identifier_simple);
	RUN_TEST(test_identifier_with_underscore);
	RUN_TEST(test_identifier_with_digits);

	/* 运算符测试 */
	RUN_TEST(test_arithmetic_operators);
	RUN_TEST(test_comparison_operators);
	RUN_TEST(test_logical_operators);
	RUN_TEST(test_ternary_operator);
	RUN_TEST(test_field_access_operators);
	RUN_TEST(test_bracket_operators);
	RUN_TEST(test_parentheses_and_braces);

	/* 空白字符和注释测试 */
	RUN_TEST(test_whitespace_skipping);
	RUN_TEST(test_line_comment);
	RUN_TEST(test_comment_at_end);

	/* 复杂表达式测试 */
	RUN_TEST(test_simple_expression);
	RUN_TEST(test_field_access_expression);
	RUN_TEST(test_function_call_expression);
	RUN_TEST(test_ternary_expression);

	/* 错误处理测试 */
	RUN_TEST(test_error_unexpected_character);
	RUN_TEST(test_error_single_ampersand);
	RUN_TEST(test_error_single_pipe);

	/* 位置跟踪测试 */
	RUN_TEST(test_location_tracking_simple);
	RUN_TEST(test_location_tracking_multiline);

	/* EOF 测试 */
	RUN_TEST(test_eof);
	RUN_TEST(test_eof_after_tokens);

	/* Peek 测试 */
	RUN_TEST(test_peek_token);

	return UNITY_END();
}
