/**
 * @file test_time.c
 * @brief CEL 时间类型方法单元测试
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

/* ========== timestamp() 函数测试 ========== */

void test_timestamp_from_int(void)
{
	/* timestamp(0) = Unix epoch: 1970-01-01T00:00:00Z */
	cel_value_t result = eval_expression("timestamp(0)");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_TIMESTAMP, result.type);
	TEST_ASSERT_EQUAL_INT64(0, result.value.timestamp_value.seconds);
}

void test_timestamp_from_string(void)
{
	/* 解析 RFC3339 格式 */
	cel_value_t result = eval_expression("timestamp(\"2021-08-15T10:30:00Z\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_TIMESTAMP, result.type);
	/* 验证时间戳不为 0 (具体值取决于时区) */
	TEST_ASSERT_NOT_EQUAL(0, result.value.timestamp_value.seconds);
}

/* ========== duration() 函数测试 ========== */

void test_duration_hours(void)
{
	cel_value_t result = eval_expression("duration(\"2h\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_DURATION, result.type);
	TEST_ASSERT_EQUAL_INT64(7200, result.value.duration_value.seconds);
}

void test_duration_minutes(void)
{
	cel_value_t result = eval_expression("duration(\"30m\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_DURATION, result.type);
	TEST_ASSERT_EQUAL_INT64(1800, result.value.duration_value.seconds);
}

void test_duration_seconds(void)
{
	cel_value_t result = eval_expression("duration(\"45s\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_DURATION, result.type);
	TEST_ASSERT_EQUAL_INT64(45, result.value.duration_value.seconds);
}

void test_duration_combined(void)
{
	/* 1h30m45s = 3600 + 1800 + 45 = 5445 */
	cel_value_t result = eval_expression("duration(\"1h30m45s\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_DURATION, result.type);
	TEST_ASSERT_EQUAL_INT64(5445, result.value.duration_value.seconds);
}

void test_duration_negative(void)
{
	cel_value_t result = eval_expression("duration(\"-1h\")");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_DURATION, result.type);
	TEST_ASSERT_EQUAL_INT64(-3600, result.value.duration_value.seconds);
}

/* ========== timestamp 方法测试 ========== */

void test_timestamp_getFullYear(void)
{
	/* 添加变量 ts = timestamp(0) 即 1970-01-01 */
	cel_value_t ts = cel_value_timestamp(0, 0, 0);
	cel_context_add_variable(ctx, "ts", &ts);

	cel_value_t result = eval_expression("ts.getFullYear()");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(1970, result.value.int_value);
}

void test_timestamp_getMonth(void)
{
	/* ts = 1970-01-01, month = 0 (January) */
	cel_value_t ts = cel_value_timestamp(0, 0, 0);
	cel_context_add_variable(ctx, "ts", &ts);

	cel_value_t result = eval_expression("ts.getMonth()");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(0, result.value.int_value);
}

void test_timestamp_getDayOfMonth(void)
{
	/* ts = 1970-01-01, day = 1 */
	cel_value_t ts = cel_value_timestamp(0, 0, 0);
	cel_context_add_variable(ctx, "ts", &ts);

	cel_value_t result = eval_expression("ts.getDayOfMonth()");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(1, result.value.int_value);
}

void test_timestamp_getDayOfWeek(void)
{
	/* 1970-01-01 是周四 (4) */
	cel_value_t ts = cel_value_timestamp(0, 0, 0);
	cel_context_add_variable(ctx, "ts", &ts);

	cel_value_t result = eval_expression("ts.getDayOfWeek()");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(4, result.value.int_value);  /* Thursday */
}

void test_timestamp_getDayOfYear(void)
{
	/* 1970-01-01, dayOfYear = 0 */
	cel_value_t ts = cel_value_timestamp(0, 0, 0);
	cel_context_add_variable(ctx, "ts", &ts);

	cel_value_t result = eval_expression("ts.getDayOfYear()");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(0, result.value.int_value);
}

void test_timestamp_getHours(void)
{
	/* ts = 1970-01-01 00:00:00 UTC */
	cel_value_t ts = cel_value_timestamp(0, 0, 0);
	cel_context_add_variable(ctx, "ts", &ts);

	cel_value_t result = eval_expression("ts.getHours()");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(0, result.value.int_value);
}

void test_timestamp_getMinutes(void)
{
	cel_value_t ts = cel_value_timestamp(0, 0, 0);
	cel_context_add_variable(ctx, "ts", &ts);

	cel_value_t result = eval_expression("ts.getMinutes()");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(0, result.value.int_value);
}

void test_timestamp_getSeconds(void)
{
	cel_value_t ts = cel_value_timestamp(0, 0, 0);
	cel_context_add_variable(ctx, "ts", &ts);

	cel_value_t result = eval_expression("ts.getSeconds()");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(0, result.value.int_value);
}

void test_timestamp_getMilliseconds(void)
{
	/* 带毫秒的时间戳 */
	cel_value_t ts = cel_value_timestamp(0, 123000000, 0);  /* 123 ms */
	cel_context_add_variable(ctx, "ts", &ts);

	cel_value_t result = eval_expression("ts.getMilliseconds()");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(123, result.value.int_value);
}

/* ========== duration 方法测试 ========== */

void test_duration_getHours(void)
{
	/* 添加变量 dur = 5445 seconds (1h30m45s) */
	cel_value_t dur = cel_value_duration(5445, 0);
	cel_context_add_variable(ctx, "dur", &dur);

	cel_value_t result = eval_expression("dur.getHours()");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(1, result.value.int_value);  /* 5445 / 3600 = 1 */
}

void test_duration_getMinutes(void)
{
	cel_value_t dur = cel_value_duration(5445, 0);
	cel_context_add_variable(ctx, "dur", &dur);

	cel_value_t result = eval_expression("dur.getMinutes()");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(90, result.value.int_value);  /* 5445 / 60 = 90 */
}

void test_duration_getSeconds(void)
{
	cel_value_t dur = cel_value_duration(5445, 0);
	cel_context_add_variable(ctx, "dur", &dur);

	cel_value_t result = eval_expression("dur.getSeconds()");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(5445, result.value.int_value);
}

void test_duration_getMilliseconds(void)
{
	/* 1 秒 500 毫秒 */
	cel_value_t dur = cel_value_duration(1, 500000000);  /* 1.5 seconds */
	cel_context_add_variable(ctx, "dur", &dur);

	cel_value_t result = eval_expression("dur.getMilliseconds()");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(1500, result.value.int_value);
}

/* ========== 链式调用测试 ========== */

void test_timestamp_chain(void)
{
	/* timestamp(0).getFullYear() */
	cel_value_t result = eval_expression("timestamp(0).getFullYear()");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(1970, result.value.int_value);
}

void test_duration_chain(void)
{
	/* duration("2h").getHours() */
	cel_value_t result = eval_expression("duration(\"2h\").getHours()");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, result.type);
	TEST_ASSERT_EQUAL_INT64(2, result.value.int_value);
}

/* ========== Main 测试运行器 ========== */

int main(void)
{
	UNITY_BEGIN();

	/* timestamp() 函数测试 */
	RUN_TEST(test_timestamp_from_int);
	RUN_TEST(test_timestamp_from_string);

	/* duration() 函数测试 */
	RUN_TEST(test_duration_hours);
	RUN_TEST(test_duration_minutes);
	RUN_TEST(test_duration_seconds);
	RUN_TEST(test_duration_combined);
	RUN_TEST(test_duration_negative);

	/* timestamp 方法测试 */
	RUN_TEST(test_timestamp_getFullYear);
	RUN_TEST(test_timestamp_getMonth);
	RUN_TEST(test_timestamp_getDayOfMonth);
	RUN_TEST(test_timestamp_getDayOfWeek);
	RUN_TEST(test_timestamp_getDayOfYear);
	RUN_TEST(test_timestamp_getHours);
	RUN_TEST(test_timestamp_getMinutes);
	RUN_TEST(test_timestamp_getSeconds);
	RUN_TEST(test_timestamp_getMilliseconds);

	/* duration 方法测试 */
	RUN_TEST(test_duration_getHours);
	RUN_TEST(test_duration_getMinutes);
	RUN_TEST(test_duration_getSeconds);
	RUN_TEST(test_duration_getMilliseconds);

	/* 链式调用测试 */
	RUN_TEST(test_timestamp_chain);
	RUN_TEST(test_duration_chain);

	return UNITY_END();
}
