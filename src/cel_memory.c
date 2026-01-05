/**
 * @file cel_memory.c
 * @brief CEL-C 内存管理模块实现
 */

#include "cel/cel_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========== 内部数据结构 ========== */

/**
 * @brief Arena 内存块
 */
typedef struct arena_block_t {
	size_t size;               /* 块大小 */
	size_t offset;             /* 当前偏移量 */
	struct arena_block_t *next; /* 下一个块 */
	char data[];               /* 柔性数组 (实际数据) */
} arena_block_t;

/**
 * @brief Arena 分配器结构
 */
struct arena_t {
	size_t block_size;      /* 每个块的默认大小 */
	arena_block_t *head;    /* 第一个块 */
	arena_block_t *current; /* 当前块 */
	size_t total_allocated; /* 总分配字节数 */
	size_t total_used;      /* 已使用字节数 */
	size_t block_count;     /* 内存块数量 */
};

/* ========== 内部辅助函数 ========== */

/**
 * @brief 对齐大小到指定边界
 */
static inline size_t align_size(size_t size, size_t alignment)
{
	return (size + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief 创建新内存块
 */
static arena_block_t *arena_block_create(size_t size)
{
	/* 分配块头 + 数据区域 */
	arena_block_t *block = (arena_block_t *)malloc(
		sizeof(arena_block_t) + size);
	if (!block) {
		return NULL;
	}

	block->size = size;
	block->offset = 0;
	block->next = NULL;

	return block;
}

/* ========== Arena 公共 API ========== */

arena_t *arena_create(size_t block_size)
{
	/* 使用默认块大小 */
	if (block_size == 0) {
		block_size = ARENA_DEFAULT_BLOCK_SIZE;
	}

	/* 分配 Arena 结构 */
	arena_t *arena = (arena_t *)malloc(sizeof(arena_t));
	if (!arena) {
		return NULL;
	}

	/* 创建第一个内存块 */
	arena->head = arena_block_create(block_size);
	if (!arena->head) {
		free(arena);
		return NULL;
	}

	arena->block_size = block_size;
	arena->current = arena->head;
	arena->total_allocated = block_size;
	arena->total_used = 0;
	arena->block_count = 1;

	return arena;
}

void *arena_alloc(arena_t *arena, size_t size)
{
	if (!arena || size == 0) {
		return NULL;
	}

	/* 对齐大小 */
	size = align_size(size, ARENA_ALIGNMENT);

	/* 检查当前块是否有足够空间 */
	arena_block_t *current = arena->current;
	if (current->offset + size <= current->size) {
		/* 从当前块分配 */
		void *ptr = current->data + current->offset;
		current->offset += size;
		arena->total_used += size;
		return ptr;
	}

	/* 当前块空间不足，需要新块 */

	/* 如果请求大小超过默认块大小，分配更大的块 */
	size_t new_block_size = (size > arena->block_size) ?
					align_size(size, 1024) :
					arena->block_size;

	/* 创建新块 */
	arena_block_t *new_block = arena_block_create(new_block_size);
	if (!new_block) {
		return NULL;
	}

	/* 链接新块 */
	current->next = new_block;
	arena->current = new_block;
	arena->total_allocated += new_block_size;
	arena->block_count++;

	/* 从新块分配 */
	void *ptr = new_block->data;
	new_block->offset = size;
	arena->total_used += size;

	return ptr;
}

void arena_reset(arena_t *arena)
{
	if (!arena) {
		return;
	}

	/* 重置所有块的偏移量 */
	for (arena_block_t *block = arena->head; block; block = block->next) {
		block->offset = 0;
	}

	/* 重置当前块为第一个块 */
	arena->current = arena->head;
	arena->total_used = 0;
}

void arena_destroy(arena_t *arena)
{
	if (!arena) {
		return;
	}

	/* 释放所有内存块 */
	arena_block_t *block = arena->head;
	while (block) {
		arena_block_t *next = block->next;
		free(block);
		block = next;
	}

	/* 释放 Arena 结构 */
	free(arena);
}

void arena_stats(const arena_t *arena, size_t *total_allocated,
		 size_t *total_used, size_t *block_count)
{
	if (!arena) {
		return;
	}

	if (total_allocated) {
		*total_allocated = arena->total_allocated;
	}

	if (total_used) {
		*total_used = arena->total_used;
	}

	if (block_count) {
		*block_count = arena->block_count;
	}
}
