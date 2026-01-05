/**
 * @file test_value.c
 * @brief CEL-C 基础值类型单元测试
 */

#include "cel/cel_value.h"
#include "test_helpers.h"
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

/* ========== null 值测试 ========== */

void test_value_null(void)
{
	cel_value_t value = cel_value_null();

	TEST_ASSERT_EQUAL(CEL_TYPE_NULL, value.type);
	TEST_ASSERT_TRUE(cel_value_is_null(&value));
	TEST_ASSERT_FALSE(cel_value_is_bool(&value));

	/* 销毁 null 值应该安全 */
	cel_value_destroy(&value);
}

void test_value_null_convenience_macro(void)
{
	cel_value_t value = CEL_NULL;

	TEST_ASSERT_EQUAL(CEL_TYPE_NULL, value.type);
	TEST_ASSERT_TRUE(cel_value_is_null(&value));
}

/* ========== bool 值测试 ========== */

void test_value_bool_true(void)
{
	cel_value_t value = cel_value_bool(true);

	TEST_ASSERT_EQUAL(CEL_TYPE_BOOL, value.type);
	TEST_ASSERT_TRUE(cel_value_is_bool(&value));

	bool result;
	TEST_ASSERT_TRUE(cel_value_get_bool(&value, &result));
	TEST_ASSERT_TRUE(result);

	cel_value_destroy(&value);
}

void test_value_bool_false(void)
{
	cel_value_t value = cel_value_bool(false);

	TEST_ASSERT_EQUAL(CEL_TYPE_BOOL, value.type);

	bool result;
	TEST_ASSERT_TRUE(cel_value_get_bool(&value, &result));
	TEST_ASSERT_FALSE(result);

	cel_value_destroy(&value);
}

void test_value_bool_convenience_macros(void)
{
	cel_value_t val_true = CEL_TRUE;
	cel_value_t val_false = CEL_FALSE;

	bool result;
	TEST_ASSERT_TRUE(cel_value_get_bool(&val_true, &result));
	TEST_ASSERT_TRUE(result);

	TEST_ASSERT_TRUE(cel_value_get_bool(&val_false, &result));
	TEST_ASSERT_FALSE(result);
}

void test_value_bool_type_mismatch(void)
{
	cel_value_t value = cel_value_int(42);

	bool result;
	TEST_ASSERT_FALSE(cel_value_get_bool(&value, &result));
}

/* ========== int 值测试 ========== */

void test_value_int_positive(void)
{
	cel_value_t value = cel_value_int(42);

	TEST_ASSERT_EQUAL(CEL_TYPE_INT, value.type);
	TEST_ASSERT_TRUE(cel_value_is_int(&value));

	int64_t result;
	TEST_ASSERT_TRUE(cel_value_get_int(&value, &result));
	TEST_ASSERT_EQUAL(42, result);

	cel_value_destroy(&value);
}

void test_value_int_negative(void)
{
	cel_value_t value = cel_value_int(-100);

	int64_t result;
	TEST_ASSERT_TRUE(cel_value_get_int(&value, &result));
	TEST_ASSERT_EQUAL(-100, result);
}

void test_value_int_zero(void)
{
	cel_value_t value = cel_value_int(0);

	int64_t result;
	TEST_ASSERT_TRUE(cel_value_get_int(&value, &result));
	TEST_ASSERT_EQUAL(0, result);
}

void test_value_int_max(void)
{
	cel_value_t value = cel_value_int(INT64_MAX);

	int64_t result;
	TEST_ASSERT_TRUE(cel_value_get_int(&value, &result));
	TEST_ASSERT_EQUAL(INT64_MAX, result);
}

void test_value_int_min(void)
{
	cel_value_t value = cel_value_int(INT64_MIN);

	int64_t result;
	TEST_ASSERT_TRUE(cel_value_get_int(&value, &result));
	TEST_ASSERT_EQUAL(INT64_MIN, result);
}

void test_value_int_convenience_macro(void)
{
	cel_value_t value = CEL_INT(123);

	int64_t result;
	TEST_ASSERT_TRUE(cel_value_get_int(&value, &result));
	TEST_ASSERT_EQUAL(123, result);
}

/* ========== uint 值测试 ========== */

void test_value_uint(void)
{
	cel_value_t value = cel_value_uint(42);

	TEST_ASSERT_EQUAL(CEL_TYPE_UINT, value.type);
	TEST_ASSERT_TRUE(cel_value_is_uint(&value));

	uint64_t result;
	TEST_ASSERT_TRUE(cel_value_get_uint(&value, &result));
	TEST_ASSERT_EQUAL(42, result);

	cel_value_destroy(&value);
}

void test_value_uint_zero(void)
{
	cel_value_t value = cel_value_uint(0);

	uint64_t result;
	TEST_ASSERT_TRUE(cel_value_get_uint(&value, &result));
	TEST_ASSERT_EQUAL(0, result);
}

void test_value_uint_max(void)
{
	cel_value_t value = cel_value_uint(UINT64_MAX);

	uint64_t result;
	TEST_ASSERT_TRUE(cel_value_get_uint(&value, &result));
	TEST_ASSERT_EQUAL(UINT64_MAX, result);
}

void test_value_uint_convenience_macro(void)
{
	cel_value_t value = CEL_UINT(456);

	uint64_t result;
	TEST_ASSERT_TRUE(cel_value_get_uint(&value, &result));
	TEST_ASSERT_EQUAL(456, result);
}

/* ========== double 值测试 ========== */

void test_value_double(void)
{
	cel_value_t value = cel_value_double(3.14159);

	TEST_ASSERT_EQUAL(CEL_TYPE_DOUBLE, value.type);
	TEST_ASSERT_TRUE(cel_value_is_double(&value));

	double result;
	TEST_ASSERT_TRUE(cel_value_get_double(&value, &result));
	TEST_ASSERT_DOUBLE_WITHIN(0.00001, 3.14159, result);

	cel_value_destroy(&value);
}

void test_value_double_zero(void)
{
	cel_value_t value = cel_value_double(0.0);

	double result;
	TEST_ASSERT_TRUE(cel_value_get_double(&value, &result));
	TEST_ASSERT_EQUAL_DOUBLE(0.0, result);
}

void test_value_double_negative(void)
{
	cel_value_t value = cel_value_double(-2.71828);

	double result;
	TEST_ASSERT_TRUE(cel_value_get_double(&value, &result));
	TEST_ASSERT_DOUBLE_WITHIN(0.00001, -2.71828, result);
}

void test_value_double_convenience_macro(void)
{
	cel_value_t value = CEL_DOUBLE(1.414);

	double result;
	TEST_ASSERT_TRUE(cel_value_get_double(&value, &result));
	TEST_ASSERT_DOUBLE_WITHIN(0.001, 1.414, result);
}

/* ========== string 值测试 ========== */

void test_value_string_basic(void)
{
	cel_value_t value = cel_value_string("hello");

	TEST_ASSERT_EQUAL(CEL_TYPE_STRING, value.type);
	TEST_ASSERT_TRUE(cel_value_is_string(&value));

	const char *str;
	size_t len;
	TEST_ASSERT_TRUE(cel_value_get_string(&value, &str, &len));
	TEST_ASSERT_EQUAL(5, len);
	TEST_ASSERT_EQUAL_STRING("hello", str);

	cel_value_destroy(&value);
}

void test_value_string_empty(void)
{
	cel_value_t value = cel_value_string("");

	const char *str;
	size_t len;
	TEST_ASSERT_TRUE(cel_value_get_string(&value, &str, &len));
	TEST_ASSERT_EQUAL(0, len);
	TEST_ASSERT_EQUAL_STRING("", str);

	cel_value_destroy(&value);
}

void test_value_string_with_length(void)
{
	cel_value_t value = cel_value_string_n("hello world", 5);

	const char *str;
	size_t len;
	TEST_ASSERT_TRUE(cel_value_get_string(&value, &str, &len));
	TEST_ASSERT_EQUAL(5, len);
	TEST_ASSERT_EQUAL_STRING("hello", str);

	cel_value_destroy(&value);
}

void test_value_string_with_null_chars(void)
{
	const char data[] = "hello\0world";
	cel_value_t value = cel_value_string_n(data, 11);

	const char *str;
	size_t len;
	TEST_ASSERT_TRUE(cel_value_get_string(&value, &str, &len));
	TEST_ASSERT_EQUAL(11, len);
	TEST_ASSERT_EQUAL_MEMORY(data, str, 11);

	cel_value_destroy(&value);
}

void test_value_string_null_input(void)
{
	cel_value_t value = cel_value_string(NULL);

	TEST_ASSERT_EQUAL(CEL_TYPE_NULL, value.type);
}

void test_value_string_convenience_macro(void)
{
	cel_value_t value = CEL_STRING("test");

	const char *str;
	TEST_ASSERT_TRUE(cel_value_get_string(&value, &str, NULL));
	TEST_ASSERT_EQUAL_STRING("test", str);

	cel_value_destroy(&value);
}

/* ========== bytes 值测试 ========== */

void test_value_bytes_basic(void)
{
	unsigned char data[] = {0x01, 0x02, 0x03, 0x04};
	cel_value_t value = cel_value_bytes(data, 4);

	TEST_ASSERT_EQUAL(CEL_TYPE_BYTES, value.type);
	TEST_ASSERT_TRUE(cel_value_is_bytes(&value));

	const unsigned char *bytes;
	size_t len;
	TEST_ASSERT_TRUE(cel_value_get_bytes(&value, &bytes, &len));
	TEST_ASSERT_EQUAL(4, len);
	TEST_ASSERT_EQUAL_MEMORY(data, bytes, 4);

	cel_value_destroy(&value);
}

void test_value_bytes_empty(void)
{
	cel_value_t value = cel_value_bytes(NULL, 0);

	const unsigned char *bytes;
	size_t len;
	TEST_ASSERT_TRUE(cel_value_get_bytes(&value, &bytes, &len));
	TEST_ASSERT_EQUAL(0, len);

	cel_value_destroy(&value);
}

void test_value_bytes_with_zeros(void)
{
	unsigned char data[] = {0x00, 0x00, 0xFF, 0x00};
	cel_value_t value = cel_value_bytes(data, 4);

	const unsigned char *bytes;
	size_t len;
	TEST_ASSERT_TRUE(cel_value_get_bytes(&value, &bytes, &len));
	TEST_ASSERT_EQUAL(4, len);
	TEST_ASSERT_EQUAL_MEMORY(data, bytes, 4);

	cel_value_destroy(&value);
}

/* ========== 引用计数测试 ========== */

void test_string_reference_counting(void)
{
	cel_string_t *str = cel_string_create("test", 4);
	TEST_ASSERT_NOT_NULL(str);
	TEST_ASSERT_EQUAL(1, str->ref_count);

	/* 增加引用 */
	cel_string_t *str2 = cel_string_retain(str);
	TEST_ASSERT_EQUAL(str, str2);
	TEST_ASSERT_EQUAL(2, str->ref_count);

	/* 释放一次，引用计数减1 */
	cel_string_release(str);
	TEST_ASSERT_EQUAL(1, str->ref_count);

	/* 再释放一次，字符串被释放 */
	cel_string_release(str2);
	/* str 已被释放，不能再访问 */
}

void test_bytes_reference_counting(void)
{
	unsigned char data[] = {0x01, 0x02};
	cel_bytes_t *bytes = cel_bytes_create(data, 2);
	TEST_ASSERT_NOT_NULL(bytes);
	TEST_ASSERT_EQUAL(1, bytes->ref_count);

	/* 增加引用 */
	cel_bytes_t *bytes2 = cel_bytes_retain(bytes);
	TEST_ASSERT_EQUAL(bytes, bytes2);
	TEST_ASSERT_EQUAL(2, bytes->ref_count);

	/* 释放 */
	cel_bytes_release(bytes);
	TEST_ASSERT_EQUAL(1, bytes->ref_count);

	cel_bytes_release(bytes2);
}

void test_string_retain_null(void)
{
	cel_string_t *str = cel_string_retain(NULL);
	TEST_ASSERT_NULL(str);
}

void test_string_release_null(void)
{
	/* 不应该崩溃 */
	cel_string_release(NULL);
}

void test_bytes_retain_null(void)
{
	cel_bytes_t *bytes = cel_bytes_retain(NULL);
	TEST_ASSERT_NULL(bytes);
}

void test_bytes_release_null(void)
{
	/* 不应该崩溃 */
	cel_bytes_release(NULL);
}

/* ========== 类型检查测试 ========== */

void test_type_name(void)
{
	TEST_ASSERT_EQUAL_STRING("null", cel_type_name(CEL_TYPE_NULL));
	TEST_ASSERT_EQUAL_STRING("bool", cel_type_name(CEL_TYPE_BOOL));
	TEST_ASSERT_EQUAL_STRING("int", cel_type_name(CEL_TYPE_INT));
	TEST_ASSERT_EQUAL_STRING("uint", cel_type_name(CEL_TYPE_UINT));
	TEST_ASSERT_EQUAL_STRING("double", cel_type_name(CEL_TYPE_DOUBLE));
	TEST_ASSERT_EQUAL_STRING("string", cel_type_name(CEL_TYPE_STRING));
	TEST_ASSERT_EQUAL_STRING("bytes", cel_type_name(CEL_TYPE_BYTES));
}

void test_value_type(void)
{
	cel_value_t val_null = cel_value_null();
	cel_value_t val_bool = cel_value_bool(true);
	cel_value_t val_int = cel_value_int(42);
	cel_value_t val_string = cel_value_string("test");

	TEST_ASSERT_EQUAL(CEL_TYPE_NULL, cel_value_type(&val_null));
	TEST_ASSERT_EQUAL(CEL_TYPE_BOOL, cel_value_type(&val_bool));
	TEST_ASSERT_EQUAL(CEL_TYPE_INT, cel_value_type(&val_int));
	TEST_ASSERT_EQUAL(CEL_TYPE_STRING, cel_value_type(&val_string));

	cel_value_destroy(&val_string);
}

/* ========== 值比较测试 ========== */

void test_value_equals_null(void)
{
	cel_value_t a = cel_value_null();
	cel_value_t b = cel_value_null();

	TEST_ASSERT_TRUE(cel_value_equals(&a, &b));
}

void test_value_equals_bool(void)
{
	cel_value_t a = cel_value_bool(true);
	cel_value_t b = cel_value_bool(true);
	cel_value_t c = cel_value_bool(false);

	TEST_ASSERT_TRUE(cel_value_equals(&a, &b));
	TEST_ASSERT_FALSE(cel_value_equals(&a, &c));
}

void test_value_equals_int(void)
{
	cel_value_t a = cel_value_int(42);
	cel_value_t b = cel_value_int(42);
	cel_value_t c = cel_value_int(100);

	TEST_ASSERT_TRUE(cel_value_equals(&a, &b));
	TEST_ASSERT_FALSE(cel_value_equals(&a, &c));
}

void test_value_equals_uint(void)
{
	cel_value_t a = cel_value_uint(42);
	cel_value_t b = cel_value_uint(42);
	cel_value_t c = cel_value_uint(100);

	TEST_ASSERT_TRUE(cel_value_equals(&a, &b));
	TEST_ASSERT_FALSE(cel_value_equals(&a, &c));
}

void test_value_equals_double(void)
{
	cel_value_t a = cel_value_double(3.14);
	cel_value_t b = cel_value_double(3.14);
	cel_value_t c = cel_value_double(2.71);

	TEST_ASSERT_TRUE(cel_value_equals(&a, &b));
	TEST_ASSERT_FALSE(cel_value_equals(&a, &c));
}

void test_value_equals_string(void)
{
	cel_value_t a = cel_value_string("hello");
	cel_value_t b = cel_value_string("hello");
	cel_value_t c = cel_value_string("world");

	TEST_ASSERT_TRUE(cel_value_equals(&a, &b));
	TEST_ASSERT_FALSE(cel_value_equals(&a, &c));

	cel_value_destroy(&a);
	cel_value_destroy(&b);
	cel_value_destroy(&c);
}

void test_value_equals_bytes(void)
{
	unsigned char data1[] = {0x01, 0x02};
	unsigned char data2[] = {0x01, 0x02};
	unsigned char data3[] = {0xFF, 0xFE};

	cel_value_t a = cel_value_bytes(data1, 2);
	cel_value_t b = cel_value_bytes(data2, 2);
	cel_value_t c = cel_value_bytes(data3, 2);

	TEST_ASSERT_TRUE(cel_value_equals(&a, &b));
	TEST_ASSERT_FALSE(cel_value_equals(&a, &c));

	cel_value_destroy(&a);
	cel_value_destroy(&b);
	cel_value_destroy(&c);
}

void test_value_equals_different_types(void)
{
	cel_value_t a = cel_value_int(42);
	cel_value_t b = cel_value_uint(42);
	cel_value_t c = cel_value_double(42.0);

	/* 不同类型永远不相等 */
	TEST_ASSERT_FALSE(cel_value_equals(&a, &b));
	TEST_ASSERT_FALSE(cel_value_equals(&a, &c));
	TEST_ASSERT_FALSE(cel_value_equals(&b, &c));
}

/* ========== 销毁测试 ========== */

void test_value_destroy_null(void)
{
	/* 不应该崩溃 */
	cel_value_destroy(NULL);
}

void test_value_destroy_basic_types(void)
{
	cel_value_t val_bool = cel_value_bool(true);
	cel_value_t val_int = cel_value_int(42);
	cel_value_t val_uint = cel_value_uint(42);
	cel_value_t val_double = cel_value_double(3.14);

	/* 销毁基本类型应该安全 */
	cel_value_destroy(&val_bool);
	cel_value_destroy(&val_int);
	cel_value_destroy(&val_uint);
	cel_value_destroy(&val_double);
}

/* ========== Unity 主函数 ========== */

int main(void)
{
	UNITY_BEGIN();

	/* null 值测试 */
	RUN_TEST(test_value_null);
	RUN_TEST(test_value_null_convenience_macro);

	/* bool 值测试 */
	RUN_TEST(test_value_bool_true);
	RUN_TEST(test_value_bool_false);
	RUN_TEST(test_value_bool_convenience_macros);
	RUN_TEST(test_value_bool_type_mismatch);

	/* int 值测试 */
	RUN_TEST(test_value_int_positive);
	RUN_TEST(test_value_int_negative);
	RUN_TEST(test_value_int_zero);
	RUN_TEST(test_value_int_max);
	RUN_TEST(test_value_int_min);
	RUN_TEST(test_value_int_convenience_macro);

	/* uint 值测试 */
	RUN_TEST(test_value_uint);
	RUN_TEST(test_value_uint_zero);
	RUN_TEST(test_value_uint_max);
	RUN_TEST(test_value_uint_convenience_macro);

	/* double 值测试 */
	RUN_TEST(test_value_double);
	RUN_TEST(test_value_double_zero);
	RUN_TEST(test_value_double_negative);
	RUN_TEST(test_value_double_convenience_macro);

	/* string 值测试 */
	RUN_TEST(test_value_string_basic);
	RUN_TEST(test_value_string_empty);
	RUN_TEST(test_value_string_with_length);
	RUN_TEST(test_value_string_with_null_chars);
	RUN_TEST(test_value_string_null_input);
	RUN_TEST(test_value_string_convenience_macro);

	/* bytes 值测试 */
	RUN_TEST(test_value_bytes_basic);
	RUN_TEST(test_value_bytes_empty);
	RUN_TEST(test_value_bytes_with_zeros);

	/* 引用计数测试 */
	RUN_TEST(test_string_reference_counting);
	RUN_TEST(test_bytes_reference_counting);
	RUN_TEST(test_string_retain_null);
	RUN_TEST(test_string_release_null);
	RUN_TEST(test_bytes_retain_null);
	RUN_TEST(test_bytes_release_null);

	/* 类型检查测试 */
	RUN_TEST(test_type_name);
	RUN_TEST(test_value_type);

	/* 值比较测试 */
	RUN_TEST(test_value_equals_null);
	RUN_TEST(test_value_equals_bool);
	RUN_TEST(test_value_equals_int);
	RUN_TEST(test_value_equals_uint);
	RUN_TEST(test_value_equals_double);
	RUN_TEST(test_value_equals_string);
	RUN_TEST(test_value_equals_bytes);
	RUN_TEST(test_value_equals_different_types);

	/* 销毁测试 */
	RUN_TEST(test_value_destroy_null);
	RUN_TEST(test_value_destroy_basic_types);

	return UNITY_END();
}
