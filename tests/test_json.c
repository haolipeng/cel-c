/**
 * @file test_json.c
 * @brief CEL JSON 转换单元测试
 */

#include "cel/cel_value.h"
#include "unity.h"
#include <string.h>
#include <stdlib.h>

/* ========== Unity 设置 ========== */

void setUp(void)
{
}

void tearDown(void)
{
}

/* ========== cel_value_to_json 测试 ========== */

void test_to_json_null(void)
{
	cel_value_t val = cel_value_null();
	char *json = cel_value_to_json(&val);
	TEST_ASSERT_NOT_NULL(json);
	TEST_ASSERT_EQUAL_STRING("null", json);
	free(json);
}

void test_to_json_bool_true(void)
{
	cel_value_t val = cel_value_bool(true);
	char *json = cel_value_to_json(&val);
	TEST_ASSERT_NOT_NULL(json);
	TEST_ASSERT_EQUAL_STRING("true", json);
	free(json);
}

void test_to_json_bool_false(void)
{
	cel_value_t val = cel_value_bool(false);
	char *json = cel_value_to_json(&val);
	TEST_ASSERT_NOT_NULL(json);
	TEST_ASSERT_EQUAL_STRING("false", json);
	free(json);
}

void test_to_json_int(void)
{
	cel_value_t val = cel_value_int(42);
	char *json = cel_value_to_json(&val);
	TEST_ASSERT_NOT_NULL(json);
	TEST_ASSERT_EQUAL_STRING("42", json);
	free(json);
}

void test_to_json_int_negative(void)
{
	cel_value_t val = cel_value_int(-123);
	char *json = cel_value_to_json(&val);
	TEST_ASSERT_NOT_NULL(json);
	TEST_ASSERT_EQUAL_STRING("-123", json);
	free(json);
}

void test_to_json_double(void)
{
	cel_value_t val = cel_value_double(3.14);
	char *json = cel_value_to_json(&val);
	TEST_ASSERT_NOT_NULL(json);
	/* cJSON 可能输出 3.14 或 3.1400000000000001 */
	TEST_ASSERT_TRUE(strstr(json, "3.14") != NULL);
	free(json);
}

void test_to_json_string(void)
{
	cel_value_t val = cel_value_string("hello");
	char *json = cel_value_to_json(&val);
	TEST_ASSERT_NOT_NULL(json);
	TEST_ASSERT_EQUAL_STRING("\"hello\"", json);
	free(json);
	cel_value_destroy(&val);
}

void test_to_json_string_with_quotes(void)
{
	cel_value_t val = cel_value_string("say \"hi\"");
	char *json = cel_value_to_json(&val);
	TEST_ASSERT_NOT_NULL(json);
	TEST_ASSERT_EQUAL_STRING("\"say \\\"hi\\\"\"", json);
	free(json);
	cel_value_destroy(&val);
}

void test_to_json_list_empty(void)
{
	cel_list_t *list = cel_list_create(0);
	cel_value_t val = cel_value_list(list);
	char *json = cel_value_to_json(&val);
	TEST_ASSERT_NOT_NULL(json);
	TEST_ASSERT_EQUAL_STRING("[]", json);
	free(json);
	cel_value_destroy(&val);
}

void test_to_json_list(void)
{
	cel_list_t *list = cel_list_create(3);
	cel_value_t v1 = cel_value_int(1);
	cel_value_t v2 = cel_value_int(2);
	cel_value_t v3 = cel_value_int(3);
	cel_list_append(list, &v1);
	cel_list_append(list, &v2);
	cel_list_append(list, &v3);

	cel_value_t val = cel_value_list(list);
	char *json = cel_value_to_json(&val);
	TEST_ASSERT_NOT_NULL(json);
	TEST_ASSERT_EQUAL_STRING("[1,2,3]", json);
	free(json);
	cel_value_destroy(&val);
}

void test_to_json_map_empty(void)
{
	cel_map_t *map = cel_map_create(0);
	cel_value_t val = cel_value_map(map);
	char *json = cel_value_to_json(&val);
	TEST_ASSERT_NOT_NULL(json);
	TEST_ASSERT_EQUAL_STRING("{}", json);
	free(json);
	cel_value_destroy(&val);
}

void test_to_json_map(void)
{
	cel_map_t *map = cel_map_create(2);
	cel_value_t k1 = cel_value_string("name");
	cel_value_t v1 = cel_value_string("Alice");
	cel_value_t k2 = cel_value_string("age");
	cel_value_t v2 = cel_value_int(30);
	cel_map_put(map, &k1, &v1);
	cel_map_put(map, &k2, &v2);
	cel_value_destroy(&k1);
	cel_value_destroy(&v1);
	cel_value_destroy(&k2);

	cel_value_t val = cel_value_map(map);
	char *json = cel_value_to_json(&val);
	TEST_ASSERT_NOT_NULL(json);
	/* Map 顺序可能不确定，检查包含关键内容 */
	TEST_ASSERT_TRUE(strstr(json, "\"name\":\"Alice\"") != NULL);
	TEST_ASSERT_TRUE(strstr(json, "\"age\":30") != NULL);
	free(json);
	cel_value_destroy(&val);
}

/* ========== cel_value_from_json 测试 ========== */

void test_from_json_null(void)
{
	cel_value_t val = cel_value_from_json("null");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_NULL, val.type);
}

void test_from_json_bool_true(void)
{
	cel_value_t val = cel_value_from_json("true");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, val.type);
	TEST_ASSERT_TRUE(val.value.bool_value);
}

void test_from_json_bool_false(void)
{
	cel_value_t val = cel_value_from_json("false");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, val.type);
	TEST_ASSERT_FALSE(val.value.bool_value);
}

void test_from_json_int(void)
{
	cel_value_t val = cel_value_from_json("42");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, val.type);
	TEST_ASSERT_EQUAL_INT64(42, val.value.int_value);
}

void test_from_json_int_negative(void)
{
	cel_value_t val = cel_value_from_json("-123");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, val.type);
	TEST_ASSERT_EQUAL_INT64(-123, val.value.int_value);
}

void test_from_json_double(void)
{
	cel_value_t val = cel_value_from_json("3.14");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_DOUBLE, val.type);
	TEST_ASSERT_DOUBLE_WITHIN(0.001, 3.14, val.value.double_value);
}

void test_from_json_string(void)
{
	cel_value_t val = cel_value_from_json("\"hello\"");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_STRING, val.type);
	TEST_ASSERT_EQUAL_STRING("hello", val.value.string_value->data);
	cel_value_destroy(&val);
}

void test_from_json_list_empty(void)
{
	cel_value_t val = cel_value_from_json("[]");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_LIST, val.type);
	TEST_ASSERT_EQUAL_INT(0, cel_list_size(val.value.list_value));
	cel_value_destroy(&val);
}

void test_from_json_list(void)
{
	cel_value_t val = cel_value_from_json("[1, 2, 3]");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_LIST, val.type);
	TEST_ASSERT_EQUAL_INT(3, cel_list_size(val.value.list_value));

	cel_value_t *item0 = cel_list_get(val.value.list_value, 0);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, item0->type);
	TEST_ASSERT_EQUAL_INT64(1, item0->value.int_value);

	cel_value_destroy(&val);
}

void test_from_json_map_empty(void)
{
	cel_value_t val = cel_value_from_json("{}");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_MAP, val.type);
	TEST_ASSERT_EQUAL_INT(0, cel_map_size(val.value.map_value));
	cel_value_destroy(&val);
}

void test_from_json_map(void)
{
	cel_value_t val = cel_value_from_json("{\"name\": \"Bob\", \"age\": 25}");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_MAP, val.type);
	TEST_ASSERT_EQUAL_INT(2, cel_map_size(val.value.map_value));

	cel_value_t key = cel_value_string("name");
	cel_value_t *name = cel_map_get(val.value.map_value, &key);
	TEST_ASSERT_NOT_NULL(name);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_STRING, name->type);
	TEST_ASSERT_EQUAL_STRING("Bob", name->value.string_value->data);
	cel_value_destroy(&key);

	cel_value_destroy(&val);
}

/* ========== 往返测试 ========== */

void test_roundtrip_nested(void)
{
	const char *json = "{\"users\":[{\"name\":\"Alice\",\"active\":true}],\"count\":1}";
	cel_value_t val = cel_value_from_json(json);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_MAP, val.type);

	char *json_out = cel_value_to_json(&val);
	TEST_ASSERT_NOT_NULL(json_out);
	/* 验证关键内容存在 */
	TEST_ASSERT_TRUE(strstr(json_out, "\"users\"") != NULL);
	TEST_ASSERT_TRUE(strstr(json_out, "\"Alice\"") != NULL);
	TEST_ASSERT_TRUE(strstr(json_out, "\"count\":1") != NULL);

	free(json_out);
	cel_value_destroy(&val);
}

void test_from_json_invalid(void)
{
	cel_value_t val = cel_value_from_json("invalid json");
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_NULL, val.type);
}

void test_from_json_null_input(void)
{
	cel_value_t val = cel_value_from_json(NULL);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_NULL, val.type);
}

/* ========== Main 测试运行器 ========== */

int main(void)
{
	UNITY_BEGIN();

	/* cel_value_to_json 测试 */
	RUN_TEST(test_to_json_null);
	RUN_TEST(test_to_json_bool_true);
	RUN_TEST(test_to_json_bool_false);
	RUN_TEST(test_to_json_int);
	RUN_TEST(test_to_json_int_negative);
	RUN_TEST(test_to_json_double);
	RUN_TEST(test_to_json_string);
	RUN_TEST(test_to_json_string_with_quotes);
	RUN_TEST(test_to_json_list_empty);
	RUN_TEST(test_to_json_list);
	RUN_TEST(test_to_json_map_empty);
	RUN_TEST(test_to_json_map);

	/* cel_value_from_json 测试 */
	RUN_TEST(test_from_json_null);
	RUN_TEST(test_from_json_bool_true);
	RUN_TEST(test_from_json_bool_false);
	RUN_TEST(test_from_json_int);
	RUN_TEST(test_from_json_int_negative);
	RUN_TEST(test_from_json_double);
	RUN_TEST(test_from_json_string);
	RUN_TEST(test_from_json_list_empty);
	RUN_TEST(test_from_json_list);
	RUN_TEST(test_from_json_map_empty);
	RUN_TEST(test_from_json_map);

	/* 往返测试 */
	RUN_TEST(test_roundtrip_nested);

	/* 错误处理测试 */
	RUN_TEST(test_from_json_invalid);
	RUN_TEST(test_from_json_null_input);

	return UNITY_END();
}
