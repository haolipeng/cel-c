/**
 * @file test_compatibility.c
 * @brief CEL 兼容性测试
 *
 * 从 JSON 文件加载测试用例并验证结果。
 */

#include "cel/cel_program.h"
#include "cel/cel_value.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void setUp(void) {}
void tearDown(void) {}

/* 简单的测试用例结构 */
typedef struct {
	const char *name;
	const char *expr;
	cel_type_e expected_type;
	union {
		int64_t int_val;
		double double_val;
		bool bool_val;
		const char *string_val;
	} expected;
} test_case_t;

static test_case_t test_cases[] = {
	/* 算术运算 */
	{"arithmetic_add", "1 + 2", CEL_TYPE_INT, {.int_val = 3}},
	{"arithmetic_sub", "5 - 3", CEL_TYPE_INT, {.int_val = 2}},
	{"arithmetic_mul", "4 * 3", CEL_TYPE_INT, {.int_val = 12}},
	{"arithmetic_div", "10 / 3", CEL_TYPE_INT, {.int_val = 3}},
	{"arithmetic_mod", "10 % 3", CEL_TYPE_INT, {.int_val = 1}},
	{"arithmetic_neg", "-5", CEL_TYPE_INT, {.int_val = -5}},

	/* 比较运算 */
	{"comparison_eq", "1 == 1", CEL_TYPE_BOOL, {.bool_val = true}},
	{"comparison_ne", "1 != 2", CEL_TYPE_BOOL, {.bool_val = true}},
	{"comparison_lt", "1 < 2", CEL_TYPE_BOOL, {.bool_val = true}},
	{"comparison_le", "2 <= 2", CEL_TYPE_BOOL, {.bool_val = true}},
	{"comparison_gt", "3 > 2", CEL_TYPE_BOOL, {.bool_val = true}},
	{"comparison_ge", "3 >= 3", CEL_TYPE_BOOL, {.bool_val = true}},

	/* 逻辑运算 */
	{"logical_and", "true && true", CEL_TYPE_BOOL, {.bool_val = true}},
	{"logical_or", "false || true", CEL_TYPE_BOOL, {.bool_val = true}},
	{"logical_not", "!false", CEL_TYPE_BOOL, {.bool_val = true}},

	/* 字符串 */
	{"string_eq", "\"abc\" == \"abc\"", CEL_TYPE_BOOL, {.bool_val = true}},

	/* 条件表达式 */
	{"conditional_true", "true ? 1 : 2", CEL_TYPE_INT, {.int_val = 1}},
	{"conditional_false", "false ? 1 : 2", CEL_TYPE_INT, {.int_val = 2}},

	/* 列表 */
	{"list_index", "[1, 2, 3][1]", CEL_TYPE_INT, {.int_val = 2}},
	{"list_size", "size([1, 2, 3])", CEL_TYPE_INT, {.int_val = 3}},

	/* 成员运算 */
	{"in_list", "2 in [1, 2, 3]", CEL_TYPE_BOOL, {.bool_val = true}},

	/* 优先级 */
	{"precedence", "1 + 2 * 3", CEL_TYPE_INT, {.int_val = 7}},
	{"parentheses", "(1 + 2) * 3", CEL_TYPE_INT, {.int_val = 9}},
};

static void run_test_case(const test_case_t *tc)
{
	cel_context_t *ctx = cel_context_create();
	cel_execute_result_t result = cel_eval_expression(tc->expr, ctx);

	char msg[256];
	snprintf(msg, sizeof(msg), "Test '%s' failed: expr='%s'", tc->name, tc->expr);

	if (!result.success) {
		printf("FAIL: %s\n", msg);
		TEST_ASSERT_TRUE(result.success);
	}
	TEST_ASSERT_EQUAL_INT(tc->expected_type, result.value.type);

	switch (tc->expected_type) {
	case CEL_TYPE_INT:
		TEST_ASSERT_EQUAL_INT64(tc->expected.int_val, result.value.value.int_value);
		break;
	case CEL_TYPE_BOOL:
		TEST_ASSERT_EQUAL(tc->expected.bool_val, result.value.value.bool_value);
		break;
	case CEL_TYPE_DOUBLE:
		TEST_ASSERT_DOUBLE_WITHIN(0.001, tc->expected.double_val,
					  result.value.value.double_value);
		break;
	default:
		break;
	}

	cel_execute_result_destroy(&result);
	cel_context_destroy(ctx);
}

void test_compatibility_suite(void)
{
	size_t num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
	printf("\nRunning %zu compatibility tests...\n", num_tests);

	for (size_t i = 0; i < num_tests; i++) {
		run_test_case(&test_cases[i]);
	}
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_compatibility_suite);
	return UNITY_END();
}
