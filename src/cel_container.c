/**
 * @file cel_container.c
 * @brief CEL-C 容器类型实现 (list, map)
 */

#include "cel/cel_value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========== 常量定义 ========== */

#define CEL_LIST_DEFAULT_CAPACITY 8
#define CEL_MAP_DEFAULT_BUCKET_COUNT 16
#define CEL_MAP_LOAD_FACTOR 0.75

/* ========== 辅助函数 ========== */

/**
 * @brief 计算 CEL 值的哈希值
 */
static size_t cel_value_hash(const cel_value_t *value)
{
	if (!value) {
		return 0;
	}

	size_t hash = 0;

	switch (value->type) {
	case CEL_TYPE_NULL:
		return 0;

	case CEL_TYPE_BOOL:
		return value->value.bool_value ? 1 : 0;

	case CEL_TYPE_INT:
		hash = (size_t)value->value.int_value;
		return hash ^ (hash >> 32);

	case CEL_TYPE_UINT:
		hash = (size_t)value->value.uint_value;
		return hash ^ (hash >> 32);

	case CEL_TYPE_DOUBLE: {
		/* 使用 memcpy 避免类型双关 */
		uint64_t bits;
		memcpy(&bits, &value->value.double_value, sizeof(double));
		hash = (size_t)bits;
		return hash ^ (hash >> 32);
	}

	case CEL_TYPE_STRING: {
		cel_string_t *str = value->value.string_value;
		if (!str) {
			return 0;
		}
		/* FNV-1a 哈希算法 */
		hash = 2166136261u;
		for (size_t i = 0; i < str->length; i++) {
			hash ^= (unsigned char)str->data[i];
			hash *= 16777619u;
		}
		return hash;
	}

	case CEL_TYPE_BYTES: {
		cel_bytes_t *bytes = value->value.bytes_value;
		if (!bytes) {
			return 0;
		}
		/* FNV-1a 哈希算法 */
		hash = 2166136261u;
		for (size_t i = 0; i < bytes->length; i++) {
			hash ^= bytes->data[i];
			hash *= 16777619u;
		}
		return hash;
	}

	default:
		/* 其他类型使用类型标签作为哈希 */
		return (size_t)value->type;
	}
}

/* ========== 列表实现 ========== */

cel_list_t *cel_list_create(size_t initial_capacity)
{
	cel_list_t *list = (cel_list_t *)malloc(sizeof(cel_list_t));
	if (!list) {
		return NULL;
	}

	if (initial_capacity == 0) {
		initial_capacity = CEL_LIST_DEFAULT_CAPACITY;
	}

	list->items =
		(cel_value_t **)malloc(initial_capacity * sizeof(cel_value_t *));
	if (!list->items) {
		free(list);
		return NULL;
	}

	list->ref_count = 1;
	list->length = 0;
	list->capacity = initial_capacity;

	return list;
}

cel_list_t *cel_list_retain(cel_list_t *list)
{
	if (!list) {
		return NULL;
	}

#ifdef CEL_THREAD_SAFE
	atomic_fetch_add(&list->ref_count, 1);
#else
	list->ref_count++;
#endif

	return list;
}

void cel_list_release(cel_list_t *list)
{
	if (!list) {
		return;
	}

#ifdef CEL_THREAD_SAFE
	if (atomic_fetch_sub(&list->ref_count, 1) != 1) {
		return;
	}
#else
	list->ref_count--;
	if (list->ref_count > 0) {
		return;
	}
#endif

	/* 释放所有元素 */
	for (size_t i = 0; i < list->length; i++) {
		if (list->items[i]) {
			cel_value_destroy(list->items[i]);
			free(list->items[i]);
		}
	}

	free(list->items);
	free(list);
}

bool cel_list_append(cel_list_t *list, cel_value_t *value)
{
	if (!list || !value) {
		return false;
	}

	/* 检查容量，需要时扩容 */
	if (list->length >= list->capacity) {
		size_t new_capacity = list->capacity * 2;
		cel_value_t **new_items = (cel_value_t **)realloc(
			list->items, new_capacity * sizeof(cel_value_t *));
		if (!new_items) {
			return false;
		}
		list->items = new_items;
		list->capacity = new_capacity;
	}

	/* 创建值的副本 */
	cel_value_t *copy = (cel_value_t *)malloc(sizeof(cel_value_t));
	if (!copy) {
		return false;
	}

	memcpy(copy, value, sizeof(cel_value_t));

	/* 增加引用计数 (对于引用类型) */
	switch (value->type) {
	case CEL_TYPE_STRING:
		cel_string_retain(value->value.string_value);
		break;
	case CEL_TYPE_BYTES:
		cel_bytes_retain(value->value.bytes_value);
		break;
	case CEL_TYPE_LIST:
		cel_list_retain(value->value.list_value);
		break;
	case CEL_TYPE_MAP:
		cel_map_retain(value->value.map_value);
		break;
	default:
		break;
	}

	list->items[list->length++] = copy;
	return true;
}

cel_value_t *cel_list_get(const cel_list_t *list, size_t index)
{
	if (!list || index >= list->length) {
		return NULL;
	}

	return list->items[index];
}

bool cel_list_set(cel_list_t *list, size_t index, cel_value_t *value)
{
	if (!list || !value || index >= list->length) {
		return false;
	}

	/* 释放旧值 */
	cel_value_t *old_value = list->items[index];
	if (old_value) {
		cel_value_destroy(old_value);
		free(old_value);
	}

	/* 创建新值的副本 */
	cel_value_t *copy = (cel_value_t *)malloc(sizeof(cel_value_t));
	if (!copy) {
		return false;
	}

	memcpy(copy, value, sizeof(cel_value_t));

	/* 增加引用计数 */
	switch (value->type) {
	case CEL_TYPE_STRING:
		cel_string_retain(value->value.string_value);
		break;
	case CEL_TYPE_BYTES:
		cel_bytes_retain(value->value.bytes_value);
		break;
	case CEL_TYPE_LIST:
		cel_list_retain(value->value.list_value);
		break;
	case CEL_TYPE_MAP:
		cel_map_retain(value->value.map_value);
		break;
	default:
		break;
	}

	list->items[index] = copy;
	return true;
}

size_t cel_list_size(const cel_list_t *list)
{
	return list ? list->length : 0;
}

/* ========== 映射实现 ========== */

cel_map_t *cel_map_create(size_t initial_bucket_count)
{
	cel_map_t *map = (cel_map_t *)malloc(sizeof(cel_map_t));
	if (!map) {
		return NULL;
	}

	if (initial_bucket_count == 0) {
		initial_bucket_count = CEL_MAP_DEFAULT_BUCKET_COUNT;
	}

	map->buckets = (cel_map_entry_t **)calloc(initial_bucket_count,
						   sizeof(cel_map_entry_t *));
	if (!map->buckets) {
		free(map);
		return NULL;
	}

	map->ref_count = 1;
	map->size = 0;
	map->bucket_count = initial_bucket_count;

	return map;
}

cel_map_t *cel_map_retain(cel_map_t *map)
{
	if (!map) {
		return NULL;
	}

#ifdef CEL_THREAD_SAFE
	atomic_fetch_add(&map->ref_count, 1);
#else
	map->ref_count++;
#endif

	return map;
}

void cel_map_release(cel_map_t *map)
{
	if (!map) {
		return;
	}

#ifdef CEL_THREAD_SAFE
	if (atomic_fetch_sub(&map->ref_count, 1) != 1) {
		return;
	}
#else
	map->ref_count--;
	if (map->ref_count > 0) {
		return;
	}
#endif

	/* 释放所有桶中的条目 */
	for (size_t i = 0; i < map->bucket_count; i++) {
		cel_map_entry_t *entry = map->buckets[i];
		while (entry) {
			cel_map_entry_t *next = entry->next;

			/* 释放键和值 */
			if (entry->key) {
				cel_value_destroy(entry->key);
				free(entry->key);
			}
			if (entry->value) {
				cel_value_destroy(entry->value);
				free(entry->value);
			}

			free(entry);
			entry = next;
		}
	}

	free(map->buckets);
	free(map);
}

bool cel_map_put(cel_map_t *map, cel_value_t *key, cel_value_t *value)
{
	if (!map || !key || !value) {
		return false;
	}

	/* 计算哈希值和桶索引 */
	size_t hash = cel_value_hash(key);
	size_t bucket_index = hash % map->bucket_count;

	/* 检查键是否已存在 */
	cel_map_entry_t *entry = map->buckets[bucket_index];
	while (entry) {
		if (cel_value_equals(entry->key, key)) {
			/* 键已存在，更新值 */
			cel_value_t *old_value = entry->value;
			if (old_value) {
				cel_value_destroy(old_value);
				free(old_value);
			}

			/* 创建新值的副本 */
			entry->value =
				(cel_value_t *)malloc(sizeof(cel_value_t));
			if (!entry->value) {
				return false;
			}
			memcpy(entry->value, value, sizeof(cel_value_t));

			/* 增加引用计数 */
			switch (value->type) {
			case CEL_TYPE_STRING:
				cel_string_retain(value->value.string_value);
				break;
			case CEL_TYPE_BYTES:
				cel_bytes_retain(value->value.bytes_value);
				break;
			case CEL_TYPE_LIST:
				cel_list_retain(value->value.list_value);
				break;
			case CEL_TYPE_MAP:
				cel_map_retain(value->value.map_value);
				break;
			default:
				break;
			}

			return true;
		}
		entry = entry->next;
	}

	/* 键不存在，创建新条目 */
	cel_map_entry_t *new_entry =
		(cel_map_entry_t *)malloc(sizeof(cel_map_entry_t));
	if (!new_entry) {
		return false;
	}

	/* 创建键的副本 */
	new_entry->key = (cel_value_t *)malloc(sizeof(cel_value_t));
	if (!new_entry->key) {
		free(new_entry);
		return false;
	}
	memcpy(new_entry->key, key, sizeof(cel_value_t));

	/* 增加键的引用计数 */
	switch (key->type) {
	case CEL_TYPE_STRING:
		cel_string_retain(key->value.string_value);
		break;
	case CEL_TYPE_BYTES:
		cel_bytes_retain(key->value.bytes_value);
		break;
	case CEL_TYPE_LIST:
		cel_list_retain(key->value.list_value);
		break;
	case CEL_TYPE_MAP:
		cel_map_retain(key->value.map_value);
		break;
	default:
		break;
	}

	/* 创建值的副本 */
	new_entry->value = (cel_value_t *)malloc(sizeof(cel_value_t));
	if (!new_entry->value) {
		cel_value_destroy(new_entry->key);
		free(new_entry->key);
		free(new_entry);
		return false;
	}
	memcpy(new_entry->value, value, sizeof(cel_value_t));

	/* 增加值的引用计数 */
	switch (value->type) {
	case CEL_TYPE_STRING:
		cel_string_retain(value->value.string_value);
		break;
	case CEL_TYPE_BYTES:
		cel_bytes_retain(value->value.bytes_value);
		break;
	case CEL_TYPE_LIST:
		cel_list_retain(value->value.list_value);
		break;
	case CEL_TYPE_MAP:
		cel_map_retain(value->value.map_value);
		break;
	default:
		break;
	}

	/* 插入到链表头部 */
	new_entry->next = map->buckets[bucket_index];
	map->buckets[bucket_index] = new_entry;
	map->size++;

	return true;
}

cel_value_t *cel_map_get(const cel_map_t *map, const cel_value_t *key)
{
	if (!map || !key) {
		return NULL;
	}

	size_t hash = cel_value_hash(key);
	size_t bucket_index = hash % map->bucket_count;

	cel_map_entry_t *entry = map->buckets[bucket_index];
	while (entry) {
		if (cel_value_equals(entry->key, key)) {
			return entry->value;
		}
		entry = entry->next;
	}

	return NULL;
}

bool cel_map_contains(const cel_map_t *map, const cel_value_t *key)
{
	return cel_map_get(map, key) != NULL;
}

bool cel_map_remove(cel_map_t *map, const cel_value_t *key)
{
	if (!map || !key) {
		return false;
	}

	size_t hash = cel_value_hash(key);
	size_t bucket_index = hash % map->bucket_count;

	cel_map_entry_t *entry = map->buckets[bucket_index];
	cel_map_entry_t *prev = NULL;

	while (entry) {
		if (cel_value_equals(entry->key, key)) {
			/* 找到要删除的条目 */
			if (prev) {
				prev->next = entry->next;
			} else {
				map->buckets[bucket_index] = entry->next;
			}

			/* 释放键和值 */
			if (entry->key) {
				cel_value_destroy(entry->key);
				free(entry->key);
			}
			if (entry->value) {
				cel_value_destroy(entry->value);
				free(entry->value);
			}

			free(entry);
			map->size--;
			return true;
		}

		prev = entry;
		entry = entry->next;
	}

	return false;
}

size_t cel_map_size(const cel_map_t *map)
{
	return map ? map->size : 0;
}
