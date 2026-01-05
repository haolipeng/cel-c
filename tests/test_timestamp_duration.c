/**
 * @file test_timestamp_duration.c
 * @brief CEL-C 时间戳和时长类型单元测试
 */

#include "cel/cel_value.h"
#include "test_helpers.h"
#include "unity.h"
#include <stdint.h>

/* ========== Unity 测试框架设置 ========== */

void setUp(void)
{
	/* 每个测试前执行 */
}

void tearDown(void)
{
	/* 每个测试后执行 */
}

/* ========== timestamp 值测试 ========== */

void test_value_timestamp_basic(void)
{
	/* 2025-01-05 12:30:45 UTC */
	cel_value_t value = cel_value_timestamp(1736083845, 123456789, 0);

	TEST_ASSERT_EQUAL(CEL_TYPE_TIMESTAMP, value.type);
	TEST_ASSERT_TRUE(cel_value_is_timestamp(&value));

	cel_timestamp_t ts;
	TEST_ASSERT_TRUE(cel_value_get_timestamp(&value, &ts));
	TEST_ASSERT_EQUAL_INT64(1736083845, ts.seconds);
	TEST_ASSERT_EQUAL_INT32(123456789, ts.nanoseconds);
	TEST_ASSERT_EQUAL_INT16(0, ts.offset_minutes);

	cel_value_destroy(&value);
}

void test_value_timestamp_with_offset(void)
{
	/* 2025-01-05 20:30:45 +08:00 (北京时间) */
	cel_value_t value = cel_value_timestamp(1736083845, 0, 480);

	cel_timestamp_t ts;
	TEST_ASSERT_TRUE(cel_value_get_timestamp(&value, &ts));
	TEST_ASSERT_EQUAL_INT64(1736083845, ts.seconds);
	TEST_ASSERT_EQUAL_INT32(0, ts.nanoseconds);
	TEST_ASSERT_EQUAL_INT16(480, ts.offset_minutes); /* +08:00 = 480 分钟 */

	cel_value_destroy(&value);
}

void test_value_timestamp_negative_offset(void)
{
	/* 时区偏移为负 (例如纽约 -05:00) */
	cel_value_t value = cel_value_timestamp(1736083845, 0, -300);

	cel_timestamp_t ts;
	TEST_ASSERT_TRUE(cel_value_get_timestamp(&value, &ts));
	TEST_ASSERT_EQUAL_INT16(-300, ts.offset_minutes);

	cel_value_destroy(&value);
}

void test_value_timestamp_zero(void)
{
	/* Unix epoch: 1970-01-01 00:00:00 UTC */
	cel_value_t value = cel_value_timestamp(0, 0, 0);

	cel_timestamp_t ts;
	TEST_ASSERT_TRUE(cel_value_get_timestamp(&value, &ts));
	TEST_ASSERT_EQUAL_INT64(0, ts.seconds);
	TEST_ASSERT_EQUAL_INT32(0, ts.nanoseconds);

	cel_value_destroy(&value);
}

void test_value_timestamp_negative(void)
{
	/* 1970 年之前的时间 */
	cel_value_t value = cel_value_timestamp(-86400, 0, 0);

	cel_timestamp_t ts;
	TEST_ASSERT_TRUE(cel_value_get_timestamp(&value, &ts));
	TEST_ASSERT_EQUAL_INT64(-86400, ts.seconds);

	cel_value_destroy(&value);
}

void test_value_timestamp_max_nanoseconds(void)
{
	/* 纳秒部分最大值 */
	cel_value_t value = cel_value_timestamp(1736083845, 999999999, 0);

	cel_timestamp_t ts;
	TEST_ASSERT_TRUE(cel_value_get_timestamp(&value, &ts));
	TEST_ASSERT_EQUAL_INT32(999999999, ts.nanoseconds);

	cel_value_destroy(&value);
}

void test_value_timestamp_convenience_macro(void)
{
	cel_value_t value = CEL_TIMESTAMP(1736083845, 0, 0);

	TEST_ASSERT_TRUE(cel_value_is_timestamp(&value));

	cel_timestamp_t ts;
	TEST_ASSERT_TRUE(cel_value_get_timestamp(&value, &ts));
	TEST_ASSERT_EQUAL_INT64(1736083845, ts.seconds);

	cel_value_destroy(&value);
}

/* ========== duration 值测试 ========== */

void test_value_duration_basic(void)
{
	/* 1 小时 30 分 45 秒 = 5445 秒 */
	cel_value_t value = cel_value_duration(5445, 0);

	TEST_ASSERT_EQUAL(CEL_TYPE_DURATION, value.type);
	TEST_ASSERT_TRUE(cel_value_is_duration(&value));

	cel_duration_t dur;
	TEST_ASSERT_TRUE(cel_value_get_duration(&value, &dur));
	TEST_ASSERT_EQUAL_INT64(5445, dur.seconds);
	TEST_ASSERT_EQUAL_INT32(0, dur.nanoseconds);

	cel_value_destroy(&value);
}

void test_value_duration_with_nanoseconds(void)
{
	/* 1 秒 + 500 毫秒 */
	cel_value_t value = cel_value_duration(1, 500000000);

	cel_duration_t dur;
	TEST_ASSERT_TRUE(cel_value_get_duration(&value, &dur));
	TEST_ASSERT_EQUAL_INT64(1, dur.seconds);
	TEST_ASSERT_EQUAL_INT32(500000000, dur.nanoseconds);

	cel_value_destroy(&value);
}

void test_value_duration_zero(void)
{
	cel_value_t value = cel_value_duration(0, 0);

	cel_duration_t dur;
	TEST_ASSERT_TRUE(cel_value_get_duration(&value, &dur));
	TEST_ASSERT_EQUAL_INT64(0, dur.seconds);
	TEST_ASSERT_EQUAL_INT32(0, dur.nanoseconds);

	cel_value_destroy(&value);
}

void test_value_duration_negative(void)
{
	/* 负时长: -1 小时 */
	cel_value_t value = cel_value_duration(-3600, 0);

	cel_duration_t dur;
	TEST_ASSERT_TRUE(cel_value_get_duration(&value, &dur));
	TEST_ASSERT_EQUAL_INT64(-3600, dur.seconds);

	cel_value_destroy(&value);
}

void test_value_duration_negative_with_nanoseconds(void)
{
	/* 负时长带纳秒: -1.5 秒 */
	cel_value_t value = cel_value_duration(-1, 500000000);

	cel_duration_t dur;
	TEST_ASSERT_TRUE(cel_value_get_duration(&value, &dur));
	TEST_ASSERT_EQUAL_INT64(-1, dur.seconds);
	TEST_ASSERT_EQUAL_INT32(500000000, dur.nanoseconds);

	cel_value_destroy(&value);
}

void test_value_duration_large(void)
{
	/* 大时长: 1000 小时 */
	cel_value_t value = cel_value_duration(3600000, 0);

	cel_duration_t dur;
	TEST_ASSERT_TRUE(cel_value_get_duration(&value, &dur));
	TEST_ASSERT_EQUAL_INT64(3600000, dur.seconds);

	cel_value_destroy(&value);
}

void test_value_duration_convenience_macro(void)
{
	cel_value_t value = CEL_DURATION(60, 0);

	TEST_ASSERT_TRUE(cel_value_is_duration(&value));

	cel_duration_t dur;
	TEST_ASSERT_TRUE(cel_value_get_duration(&value, &dur));
	TEST_ASSERT_EQUAL_INT64(60, dur.seconds);

	cel_value_destroy(&value);
}

/* ========== 类型检查测试 ========== */

void test_timestamp_type_check(void)
{
	cel_value_t value = cel_value_timestamp(1736083845, 0, 0);

	TEST_ASSERT_TRUE(cel_value_is_timestamp(&value));
	TEST_ASSERT_FALSE(cel_value_is_duration(&value));
	TEST_ASSERT_FALSE(cel_value_is_int(&value));
	TEST_ASSERT_EQUAL(CEL_TYPE_TIMESTAMP, cel_value_type(&value));

	cel_value_destroy(&value);
}

void test_duration_type_check(void)
{
	cel_value_t value = cel_value_duration(3600, 0);

	TEST_ASSERT_TRUE(cel_value_is_duration(&value));
	TEST_ASSERT_FALSE(cel_value_is_timestamp(&value));
	TEST_ASSERT_FALSE(cel_value_is_int(&value));
	TEST_ASSERT_EQUAL(CEL_TYPE_DURATION, cel_value_type(&value));

	cel_value_destroy(&value);
}

void test_timestamp_type_name(void)
{
	TEST_ASSERT_EQUAL_STRING("timestamp", cel_type_name(CEL_TYPE_TIMESTAMP));
	TEST_ASSERT_EQUAL_STRING("duration", cel_type_name(CEL_TYPE_DURATION));
}

/* ========== 值相等性测试 ========== */

void test_value_equals_timestamp(void)
{
	cel_value_t a = cel_value_timestamp(1736083845, 123456789, 480);
	cel_value_t b = cel_value_timestamp(1736083845, 123456789, 480);
	cel_value_t c = cel_value_timestamp(1736083845, 0, 480);
	cel_value_t d = cel_value_timestamp(1736083845, 123456789, 0);

	/* 完全相同 */
	TEST_ASSERT_TRUE(cel_value_equals(&a, &b));

	/* 纳秒不同 */
	TEST_ASSERT_FALSE(cel_value_equals(&a, &c));

	/* 时区偏移不同 */
	TEST_ASSERT_FALSE(cel_value_equals(&a, &d));

	cel_value_destroy(&a);
	cel_value_destroy(&b);
	cel_value_destroy(&c);
	cel_value_destroy(&d);
}

void test_value_equals_duration(void)
{
	cel_value_t a = cel_value_duration(3600, 500000000);
	cel_value_t b = cel_value_duration(3600, 500000000);
	cel_value_t c = cel_value_duration(3600, 0);
	cel_value_t d = cel_value_duration(-3600, 500000000);

	/* 完全相同 */
	TEST_ASSERT_TRUE(cel_value_equals(&a, &b));

	/* 纳秒不同 */
	TEST_ASSERT_FALSE(cel_value_equals(&a, &c));

	/* 符号不同 */
	TEST_ASSERT_FALSE(cel_value_equals(&a, &d));

	cel_value_destroy(&a);
	cel_value_destroy(&b);
	cel_value_destroy(&c);
	cel_value_destroy(&d);
}

void test_value_equals_timestamp_duration_different_types(void)
{
	cel_value_t ts = cel_value_timestamp(3600, 0, 0);
	cel_value_t dur = cel_value_duration(3600, 0);

	/* 不同类型永不相等 */
	TEST_ASSERT_FALSE(cel_value_equals(&ts, &dur));

	cel_value_destroy(&ts);
	cel_value_destroy(&dur);
}

/* ========== 边界条件测试 ========== */

void test_timestamp_get_with_null_output(void)
{
	cel_value_t value = cel_value_timestamp(1736083845, 0, 0);

	/* 输出参数可以为 NULL */
	TEST_ASSERT_TRUE(cel_value_get_timestamp(&value, NULL));

	cel_value_destroy(&value);
}

void test_duration_get_with_null_output(void)
{
	cel_value_t value = cel_value_duration(3600, 0);

	/* 输出参数可以为 NULL */
	TEST_ASSERT_TRUE(cel_value_get_duration(&value, NULL));

	cel_value_destroy(&value);
}

void test_timestamp_get_type_mismatch(void)
{
	cel_value_t value = cel_value_duration(3600, 0);

	cel_timestamp_t ts;
	/* duration 不能作为 timestamp 访问 */
	TEST_ASSERT_FALSE(cel_value_get_timestamp(&value, &ts));

	cel_value_destroy(&value);
}

void test_duration_get_type_mismatch(void)
{
	cel_value_t value = cel_value_timestamp(1736083845, 0, 0);

	cel_duration_t dur;
	/* timestamp 不能作为 duration 访问 */
	TEST_ASSERT_FALSE(cel_value_get_duration(&value, &dur));

	cel_value_destroy(&value);
}

/* ========== 销毁测试 ========== */

void test_timestamp_destroy(void)
{
	cel_value_t value = cel_value_timestamp(1736083845, 0, 0);

	/* timestamp 是栈分配，销毁应该安全 */
	cel_value_destroy(&value);

	/* 销毁后类型应为 null */
	TEST_ASSERT_EQUAL(CEL_TYPE_NULL, value.type);
}

void test_duration_destroy(void)
{
	cel_value_t value = cel_value_duration(3600, 0);

	/* duration 是栈分配，销毁应该安全 */
	cel_value_destroy(&value);

	/* 销毁后类型应为 null */
	TEST_ASSERT_EQUAL(CEL_TYPE_NULL, value.type);
}

/* ========== Unity 主函数 ========== */

int main(void)
{
	UNITY_BEGIN();

	/* timestamp 值测试 */
	RUN_TEST(test_value_timestamp_basic);
	RUN_TEST(test_value_timestamp_with_offset);
	RUN_TEST(test_value_timestamp_negative_offset);
	RUN_TEST(test_value_timestamp_zero);
	RUN_TEST(test_value_timestamp_negative);
	RUN_TEST(test_value_timestamp_max_nanoseconds);
	RUN_TEST(test_value_timestamp_convenience_macro);

	/* duration 值测试 */
	RUN_TEST(test_value_duration_basic);
	RUN_TEST(test_value_duration_with_nanoseconds);
	RUN_TEST(test_value_duration_zero);
	RUN_TEST(test_value_duration_negative);
	RUN_TEST(test_value_duration_negative_with_nanoseconds);
	RUN_TEST(test_value_duration_large);
	RUN_TEST(test_value_duration_convenience_macro);

	/* 类型检查测试 */
	RUN_TEST(test_timestamp_type_check);
	RUN_TEST(test_duration_type_check);
	RUN_TEST(test_timestamp_type_name);

	/* 值相等性测试 */
	RUN_TEST(test_value_equals_timestamp);
	RUN_TEST(test_value_equals_duration);
	RUN_TEST(test_value_equals_timestamp_duration_different_types);

	/* 边界条件测试 */
	RUN_TEST(test_timestamp_get_with_null_output);
	RUN_TEST(test_duration_get_with_null_output);
	RUN_TEST(test_timestamp_get_type_mismatch);
	RUN_TEST(test_duration_get_type_mismatch);

	/* 销毁测试 */
	RUN_TEST(test_timestamp_destroy);
	RUN_TEST(test_duration_destroy);

	return UNITY_END();
}
