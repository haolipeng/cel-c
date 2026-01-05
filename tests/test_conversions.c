/**
 * @file test_conversions.c
 * @brief CEL-C 类型转换和字符串操作单元测试
 */

#include "cel/cel_value.h"
#include "test_helpers.h"
#include "unity.h"

/* ========== Unity 测试框架设置 ========== */

void setUp(void)
{
	/* 每个测试前执行 */
}

void tearDown(void)
{
	/* 每个测试后执行 */
}

/* ========== 类型转换 int 测试 ========== */

void test_int_to_int(void)
{
	cel_value_t v = cel_value_int(42);
	int64_t result;

	TEST_ASSERT_TRUE(cel_value_to_int(&v, &result));
	TEST_ASSERT_EQUAL_INT64(42, result);
}

void test_uint_to_int(void)
{
	cel_value_t v = cel_value_uint(100);
	int64_t result;

	TEST_ASSERT_TRUE(cel_value_to_int(&v, &result));
	TEST_ASSERT_EQUAL_INT64(100, result);
}

void test_uint_overflow_to_int(void)
{
	cel_value_t v = cel_value_uint(UINT64_MAX);
	int64_t result;

	/* UINT64_MAX 无法转换为 int64 */
	TEST_ASSERT_FALSE(cel_value_to_int(&v, &result));
}

void test_double_to_int(void)
{
	cel_value_t v = cel_value_double(123.456);
	int64_t result;

	TEST_ASSERT_TRUE(cel_value_to_int(&v, &result));
	TEST_ASSERT_EQUAL_INT64(123, result); /* 截断小数 */
}

void test_bool_to_int(void)
{
	cel_value_t v_true = cel_value_bool(true);
	cel_value_t v_false = cel_value_bool(false);
	int64_t result;

	TEST_ASSERT_TRUE(cel_value_to_int(&v_true, &result));
	TEST_ASSERT_EQUAL_INT64(1, result);

	TEST_ASSERT_TRUE(cel_value_to_int(&v_false, &result));
	TEST_ASSERT_EQUAL_INT64(0, result);
}

void test_string_to_int(void)
{
	cel_value_t v1 = cel_value_string("12345");
	cel_value_t v2 = cel_value_string("-999");
	cel_value_t v3 = cel_value_string("not a number");
	int64_t result;

	TEST_ASSERT_TRUE(cel_value_to_int(&v1, &result));
	TEST_ASSERT_EQUAL_INT64(12345, result);

	TEST_ASSERT_TRUE(cel_value_to_int(&v2, &result));
	TEST_ASSERT_EQUAL_INT64(-999, result);

	TEST_ASSERT_FALSE(cel_value_to_int(&v3, &result)); /* 解析失败 */

	cel_value_destroy(&v1);
	cel_value_destroy(&v2);
	cel_value_destroy(&v3);
}

void test_timestamp_to_int(void)
{
	cel_value_t v = cel_value_timestamp(1704441600, 0, 0);
	int64_t result;

	TEST_ASSERT_TRUE(cel_value_to_int(&v, &result));
	TEST_ASSERT_EQUAL_INT64(1704441600, result);
}

void test_duration_to_int(void)
{
	cel_value_t v = cel_value_duration(3665, 0); /* 1h1m5s */
	int64_t result;

	TEST_ASSERT_TRUE(cel_value_to_int(&v, &result));
	TEST_ASSERT_EQUAL_INT64(3665, result);
}

/* ========== 类型转换 uint 测试 ========== */

void test_uint_to_uint(void)
{
	cel_value_t v = cel_value_uint(42);
	uint64_t result;

	TEST_ASSERT_TRUE(cel_value_to_uint(&v, &result));
	TEST_ASSERT_EQUAL_UINT64(42, result);
}

void test_int_to_uint(void)
{
	cel_value_t v_pos = cel_value_int(100);
	cel_value_t v_neg = cel_value_int(-50);
	uint64_t result;

	TEST_ASSERT_TRUE(cel_value_to_uint(&v_pos, &result));
	TEST_ASSERT_EQUAL_UINT64(100, result);

	/* 负数无法转换为 uint */
	TEST_ASSERT_FALSE(cel_value_to_uint(&v_neg, &result));
}

void test_double_to_uint(void)
{
	cel_value_t v_pos = cel_value_double(123.456);
	cel_value_t v_neg = cel_value_double(-10.5);
	uint64_t result;

	TEST_ASSERT_TRUE(cel_value_to_uint(&v_pos, &result));
	TEST_ASSERT_EQUAL_UINT64(123, result);

	/* 负数无法转换为 uint */
	TEST_ASSERT_FALSE(cel_value_to_uint(&v_neg, &result));
}

void test_string_to_uint(void)
{
	cel_value_t v1 = cel_value_string("12345");
	cel_value_t v2 = cel_value_string("-999");
	uint64_t result;

	TEST_ASSERT_TRUE(cel_value_to_uint(&v1, &result));
	TEST_ASSERT_EQUAL_UINT64(12345, result);

	/* 负数字符串无法转换为 uint */
	TEST_ASSERT_FALSE(cel_value_to_uint(&v2, &result));

	cel_value_destroy(&v1);
	cel_value_destroy(&v2);
}

/* ========== 类型转换 double 测试 ========== */

void test_double_to_double(void)
{
	cel_value_t v = cel_value_double(3.14159);
	double result;

	TEST_ASSERT_TRUE(cel_value_to_double(&v, &result));
	TEST_ASSERT_DOUBLE_WITHIN(0.00001, 3.14159, result);
}

void test_int_to_double(void)
{
	cel_value_t v = cel_value_int(42);
	double result;

	TEST_ASSERT_TRUE(cel_value_to_double(&v, &result));
	TEST_ASSERT_DOUBLE_WITHIN(0.00001, 42.0, result);
}

void test_uint_to_double(void)
{
	cel_value_t v = cel_value_uint(100);
	double result;

	TEST_ASSERT_TRUE(cel_value_to_double(&v, &result));
	TEST_ASSERT_DOUBLE_WITHIN(0.00001, 100.0, result);
}

void test_bool_to_double(void)
{
	cel_value_t v_true = cel_value_bool(true);
	cel_value_t v_false = cel_value_bool(false);
	double result;

	TEST_ASSERT_TRUE(cel_value_to_double(&v_true, &result));
	TEST_ASSERT_DOUBLE_WITHIN(0.00001, 1.0, result);

	TEST_ASSERT_TRUE(cel_value_to_double(&v_false, &result));
	TEST_ASSERT_DOUBLE_WITHIN(0.00001, 0.0, result);
}

void test_string_to_double(void)
{
	cel_value_t v1 = cel_value_string("3.14159");
	cel_value_t v2 = cel_value_string("-2.5");
	cel_value_t v3 = cel_value_string("1.23e10");
	double result;

	TEST_ASSERT_TRUE(cel_value_to_double(&v1, &result));
	TEST_ASSERT_DOUBLE_WITHIN(0.00001, 3.14159, result);

	TEST_ASSERT_TRUE(cel_value_to_double(&v2, &result));
	TEST_ASSERT_DOUBLE_WITHIN(0.00001, -2.5, result);

	TEST_ASSERT_TRUE(cel_value_to_double(&v3, &result));
	TEST_ASSERT_DOUBLE_WITHIN(1e5, 1.23e10, result);

	cel_value_destroy(&v1);
	cel_value_destroy(&v2);
	cel_value_destroy(&v3);
}

/* ========== 类型转换 string 测试 ========== */

void test_null_to_string(void)
{
	cel_value_t v = cel_value_null();
	cel_value_t result = cel_value_to_string(&v);

	TEST_ASSERT_TRUE(cel_value_is_string(&result));
	const char *str;
	cel_value_get_string(&result, &str, NULL);
	TEST_ASSERT_EQUAL_STRING("null", str);

	cel_value_destroy(&result);
}

void test_bool_to_string(void)
{
	cel_value_t v_true = cel_value_bool(true);
	cel_value_t v_false = cel_value_bool(false);

	cel_value_t r_true = cel_value_to_string(&v_true);
	cel_value_t r_false = cel_value_to_string(&v_false);

	const char *str;
	cel_value_get_string(&r_true, &str, NULL);
	TEST_ASSERT_EQUAL_STRING("true", str);

	cel_value_get_string(&r_false, &str, NULL);
	TEST_ASSERT_EQUAL_STRING("false", str);

	cel_value_destroy(&r_true);
	cel_value_destroy(&r_false);
}

void test_int_to_string(void)
{
	cel_value_t v = cel_value_int(12345);
	cel_value_t result = cel_value_to_string(&v);

	const char *str;
	cel_value_get_string(&result, &str, NULL);
	TEST_ASSERT_EQUAL_STRING("12345", str);

	cel_value_destroy(&result);
}

void test_uint_to_string(void)
{
	cel_value_t v = cel_value_uint(987654321);
	cel_value_t result = cel_value_to_string(&v);

	const char *str;
	cel_value_get_string(&result, &str, NULL);
	TEST_ASSERT_EQUAL_STRING("987654321", str);

	cel_value_destroy(&result);
}

void test_double_to_string(void)
{
	cel_value_t v = cel_value_double(3.14159);
	cel_value_t result = cel_value_to_string(&v);

	const char *str;
	cel_value_get_string(&result, &str, NULL);
	/* 检查字符串包含 "3.14" */
	TEST_ASSERT_TRUE(strstr(str, "3.14") != NULL);

	cel_value_destroy(&result);
}

void test_string_to_string(void)
{
	cel_value_t v = cel_value_string("hello");
	cel_value_t result = cel_value_to_string(&v);

	const char *str;
	cel_value_get_string(&result, &str, NULL);
	TEST_ASSERT_EQUAL_STRING("hello", str);

	cel_value_destroy(&v);
	cel_value_destroy(&result);
}

void test_bytes_to_string(void)
{
	unsigned char data[] = { 0x48, 0x65, 0x6c, 0x6c, 0x6f }; /* "Hello" */
	cel_value_t v = cel_value_bytes(data, sizeof(data));
	cel_value_t result = cel_value_to_string(&v);

	TEST_ASSERT_TRUE(cel_value_is_string(&result));
	const char *str;
	cel_value_get_string(&result, &str, NULL);
	/* 应该是十六进制: 48656c6c6f */
	TEST_ASSERT_EQUAL_STRING("48656c6c6f", str);

	cel_value_destroy(&v);
	cel_value_destroy(&result);
}

void test_duration_to_string(void)
{
	cel_value_t v = cel_value_duration(3665, 0); /* 1h1m5s */
	cel_value_t result = cel_value_to_string(&v);

	const char *str;
	cel_value_get_string(&result, &str, NULL);
	TEST_ASSERT_EQUAL_STRING("1h1m5s", str);

	cel_value_destroy(&result);
}

/* ========== 类型转换 bytes 测试 ========== */

void test_bytes_to_bytes(void)
{
	unsigned char data[] = { 0x01, 0x02, 0x03 };
	cel_value_t v = cel_value_bytes(data, sizeof(data));
	cel_value_t result = cel_value_to_bytes(&v);

	TEST_ASSERT_TRUE(cel_value_is_bytes(&result));

	const unsigned char *bytes_data;
	size_t len;
	cel_value_get_bytes(&result, &bytes_data, &len);
	TEST_ASSERT_EQUAL(3, len);
	TEST_ASSERT_EQUAL_MEMORY(data, bytes_data, 3);

	cel_value_destroy(&v);
	cel_value_destroy(&result);
}

void test_string_to_bytes(void)
{
	cel_value_t v = cel_value_string("hello");
	cel_value_t result = cel_value_to_bytes(&v);

	TEST_ASSERT_TRUE(cel_value_is_bytes(&result));

	const unsigned char *bytes_data;
	size_t len;
	cel_value_get_bytes(&result, &bytes_data, &len);
	TEST_ASSERT_EQUAL(5, len);
	TEST_ASSERT_EQUAL_MEMORY("hello", bytes_data, 5);

	cel_value_destroy(&v);
	cel_value_destroy(&result);
}

/* ========== 字符串操作测试 ========== */

void test_starts_with_true(void)
{
	cel_value_t str = cel_value_string("hello world");
	cel_value_t prefix = cel_value_string("hello");
	bool result;

	TEST_ASSERT_TRUE(cel_string_starts_with(&str, &prefix, &result));
	TEST_ASSERT_TRUE(result);

	cel_value_destroy(&str);
	cel_value_destroy(&prefix);
}

void test_starts_with_false(void)
{
	cel_value_t str = cel_value_string("hello world");
	cel_value_t prefix = cel_value_string("world");
	bool result;

	TEST_ASSERT_TRUE(cel_string_starts_with(&str, &prefix, &result));
	TEST_ASSERT_FALSE(result);

	cel_value_destroy(&str);
	cel_value_destroy(&prefix);
}

void test_starts_with_empty_prefix(void)
{
	cel_value_t str = cel_value_string("hello");
	cel_value_t prefix = cel_value_string("");
	bool result;

	TEST_ASSERT_TRUE(cel_string_starts_with(&str, &prefix, &result));
	TEST_ASSERT_TRUE(result); /* 空前缀总是匹配 */

	cel_value_destroy(&str);
	cel_value_destroy(&prefix);
}

void test_starts_with_longer_prefix(void)
{
	cel_value_t str = cel_value_string("hi");
	cel_value_t prefix = cel_value_string("hello");
	bool result;

	TEST_ASSERT_TRUE(cel_string_starts_with(&str, &prefix, &result));
	TEST_ASSERT_FALSE(result);

	cel_value_destroy(&str);
	cel_value_destroy(&prefix);
}

void test_ends_with_true(void)
{
	cel_value_t str = cel_value_string("hello world");
	cel_value_t suffix = cel_value_string("world");
	bool result;

	TEST_ASSERT_TRUE(cel_string_ends_with(&str, &suffix, &result));
	TEST_ASSERT_TRUE(result);

	cel_value_destroy(&str);
	cel_value_destroy(&suffix);
}

void test_ends_with_false(void)
{
	cel_value_t str = cel_value_string("hello world");
	cel_value_t suffix = cel_value_string("hello");
	bool result;

	TEST_ASSERT_TRUE(cel_string_ends_with(&str, &suffix, &result));
	TEST_ASSERT_FALSE(result);

	cel_value_destroy(&str);
	cel_value_destroy(&suffix);
}

void test_ends_with_empty_suffix(void)
{
	cel_value_t str = cel_value_string("hello");
	cel_value_t suffix = cel_value_string("");
	bool result;

	TEST_ASSERT_TRUE(cel_string_ends_with(&str, &suffix, &result));
	TEST_ASSERT_TRUE(result); /* 空后缀总是匹配 */

	cel_value_destroy(&str);
	cel_value_destroy(&suffix);
}

void test_contains_true(void)
{
	cel_value_t str = cel_value_string("hello world");
	cel_value_t substr = cel_value_string("lo wo");
	bool result;

	TEST_ASSERT_TRUE(cel_string_contains(&str, &substr, &result));
	TEST_ASSERT_TRUE(result);

	cel_value_destroy(&str);
	cel_value_destroy(&substr);
}

void test_contains_false(void)
{
	cel_value_t str = cel_value_string("hello world");
	cel_value_t substr = cel_value_string("xyz");
	bool result;

	TEST_ASSERT_TRUE(cel_string_contains(&str, &substr, &result));
	TEST_ASSERT_FALSE(result);

	cel_value_destroy(&str);
	cel_value_destroy(&substr);
}

void test_contains_empty_substr(void)
{
	cel_value_t str = cel_value_string("hello");
	cel_value_t substr = cel_value_string("");
	bool result;

	TEST_ASSERT_TRUE(cel_string_contains(&str, &substr, &result));
	TEST_ASSERT_TRUE(result); /* 空子串总是包含 */

	cel_value_destroy(&str);
	cel_value_destroy(&substr);
}

void test_contains_at_beginning(void)
{
	cel_value_t str = cel_value_string("hello world");
	cel_value_t substr = cel_value_string("hello");
	bool result;

	TEST_ASSERT_TRUE(cel_string_contains(&str, &substr, &result));
	TEST_ASSERT_TRUE(result);

	cel_value_destroy(&str);
	cel_value_destroy(&substr);
}

void test_contains_at_end(void)
{
	cel_value_t str = cel_value_string("hello world");
	cel_value_t substr = cel_value_string("world");
	bool result;

	TEST_ASSERT_TRUE(cel_string_contains(&str, &substr, &result));
	TEST_ASSERT_TRUE(result);

	cel_value_destroy(&str);
	cel_value_destroy(&substr);
}

void test_string_concat(void)
{
	cel_value_t a = cel_value_string("hello");
	cel_value_t b = cel_value_string(" world");
	cel_value_t result = cel_string_concat(&a, &b);

	TEST_ASSERT_TRUE(cel_value_is_string(&result));

	const char *str;
	size_t len;
	cel_value_get_string(&result, &str, &len);
	TEST_ASSERT_EQUAL_STRING("hello world", str);
	TEST_ASSERT_EQUAL(11, len);

	cel_value_destroy(&a);
	cel_value_destroy(&b);
	cel_value_destroy(&result);
}

void test_string_concat_empty(void)
{
	cel_value_t a = cel_value_string("hello");
	cel_value_t b = cel_value_string("");
	cel_value_t result = cel_string_concat(&a, &b);

	const char *str;
	cel_value_get_string(&result, &str, NULL);
	TEST_ASSERT_EQUAL_STRING("hello", str);

	cel_value_destroy(&a);
	cel_value_destroy(&b);
	cel_value_destroy(&result);
}

void test_string_length(void)
{
	cel_value_t v1 = cel_value_string("hello");
	cel_value_t v2 = cel_value_string("");
	cel_value_t v3 = cel_value_int(42);

	TEST_ASSERT_EQUAL(5, cel_string_length(&v1));
	TEST_ASSERT_EQUAL(0, cel_string_length(&v2));
	TEST_ASSERT_EQUAL(0, cel_string_length(&v3)); /* 非字符串返回 0 */

	cel_value_destroy(&v1);
	cel_value_destroy(&v2);
}

/* ========== 边界条件测试 ========== */

void test_conversion_null_input(void)
{
	int64_t i_result;
	uint64_t u_result;
	double d_result;

	TEST_ASSERT_FALSE(cel_value_to_int(NULL, &i_result));
	TEST_ASSERT_FALSE(cel_value_to_uint(NULL, &u_result));
	TEST_ASSERT_FALSE(cel_value_to_double(NULL, &d_result));

	cel_value_t v = cel_value_to_string(NULL);
	TEST_ASSERT_TRUE(cel_value_is_null(&v));

	v = cel_value_to_bytes(NULL);
	TEST_ASSERT_TRUE(cel_value_is_null(&v));
}

void test_conversion_null_output(void)
{
	cel_value_t v = cel_value_int(42);

	TEST_ASSERT_FALSE(cel_value_to_int(&v, NULL));
	TEST_ASSERT_FALSE(cel_value_to_uint(&v, NULL));
	TEST_ASSERT_FALSE(cel_value_to_double(&v, NULL));
}

void test_string_ops_type_mismatch(void)
{
	cel_value_t str = cel_value_string("hello");
	cel_value_t num = cel_value_int(42);
	bool result;

	TEST_ASSERT_FALSE(cel_string_starts_with(&num, &str, &result));
	TEST_ASSERT_FALSE(cel_string_ends_with(&str, &num, &result));
	TEST_ASSERT_FALSE(cel_string_contains(&num, &num, &result));

	cel_value_t concat_result = cel_string_concat(&str, &num);
	TEST_ASSERT_TRUE(cel_value_is_null(&concat_result));

	cel_value_destroy(&str);
}

/* ========== Unity 主函数 ========== */

int main(void)
{
	UNITY_BEGIN();

	/* 类型转换 int 测试 */
	RUN_TEST(test_int_to_int);
	RUN_TEST(test_uint_to_int);
	RUN_TEST(test_uint_overflow_to_int);
	RUN_TEST(test_double_to_int);
	RUN_TEST(test_bool_to_int);
	RUN_TEST(test_string_to_int);
	RUN_TEST(test_timestamp_to_int);
	RUN_TEST(test_duration_to_int);

	/* 类型转换 uint 测试 */
	RUN_TEST(test_uint_to_uint);
	RUN_TEST(test_int_to_uint);
	RUN_TEST(test_double_to_uint);
	RUN_TEST(test_string_to_uint);

	/* 类型转换 double 测试 */
	RUN_TEST(test_double_to_double);
	RUN_TEST(test_int_to_double);
	RUN_TEST(test_uint_to_double);
	RUN_TEST(test_bool_to_double);
	RUN_TEST(test_string_to_double);

	/* 类型转换 string 测试 */
	RUN_TEST(test_null_to_string);
	RUN_TEST(test_bool_to_string);
	RUN_TEST(test_int_to_string);
	RUN_TEST(test_uint_to_string);
	RUN_TEST(test_double_to_string);
	RUN_TEST(test_string_to_string);
	RUN_TEST(test_bytes_to_string);
	RUN_TEST(test_duration_to_string);

	/* 类型转换 bytes 测试 */
	RUN_TEST(test_bytes_to_bytes);
	RUN_TEST(test_string_to_bytes);

	/* 字符串操作测试 */
	RUN_TEST(test_starts_with_true);
	RUN_TEST(test_starts_with_false);
	RUN_TEST(test_starts_with_empty_prefix);
	RUN_TEST(test_starts_with_longer_prefix);
	RUN_TEST(test_ends_with_true);
	RUN_TEST(test_ends_with_false);
	RUN_TEST(test_ends_with_empty_suffix);
	RUN_TEST(test_contains_true);
	RUN_TEST(test_contains_false);
	RUN_TEST(test_contains_empty_substr);
	RUN_TEST(test_contains_at_beginning);
	RUN_TEST(test_contains_at_end);
	RUN_TEST(test_string_concat);
	RUN_TEST(test_string_concat_empty);
	RUN_TEST(test_string_length);

	/* 边界条件测试 */
	RUN_TEST(test_conversion_null_input);
	RUN_TEST(test_conversion_null_output);
	RUN_TEST(test_string_ops_type_mismatch);

	return UNITY_END();
}
