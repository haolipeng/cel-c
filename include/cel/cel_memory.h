/**
 * @file cel_memory.h
 * @brief CEL-C 内存管理模块
 *
 * 提供 Arena 内存分配器，用于快速分配 AST 节点和临时对象。
 * 支持多块链接、对齐、重置功能。
 */

#ifndef CEL_MEMORY_H
#define CEL_MEMORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Arena 分配器 ========== */

/**
 * @brief Arena 分配器 (不透明指针)
 */
typedef struct arena_t arena_t;

/**
 * @brief 默认内存块大小 (4KB)
 */
#define ARENA_DEFAULT_BLOCK_SIZE (4 * 1024)

/**
 * @brief 内存对齐 (8 字节)
 */
#define ARENA_ALIGNMENT 8

/**
 * @brief 创建 Arena 分配器
 *
 * @param block_size 每个内存块的大小 (0 表示使用默认值)
 * @return 新创建的 Arena (失败返回 NULL)
 */
arena_t *arena_create(size_t block_size);

/**
 * @brief 从 Arena 分配内存
 *
 * 分配的内存保证对齐到 ARENA_ALIGNMENT。
 *
 * @param arena Arena 分配器
 * @param size 要分配的字节数
 * @return 分配的内存指针 (失败返回 NULL)
 */
void *arena_alloc(arena_t *arena, size_t size);

/**
 * @brief 重置 Arena (保留内存块，重置偏移量)
 *
 * 重置后可以重用已分配的内存块，无需重新分配。
 *
 * @param arena Arena 分配器
 */
void arena_reset(arena_t *arena);

/**
 * @brief 销毁 Arena (释放所有内存)
 *
 * @param arena Arena 分配器 (可以为 NULL)
 */
void arena_destroy(arena_t *arena);

/**
 * @brief 获取 Arena 的统计信息
 *
 * @param arena Arena 分配器
 * @param total_allocated 总分配字节数 (输出参数，可选)
 * @param total_used 已使用字节数 (输出参数，可选)
 * @param block_count 内存块数量 (输出参数，可选)
 */
void arena_stats(const arena_t *arena, size_t *total_allocated,
		 size_t *total_used, size_t *block_count);

/* ========== 便捷宏 ========== */

/**
 * @brief 从 Arena 分配类型化内存
 *
 * 示例:
 *   ast_node_t *node = ARENA_ALLOC(arena, ast_node_t);
 */
#define ARENA_ALLOC(arena, type) \
	((type *)arena_alloc((arena), sizeof(type)))

/**
 * @brief 从 Arena 分配数组
 *
 * 示例:
 *   int *array = ARENA_ALLOC_ARRAY(arena, int, 100);
 */
#define ARENA_ALLOC_ARRAY(arena, type, count) \
	((type *)arena_alloc((arena), sizeof(type) * (count)))

/* ========== 引用计数辅助宏 (单线程) ========== */

/**
 * @brief 引用计数增加 (单线程版本)
 *
 * @param obj 对象指针 (必须有 ref_count 字段)
 */
#define CEL_RETAIN(obj)              \
	do {                         \
		if (obj) {           \
			(obj)->ref_count++; \
		}                    \
	} while (0)

/**
 * @brief 引用计数减少并释放 (单线程版本)
 *
 * @param obj 对象指针 (必须有 ref_count 字段)
 * @param destroy_func 销毁函数
 */
#define CEL_RELEASE(obj, destroy_func)       \
	do {                                 \
		if (obj) {                   \
			(obj)->ref_count--;      \
			if ((obj)->ref_count == 0) { \
				destroy_func(obj);   \
			}                        \
		}                            \
	} while (0)

/* ========== 引用计数辅助宏 (多线程) ========== */

#ifdef CEL_THREAD_SAFE
#include <stdatomic.h>

/**
 * @brief 引用计数增加 (线程安全版本)
 *
 * @param obj 对象指针 (必须有 atomic_int ref_count 字段)
 */
#define CEL_ATOMIC_RETAIN(obj)                                    \
	do {                                                      \
		if (obj) {                                        \
			atomic_fetch_add(&(obj)->ref_count, 1); \
		}                                                 \
	} while (0)

/**
 * @brief 引用计数减少并释放 (线程安全版本)
 *
 * @param obj 对象指针 (必须有 atomic_int ref_count 字段)
 * @param destroy_func 销毁函数
 */
#define CEL_ATOMIC_RELEASE(obj, destroy_func)                          \
	do {                                                           \
		if (obj) {                                             \
			if (atomic_fetch_sub(&(obj)->ref_count, 1) == 1) { \
				destroy_func(obj);                     \
			}                                              \
		}                                                      \
	} while (0)

#endif /* CEL_THREAD_SAFE */

#ifdef __cplusplus
}
#endif

#endif /* CEL_MEMORY_H */
