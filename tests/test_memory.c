/**
 * @file test_memory.c
 * @brief CEL-C 内存管理模块单元测试
 */

#include "cel/cel_memory.h"
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

/* ========== Arena 基础测试 ========== */

void test_arena_create_and_destroy(void)
{
	arena_t *arena = arena_create(0); /* 使用默认大小 */

	TEST_ASSERT_NOT_NULL(arena);

	arena_destroy(arena);
}

void test_arena_destroy_null(void)
{
	/* 不应该崩溃 */
	arena_destroy(NULL);
}

void test_arena_simple_alloc(void)
{
	arena_t *arena = arena_create(1024);
	TEST_ASSERT_NOT_NULL(arena);

	/* 分配 100 字节 */
	void *ptr1 = arena_alloc(arena, 100);
	TEST_ASSERT_NOT_NULL(ptr1);

	/* 分配 200 字节 */
	void *ptr2 = arena_alloc(arena, 200);
	TEST_ASSERT_NOT_NULL(ptr2);

	/* 确保两个指针不同 */
	TEST_ASSERT_NOT_EQUAL(ptr1, ptr2);

	/* 写入数据验证可用性 */
	memset(ptr1, 0xAA, 100);
	memset(ptr2, 0xBB, 200);

	arena_destroy(arena);
}

void test_arena_alloc_zero_size(void)
{
	arena_t *arena = arena_create(1024);
	TEST_ASSERT_NOT_NULL(arena);

	/* 分配 0 字节应该返回 NULL */
	void *ptr = arena_alloc(arena, 0);
	TEST_ASSERT_NULL(ptr);

	arena_destroy(arena);
}

void test_arena_alloc_null_arena(void)
{
	/* 传入 NULL 应该返回 NULL */
	void *ptr = arena_alloc(NULL, 100);
	TEST_ASSERT_NULL(ptr);
}

/* ========== Arena 多块测试 ========== */

void test_arena_multiple_blocks(void)
{
	/* 创建小块 Arena (256 字节) */
	arena_t *arena = arena_create(256);
	TEST_ASSERT_NOT_NULL(arena);

	size_t block_count_before = 0;
	arena_stats(arena, NULL, NULL, &block_count_before);
	TEST_ASSERT_EQUAL(1, block_count_before);

	/* 分配 100 字节 x 3 = 300 字节 (超过一个块) */
	void *ptr1 = arena_alloc(arena, 100);
	void *ptr2 = arena_alloc(arena, 100);
	void *ptr3 = arena_alloc(arena, 100); /* 这个应该触发新块 */

	TEST_ASSERT_NOT_NULL(ptr1);
	TEST_ASSERT_NOT_NULL(ptr2);
	TEST_ASSERT_NOT_NULL(ptr3);

	/* 验证已经创建了多个块 */
	size_t block_count_after = 0;
	arena_stats(arena, NULL, NULL, &block_count_after);
	TEST_ASSERT_GREATER_THAN(1, block_count_after);

	arena_destroy(arena);
}

void test_arena_large_alloc(void)
{
	/* 创建小块 Arena */
	arena_t *arena = arena_create(256);
	TEST_ASSERT_NOT_NULL(arena);

	/* 分配超大块 (超过默认块大小) */
	void *ptr = arena_alloc(arena, 8192);
	TEST_ASSERT_NOT_NULL(ptr);

	/* 验证创建了新块 */
	size_t block_count = 0;
	arena_stats(arena, NULL, NULL, &block_count);
	TEST_ASSERT_EQUAL(2, block_count);

	arena_destroy(arena);
}

/* ========== Arena 重置测试 ========== */

void test_arena_reset(void)
{
	arena_t *arena = arena_create(1024);
	TEST_ASSERT_NOT_NULL(arena);

	/* 分配一些内存 */
	void *ptr1 = arena_alloc(arena, 100);
	void *ptr2 = arena_alloc(arena, 200);
	TEST_ASSERT_NOT_NULL(ptr1);
	TEST_ASSERT_NOT_NULL(ptr2);

	size_t used_before = 0;
	arena_stats(arena, NULL, &used_before, NULL);
	TEST_ASSERT_GREATER_THAN(0, used_before);

	/* 重置 Arena */
	arena_reset(arena);

	/* 验证已使用字节数归零 */
	size_t used_after = 0;
	arena_stats(arena, NULL, &used_after, NULL);
	TEST_ASSERT_EQUAL(0, used_after);

	/* 验证可以重新分配 */
	void *ptr3 = arena_alloc(arena, 100);
	TEST_ASSERT_NOT_NULL(ptr3);

	/* 重置后的第一次分配应该返回相同地址 */
	TEST_ASSERT_EQUAL(ptr1, ptr3);

	arena_destroy(arena);
}

void test_arena_reset_null(void)
{
	/* 不应该崩溃 */
	arena_reset(NULL);
}

/* ========== Arena 对齐测试 ========== */

void test_arena_alignment(void)
{
	arena_t *arena = arena_create(1024);
	TEST_ASSERT_NOT_NULL(arena);

	/* 分配多个小块，验证对齐 */
	for (int i = 0; i < 10; i++) {
		void *ptr = arena_alloc(arena, 1); /* 分配 1 字节 */
		TEST_ASSERT_NOT_NULL(ptr);

		/* 验证地址是 8 字节对齐 */
		uintptr_t addr = (uintptr_t)ptr;
		TEST_ASSERT_EQUAL(0, addr % ARENA_ALIGNMENT);
	}

	arena_destroy(arena);
}

/* ========== Arena 统计测试 ========== */

void test_arena_stats(void)
{
	arena_t *arena = arena_create(1024);
	TEST_ASSERT_NOT_NULL(arena);

	size_t total_allocated = 0;
	size_t total_used = 0;
	size_t block_count = 0;

	/* 初始状态 */
	arena_stats(arena, &total_allocated, &total_used, &block_count);
	TEST_ASSERT_EQUAL(1024, total_allocated);
	TEST_ASSERT_EQUAL(0, total_used);
	TEST_ASSERT_EQUAL(1, block_count);

	/* 分配一些内存 */
	arena_alloc(arena, 100);
	arena_alloc(arena, 200);

	arena_stats(arena, &total_allocated, &total_used, &block_count);
	TEST_ASSERT_EQUAL(1024, total_allocated);
	TEST_ASSERT_GREATER_THAN(0, total_used);
	TEST_ASSERT_EQUAL(1, block_count);

	arena_destroy(arena);
}

void test_arena_stats_null(void)
{
	/* 传入 NULL 不应该崩溃 */
	arena_stats(NULL, NULL, NULL, NULL);
}

/* ========== Arena 宏测试 ========== */

typedef struct {
	int x;
	int y;
} point_t;

void test_macro_arena_alloc(void)
{
	arena_t *arena = arena_create(1024);
	TEST_ASSERT_NOT_NULL(arena);

	/* 使用宏分配类型化内存 */
	point_t *p = ARENA_ALLOC(arena, point_t);
	TEST_ASSERT_NOT_NULL(p);

	/* 验证可以写入 */
	p->x = 10;
	p->y = 20;
	TEST_ASSERT_EQUAL(10, p->x);
	TEST_ASSERT_EQUAL(20, p->y);

	arena_destroy(arena);
}

void test_macro_arena_alloc_array(void)
{
	arena_t *arena = arena_create(1024);
	TEST_ASSERT_NOT_NULL(arena);

	/* 使用宏分配数组 */
	int *arr = ARENA_ALLOC_ARRAY(arena, int, 10);
	TEST_ASSERT_NOT_NULL(arr);

	/* 验证可以写入 */
	for (int i = 0; i < 10; i++) {
		arr[i] = i * 10;
	}

	for (int i = 0; i < 10; i++) {
		TEST_ASSERT_EQUAL(i * 10, arr[i]);
	}

	arena_destroy(arena);
}

/* ========== Unity 主函数 ========== */

int main(void)
{
	UNITY_BEGIN();

	/* 基础测试 */
	RUN_TEST(test_arena_create_and_destroy);
	RUN_TEST(test_arena_destroy_null);
	RUN_TEST(test_arena_simple_alloc);
	RUN_TEST(test_arena_alloc_zero_size);
	RUN_TEST(test_arena_alloc_null_arena);

	/* 多块测试 */
	RUN_TEST(test_arena_multiple_blocks);
	RUN_TEST(test_arena_large_alloc);

	/* 重置测试 */
	RUN_TEST(test_arena_reset);
	RUN_TEST(test_arena_reset_null);

	/* 对齐测试 */
	RUN_TEST(test_arena_alignment);

	/* 统计测试 */
	RUN_TEST(test_arena_stats);
	RUN_TEST(test_arena_stats_null);

	/* 宏测试 */
	RUN_TEST(test_macro_arena_alloc);
	RUN_TEST(test_macro_arena_alloc_array);

	return UNITY_END();
}
