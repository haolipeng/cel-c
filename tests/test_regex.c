/**
 * @file test_regex.c
 * @brief CEL 正则表达式 matches() 函数单元测试
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

/* ========== matches() 基本测试 ========== */

void test_matches_simple_pattern(void)
{
	/* 简单字符串匹配 */
	cel_value_t result = eval_expression("matches(\"hello\", \"hello\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);
}

void test_matches_no_match(void)
{
	cel_value_t result = eval_expression("matches(\"hello\", \"world\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_FALSE(result.value.bool_value);
}

void test_matches_partial(void)
{
	/* 部分匹配 */
	cel_value_t result = eval_expression("matches(\"hello world\", \"world\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);
}

/* ========== 正则表达式模式测试 ========== */

void test_matches_dot_star(void)
{
	/* .* 匹配任意字符 */
	cel_value_t result = eval_expression("matches(\"hello\", \"h.*o\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);
}

void test_matches_anchor_start(void)
{
	/* ^ 锚定开头 */
	cel_value_t result = eval_expression("matches(\"hello\", \"^hello\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);
}

void test_matches_anchor_start_fail(void)
{
	cel_value_t result = eval_expression("matches(\"say hello\", \"^hello\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_FALSE(result.value.bool_value);
}

void test_matches_anchor_end(void)
{
	/* $ 锚定结尾 */
	cel_value_t result = eval_expression("matches(\"hello\", \"hello$\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);
}

void test_matches_anchor_end_fail(void)
{
	cel_value_t result = eval_expression("matches(\"hello world\", \"hello$\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_FALSE(result.value.bool_value);
}

void test_matches_full_anchor(void)
{
	/* ^...$ 完全匹配 */
	cel_value_t result = eval_expression("matches(\"hello\", \"^hello$\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);
}

void test_matches_character_class(void)
{
	/* [a-z] 字符类 */
	cel_value_t result = eval_expression("matches(\"abc123\", \"[a-z]+\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);
}

void test_matches_digit(void)
{
	/* [0-9] 数字 - 使用字符类代替 \d */
	cel_value_t result = eval_expression("matches(\"abc123\", \"[0-9]+\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);
}

void test_matches_word(void)
{
	/* [a-zA-Z0-9_] 单词字符 - 使用字符类代替 \w */
	cel_value_t result = eval_expression("matches(\"hello_world\", \"[a-zA-Z0-9_]+\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);
}

void test_matches_email_pattern(void)
{
	/* 简单邮箱模式 - 使用字符类 */
	cel_value_t result = eval_expression(
		"matches(\"test@example.com\", \"^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+[.][a-zA-Z]{2,}$\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);
}

void test_matches_email_pattern_fail(void)
{
	cel_value_t result = eval_expression(
		"matches(\"invalid-email\", \"^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+[.][a-zA-Z]{2,}$\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_FALSE(result.value.bool_value);
}

/* ========== 方法调用语法测试 ========== */

void test_matches_method_syntax(void)
{
	/* str.matches(pattern) 语法 */
	cel_value_t result = eval_expression("\"hello\".matches(\"h.*o\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);
}

void test_matches_method_syntax_no_match(void)
{
	cel_value_t result = eval_expression("\"hello\".matches(\"^world$\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_FALSE(result.value.bool_value);
}

/* ========== 变量测试 ========== */

void test_matches_with_variable(void)
{
	cel_value_t str = cel_value_string("hello world");
	cel_context_add_variable(ctx, "text", &str);

	cel_value_t result = eval_expression("text.matches(\"world\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);
}

/* ========== 空字符串测试 ========== */

void test_matches_empty_string(void)
{
	cel_value_t result = eval_expression("matches(\"\", \"^$\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);
}

void test_matches_empty_pattern(void)
{
	/* 空模式匹配任何字符串 */
	cel_value_t result = eval_expression("matches(\"hello\", \"\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);
}

/* ========== Main 测试运行器 ========== */

int main(void)
{
	UNITY_BEGIN();

	/* 基本测试 */
	RUN_TEST(test_matches_simple_pattern);
	RUN_TEST(test_matches_no_match);
	RUN_TEST(test_matches_partial);

	/* 正则表达式模式测试 */
	RUN_TEST(test_matches_dot_star);
	RUN_TEST(test_matches_anchor_start);
	RUN_TEST(test_matches_anchor_start_fail);
	RUN_TEST(test_matches_anchor_end);
	RUN_TEST(test_matches_anchor_end_fail);
	RUN_TEST(test_matches_full_anchor);
	RUN_TEST(test_matches_character_class);
	RUN_TEST(test_matches_digit);
	RUN_TEST(test_matches_word);
	RUN_TEST(test_matches_email_pattern);
	RUN_TEST(test_matches_email_pattern_fail);

	/* 方法调用语法测试 */
	RUN_TEST(test_matches_method_syntax);
	RUN_TEST(test_matches_method_syntax_no_match);

	/* 变量测试 */
	RUN_TEST(test_matches_with_variable);

	/* 空字符串测试 */
	RUN_TEST(test_matches_empty_string);
	RUN_TEST(test_matches_empty_pattern);

	return UNITY_END();
}
