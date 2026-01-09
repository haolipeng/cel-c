/**
 * @file test_list_map.c
 * @brief CEL-C 容器类型单元测试
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

/* ========== 列表测试 ========== */

void test_list_create_and_destroy(void)
{
	cel_list_t *list = cel_list_create(0);
	TEST_ASSERT_NOT_NULL(list);
	TEST_ASSERT_EQUAL(0, cel_list_size(list));
	TEST_ASSERT_EQUAL(1, list->ref_count);

	cel_list_release(list);
}

void test_list_append(void)
{
	cel_list_t *list = cel_list_create(0);
	TEST_ASSERT_NOT_NULL(list);

	cel_value_t v1 = cel_value_int(42);
	cel_value_t v2 = cel_value_string("hello");
	cel_value_t v3 = cel_value_bool(true);

	TEST_ASSERT_TRUE(cel_list_append(list, &v1));
	TEST_ASSERT_EQUAL(1, cel_list_size(list));

	TEST_ASSERT_TRUE(cel_list_append(list, &v2));
	TEST_ASSERT_EQUAL(2, cel_list_size(list));

	TEST_ASSERT_TRUE(cel_list_append(list, &v3));
	TEST_ASSERT_EQUAL(3, cel_list_size(list));

	cel_value_destroy(&v2);
	cel_list_release(list);
}

void test_list_get(void)
{
	cel_list_t *list = cel_list_create(0);

	cel_value_t v1 = cel_value_int(10);
	cel_value_t v2 = cel_value_int(20);
	cel_value_t v3 = cel_value_int(30);

	cel_list_append(list, &v1);
	cel_list_append(list, &v2);
	cel_list_append(list, &v3);

	cel_value_t *result = cel_list_get(list, 0);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_TRUE(cel_value_is_int(result));
	int64_t val;
	cel_value_get_int(result, &val);
	TEST_ASSERT_EQUAL(10, val);

	result = cel_list_get(list, 1);
	cel_value_get_int(result, &val);
	TEST_ASSERT_EQUAL(20, val);

	result = cel_list_get(list, 2);
	cel_value_get_int(result, &val);
	TEST_ASSERT_EQUAL(30, val);

	/* 越界访问 */
	result = cel_list_get(list, 3);
	TEST_ASSERT_NULL(result);

	cel_list_release(list);
}

void test_list_set(void)
{
	cel_list_t *list = cel_list_create(0);

	cel_value_t v1 = cel_value_int(10);
	cel_value_t v2 = cel_value_int(20);

	cel_list_append(list, &v1);
	cel_list_append(list, &v2);

	/* 修改第一个元素 */
	cel_value_t new_val = cel_value_int(100);
	TEST_ASSERT_TRUE(cel_list_set(list, 0, &new_val));

	cel_value_t *result = cel_list_get(list, 0);
	int64_t val;
	cel_value_get_int(result, &val);
	TEST_ASSERT_EQUAL(100, val);

	/* 越界设置 */
	TEST_ASSERT_FALSE(cel_list_set(list, 10, &new_val));

	cel_list_release(list);
}

void test_list_reference_counting(void)
{
	cel_list_t *list = cel_list_create(0);
	TEST_ASSERT_EQUAL(1, list->ref_count);

	cel_list_t *list2 = cel_list_retain(list);
	TEST_ASSERT_EQUAL_PTR(list, list2);
	TEST_ASSERT_EQUAL(2, list->ref_count);

	cel_list_release(list2);
	TEST_ASSERT_EQUAL(1, list->ref_count);

	cel_list_release(list);
}

void test_list_value_wrapper(void)
{
	cel_list_t *list = cel_list_create(0);
	cel_value_t v1 = cel_value_int(42);
	cel_list_append(list, &v1);

	cel_value_t list_value = cel_value_list(list);
	TEST_ASSERT_TRUE(cel_value_is_list(&list_value));
	TEST_ASSERT_EQUAL(CEL_TYPE_LIST, list_value.type);

	cel_list_t *retrieved;
	TEST_ASSERT_TRUE(cel_value_get_list(&list_value, &retrieved));
	TEST_ASSERT_EQUAL_PTR(list, retrieved);
	TEST_ASSERT_EQUAL(1, cel_list_size(retrieved));

	cel_value_destroy(&list_value);
}

void test_list_with_mixed_types(void)
{
	cel_list_t *list = cel_list_create(0);

	cel_value_t v_int = cel_value_int(42);
	cel_value_t v_str = cel_value_string("hello");
	cel_value_t v_bool = cel_value_bool(true);
	cel_value_t v_double = cel_value_double(3.14);

	cel_list_append(list, &v_int);
	cel_list_append(list, &v_str);
	cel_list_append(list, &v_bool);
	cel_list_append(list, &v_double);

	TEST_ASSERT_EQUAL(4, cel_list_size(list));

	/* 验证类型 */
	TEST_ASSERT_TRUE(cel_value_is_int(cel_list_get(list, 0)));
	TEST_ASSERT_TRUE(cel_value_is_string(cel_list_get(list, 1)));
	TEST_ASSERT_TRUE(cel_value_is_bool(cel_list_get(list, 2)));
	TEST_ASSERT_TRUE(cel_value_is_double(cel_list_get(list, 3)));

	cel_value_destroy(&v_str);
	cel_list_release(list);
}

void test_list_nested(void)
{
	/* 创建内层列表 */
	cel_list_t *inner_list = cel_list_create(0);
	cel_value_t v1 = cel_value_int(1);
	cel_value_t v2 = cel_value_int(2);
	cel_list_append(inner_list, &v1);
	cel_list_append(inner_list, &v2);

	/* 创建外层列表 */
	cel_list_t *outer_list = cel_list_create(0);
	cel_value_t inner_value = cel_value_list(inner_list);
	cel_list_append(outer_list, &inner_value);

	/* 释放原始引用 (append 已增加引用计数) */
	cel_list_release(inner_list);

	TEST_ASSERT_EQUAL(1, cel_list_size(outer_list));

	cel_value_t *retrieved = cel_list_get(outer_list, 0);
	TEST_ASSERT_TRUE(cel_value_is_list(retrieved));

	cel_list_t *retrieved_list;
	cel_value_get_list(retrieved, &retrieved_list);
	TEST_ASSERT_EQUAL(2, cel_list_size(retrieved_list));

	cel_list_release(outer_list);
}

void test_list_equals(void)
{
	cel_list_t *list1 = cel_list_create(0);
	cel_list_t *list2 = cel_list_create(0);

	cel_value_t v1 = cel_value_int(10);
	cel_value_t v2 = cel_value_int(20);

	cel_list_append(list1, &v1);
	cel_list_append(list1, &v2);

	cel_list_append(list2, &v1);
	cel_list_append(list2, &v2);

	cel_value_t val1 = cel_value_list(list1);
	cel_value_t val2 = cel_value_list(list2);

	TEST_ASSERT_TRUE(cel_value_equals(&val1, &val2));

	/* 修改list2 */
	cel_value_t v3 = cel_value_int(30);
	cel_list_append(list2, &v3);

	TEST_ASSERT_FALSE(cel_value_equals(&val1, &val2));

	cel_value_destroy(&val1);
	cel_value_destroy(&val2);
}

/* ========== 映射测试 ========== */

void test_map_create_and_destroy(void)
{
	cel_map_t *map = cel_map_create(0);
	TEST_ASSERT_NOT_NULL(map);
	TEST_ASSERT_EQUAL(0, cel_map_size(map));
	TEST_ASSERT_EQUAL(1, map->ref_count);

	cel_map_release(map);
}

void test_map_put_and_get(void)
{
	cel_map_t *map = cel_map_create(0);

	cel_value_t key1 = cel_value_string("name");
	cel_value_t val1 = cel_value_string("Alice");

	TEST_ASSERT_TRUE(cel_map_put(map, &key1, &val1));
	TEST_ASSERT_EQUAL(1, cel_map_size(map));

	cel_value_t *retrieved = cel_map_get(map, &key1);
	TEST_ASSERT_NOT_NULL(retrieved);
	TEST_ASSERT_TRUE(cel_value_is_string(retrieved));

	const char *str;
	cel_value_get_string(retrieved, &str, NULL);
	TEST_ASSERT_EQUAL_STRING("Alice", str);

	cel_value_destroy(&key1);
	cel_value_destroy(&val1);
	cel_map_release(map);
}

void test_map_put_update(void)
{
	cel_map_t *map = cel_map_create(0);

	cel_value_t key = cel_value_string("age");
	cel_value_t val1 = cel_value_int(25);
	cel_value_t val2 = cel_value_int(30);

	/* 第一次插入 */
	TEST_ASSERT_TRUE(cel_map_put(map, &key, &val1));
	TEST_ASSERT_EQUAL(1, cel_map_size(map));

	/* 更新值 */
	TEST_ASSERT_TRUE(cel_map_put(map, &key, &val2));
	TEST_ASSERT_EQUAL(1, cel_map_size(map)); /* 大小不变 */

	cel_value_t *retrieved = cel_map_get(map, &key);
	int64_t age;
	cel_value_get_int(retrieved, &age);
	TEST_ASSERT_EQUAL(30, age);

	cel_value_destroy(&key);
	cel_map_release(map);
}

void test_map_contains(void)
{
	cel_map_t *map = cel_map_create(0);

	cel_value_t key1 = cel_value_string("a");
	cel_value_t val1 = cel_value_int(1);

	cel_map_put(map, &key1, &val1);

	TEST_ASSERT_TRUE(cel_map_contains(map, &key1));

	cel_value_t key2 = cel_value_string("b");
	TEST_ASSERT_FALSE(cel_map_contains(map, &key2));

	cel_value_destroy(&key1);
	cel_value_destroy(&key2);
	cel_map_release(map);
}

void test_map_remove(void)
{
	cel_map_t *map = cel_map_create(0);

	cel_value_t key1 = cel_value_string("x");
	cel_value_t val1 = cel_value_int(10);

	cel_map_put(map, &key1, &val1);
	TEST_ASSERT_EQUAL(1, cel_map_size(map));

	TEST_ASSERT_TRUE(cel_map_remove(map, &key1));
	TEST_ASSERT_EQUAL(0, cel_map_size(map));
	TEST_ASSERT_FALSE(cel_map_contains(map, &key1));

	/* 再次删除应该失败 */
	TEST_ASSERT_FALSE(cel_map_remove(map, &key1));

	cel_value_destroy(&key1);
	cel_map_release(map);
}

void test_map_reference_counting(void)
{
	cel_map_t *map = cel_map_create(0);
	TEST_ASSERT_EQUAL(1, map->ref_count);

	cel_map_t *map2 = cel_map_retain(map);
	TEST_ASSERT_EQUAL_PTR(map, map2);
	TEST_ASSERT_EQUAL(2, map->ref_count);

	cel_map_release(map2);
	TEST_ASSERT_EQUAL(1, map->ref_count);

	cel_map_release(map);
}

void test_map_value_wrapper(void)
{
	cel_map_t *map = cel_map_create(0);
	cel_value_t key = cel_value_string("test");
	cel_value_t val = cel_value_int(42);
	cel_map_put(map, &key, &val);

	cel_value_t map_value = cel_value_map(map);
	TEST_ASSERT_TRUE(cel_value_is_map(&map_value));
	TEST_ASSERT_EQUAL(CEL_TYPE_MAP, map_value.type);

	cel_map_t *retrieved;
	TEST_ASSERT_TRUE(cel_value_get_map(&map_value, &retrieved));
	TEST_ASSERT_EQUAL_PTR(map, retrieved);
	TEST_ASSERT_EQUAL(1, cel_map_size(retrieved));

	cel_value_destroy(&key);
	cel_value_destroy(&map_value);
}

void test_map_with_int_keys(void)
{
	cel_map_t *map = cel_map_create(0);

	cel_value_t key1 = cel_value_int(1);
	cel_value_t key2 = cel_value_int(2);
	cel_value_t val1 = cel_value_string("one");
	cel_value_t val2 = cel_value_string("two");

	cel_map_put(map, &key1, &val1);
	cel_map_put(map, &key2, &val2);

	TEST_ASSERT_EQUAL(2, cel_map_size(map));
	TEST_ASSERT_TRUE(cel_map_contains(map, &key1));
	TEST_ASSERT_TRUE(cel_map_contains(map, &key2));

	cel_value_destroy(&val1);
	cel_value_destroy(&val2);
	cel_map_release(map);
}

void test_map_nested(void)
{
	/* 创建内层 map */
	cel_map_t *inner_map = cel_map_create(0);
	cel_value_t inner_key = cel_value_string("inner");
	cel_value_t inner_val = cel_value_int(100);
	cel_map_put(inner_map, &inner_key, &inner_val);

	/* 创建外层 map */
	cel_map_t *outer_map = cel_map_create(0);
	cel_value_t outer_key = cel_value_string("nested");
	cel_value_t inner_map_value = cel_value_map(inner_map);
	cel_map_put(outer_map, &outer_key, &inner_map_value);

	/* 释放原始引用 (put 已增加引用计数) */
	cel_map_release(inner_map);

	TEST_ASSERT_EQUAL(1, cel_map_size(outer_map));

	cel_value_t *retrieved = cel_map_get(outer_map, &outer_key);
	TEST_ASSERT_TRUE(cel_value_is_map(retrieved));

	cel_map_t *retrieved_map;
	cel_value_get_map(retrieved, &retrieved_map);
	TEST_ASSERT_EQUAL(1, cel_map_size(retrieved_map));

	cel_value_destroy(&inner_key);
	cel_value_destroy(&outer_key);
	cel_map_release(outer_map);
}

void test_map_equals(void)
{
	cel_map_t *map1 = cel_map_create(0);
	cel_map_t *map2 = cel_map_create(0);

	cel_value_t key1 = cel_value_string("a");
	cel_value_t val1 = cel_value_int(1);
	cel_value_t key2 = cel_value_string("b");
	cel_value_t val2 = cel_value_int(2);

	cel_map_put(map1, &key1, &val1);
	cel_map_put(map1, &key2, &val2);

	cel_map_put(map2, &key1, &val1);
	cel_map_put(map2, &key2, &val2);

	cel_value_t map_val1 = cel_value_map(map1);
	cel_value_t map_val2 = cel_value_map(map2);

	TEST_ASSERT_TRUE(cel_value_equals(&map_val1, &map_val2));

	/* 添加新键值对 */
	cel_value_t key3 = cel_value_string("c");
	cel_value_t val3 = cel_value_int(3);
	cel_map_put(map2, &key3, &val3);

	TEST_ASSERT_FALSE(cel_value_equals(&map_val1, &map_val2));

	cel_value_destroy(&key1);
	cel_value_destroy(&key2);
	cel_value_destroy(&key3);
	cel_value_destroy(&map_val1);
	cel_value_destroy(&map_val2);
}

/* ========== 边界条件测试 ========== */

void test_list_null_safety(void)
{
	TEST_ASSERT_EQUAL(0, cel_list_size(NULL));
	TEST_ASSERT_NULL(cel_list_get(NULL, 0));
	TEST_ASSERT_FALSE(cel_list_append(NULL, NULL));

	cel_list_release(NULL); /* 应该安全 */
	TEST_ASSERT_NULL(cel_list_retain(NULL));
}

void test_map_null_safety(void)
{
	TEST_ASSERT_EQUAL(0, cel_map_size(NULL));
	TEST_ASSERT_NULL(cel_map_get(NULL, NULL));
	TEST_ASSERT_FALSE(cel_map_put(NULL, NULL, NULL));
	TEST_ASSERT_FALSE(cel_map_contains(NULL, NULL));
	TEST_ASSERT_FALSE(cel_map_remove(NULL, NULL));

	cel_map_release(NULL); /* 应该安全 */
	TEST_ASSERT_NULL(cel_map_retain(NULL));
}

void test_list_auto_resize(void)
{
	cel_list_t *list = cel_list_create(2); /* 初始容量 2 */

	cel_value_t val = cel_value_int(1);

	/* 添加超过初始容量的元素 */
	for (int i = 0; i < 10; i++) {
		TEST_ASSERT_TRUE(cel_list_append(list, &val));
	}

	TEST_ASSERT_EQUAL(10, cel_list_size(list));
	TEST_ASSERT(list->capacity >= 10);

	cel_list_release(list);
}

/* ========== Unity 主函数 ========== */

int main(void)
{
	UNITY_BEGIN();

	/* 列表测试 */
	RUN_TEST(test_list_create_and_destroy);
	RUN_TEST(test_list_append);
	RUN_TEST(test_list_get);
	RUN_TEST(test_list_set);
	RUN_TEST(test_list_reference_counting);
	RUN_TEST(test_list_value_wrapper);
	RUN_TEST(test_list_with_mixed_types);
	RUN_TEST(test_list_nested);
	RUN_TEST(test_list_equals);

	/* 映射测试 */
	RUN_TEST(test_map_create_and_destroy);
	RUN_TEST(test_map_put_and_get);
	RUN_TEST(test_map_put_update);
	RUN_TEST(test_map_contains);
	RUN_TEST(test_map_remove);
	RUN_TEST(test_map_reference_counting);
	RUN_TEST(test_map_value_wrapper);
	RUN_TEST(test_map_with_int_keys);
	RUN_TEST(test_map_nested);
	RUN_TEST(test_map_equals);

	/* 边界条件测试 */
	RUN_TEST(test_list_null_safety);
	RUN_TEST(test_map_null_safety);
	RUN_TEST(test_list_auto_resize);

	return UNITY_END();
}
