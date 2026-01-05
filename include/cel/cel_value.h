/**
 * @file cel_value.h
 * @brief CEL-C 基础值类型
 *
 * 实现 CEL 的基础值类型系统，包括:
 * - bool, int, uint, double
 * - string, bytes
 * - 引用计数管理
 */

#ifndef CEL_VALUE_H
#define CEL_VALUE_H

#include "cel/cel_error.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef CEL_THREAD_SAFE
#include <stdatomic.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 值类型枚举 ========== */

/**
 * @brief CEL 值类型
 */
typedef enum {
	CEL_TYPE_NULL = 0,    /* null 值 */
	CEL_TYPE_BOOL,        /* bool 值 */
	CEL_TYPE_INT,         /* int64 值 */
	CEL_TYPE_UINT,        /* uint64 值 */
	CEL_TYPE_DOUBLE,      /* double 值 */
	CEL_TYPE_STRING,      /* string 值 (引用计数) */
	CEL_TYPE_BYTES,       /* bytes 值 (引用计数) */
	CEL_TYPE_LIST,        /* list 值 (引用计数) */
	CEL_TYPE_MAP,         /* map 值 (引用计数) */
	CEL_TYPE_TIMESTAMP,   /* timestamp 值 */
	CEL_TYPE_DURATION,    /* duration 值 */
	CEL_TYPE_TYPE,        /* type 值 */
	CEL_TYPE_ERROR        /* error 值 */
} cel_type_e;

/* ========== 字符串类型 ========== */

/**
 * @brief CEL 字符串 (引用计数)
 */
typedef struct {
#ifdef CEL_THREAD_SAFE
	atomic_int ref_count;
#else
	int ref_count;
#endif
	size_t length;    /* 字符串长度 (字节数，不含 \0) */
	char data[];      /* 柔性数组 (以 \0 结尾) */
} cel_string_t;

/* ========== 字节数组类型 ========== */

/**
 * @brief CEL 字节数组 (引用计数)
 */
typedef struct {
#ifdef CEL_THREAD_SAFE
	atomic_int ref_count;
#else
	int ref_count;
#endif
	size_t length;         /* 字节数组长度 */
	unsigned char data[];  /* 柔性数组 */
} cel_bytes_t;

/* ========== 时间戳类型 ========== */

/**
 * @brief CEL 时间戳 (RFC3339)
 *
 * 表示带时区的时间点，精度到纳秒。
 * 例如: "2025-01-05T12:30:45+08:00"
 */
typedef struct {
	int64_t seconds;         /* 自 1970-01-01 00:00:00 UTC 的秒数 */
	int32_t nanoseconds;     /* 纳秒部分 (0-999999999) */
	int16_t offset_minutes;  /* UTC 偏移量 (分钟，-720 到 +840) */
} cel_timestamp_t;

/* ========== 时长类型 ========== */

/**
 * @brief CEL 时长
 *
 * 表示时间段，可以为负。精度到纳秒。
 * 例如: "1h30m45s" = 5445 秒
 */
typedef struct {
	int64_t seconds;      /* 秒数 (可为负) */
	int32_t nanoseconds;  /* 纳秒部分 (0-999999999，符号与 seconds 相同) */
} cel_duration_t;

/* ========== 容器类型 ========== */

/* 前向声明 */
struct cel_value;

/**
 * @brief CEL 列表 (动态数组，引用计数)
 *
 * 存储 cel_value_t 元素的动态数组。
 */
typedef struct {
#ifdef CEL_THREAD_SAFE
	atomic_int ref_count;
#else
	int ref_count;
#endif
	size_t length;              /* 元素数量 */
	size_t capacity;            /* 分配的容量 */
	struct cel_value **items;   /* cel_value_t 指针数组 */
} cel_list_t;

/**
 * @brief CEL 映射 (哈希表，引用计数)
 *
 * 存储键值对的哈希表。键可以是任意 CEL 值类型。
 */
typedef struct cel_map_entry {
	struct cel_value *key;      /* 键 (任意 CEL 值) */
	struct cel_value *value;    /* 值 (任意 CEL 值) */
	struct cel_map_entry *next; /* 哈希冲突链表 */
} cel_map_entry_t;

typedef struct {
#ifdef CEL_THREAD_SAFE
	atomic_int ref_count;
#else
	int ref_count;
#endif
	size_t size;                /* 键值对数量 */
	size_t bucket_count;        /* 桶数量 */
	cel_map_entry_t **buckets;  /* 哈希桶数组 */
} cel_map_t;

/* ========== 值联合体 ========== */

/**
 * @brief CEL 值结构
 */
typedef struct cel_value {
	cel_type_e type;  /* 值类型 */
	union {
		bool bool_value;
		int64_t int_value;
		uint64_t uint_value;
		double double_value;
		cel_string_t *string_value;       /* 指针，引用计数 */
		cel_bytes_t *bytes_value;         /* 指针，引用计数 */
		cel_timestamp_t timestamp_value;  /* 直接存储时间戳 (16 字节) */
		cel_duration_t duration_value;    /* 直接存储时长 (16 字节) */
		cel_list_t *list_value;           /* 指针，引用计数 */
		cel_map_t *map_value;             /* 指针，引用计数 */
		void *ptr_value;                  /* 泛型指针 (其他类型) */
	} value;
} cel_value_t;

/* ========== 值创建 API ========== */

/**
 * @brief 创建 null 值
 */
cel_value_t cel_value_null(void);

/**
 * @brief 创建 bool 值
 */
cel_value_t cel_value_bool(bool value);

/**
 * @brief 创建 int 值
 */
cel_value_t cel_value_int(int64_t value);

/**
 * @brief 创建 uint 值
 */
cel_value_t cel_value_uint(uint64_t value);

/**
 * @brief 创建 double 值
 */
cel_value_t cel_value_double(double value);

/**
 * @brief 创建 string 值 (复制字符串)
 *
 * @param str C 字符串 (以 \0 结尾)
 * @return 新创建的 string 值 (失败返回 null 值)
 */
cel_value_t cel_value_string(const char *str);

/**
 * @brief 创建 string 值 (指定长度)
 *
 * @param str 字符串数据
 * @param length 字符串长度 (字节数)
 * @return 新创建的 string 值 (失败返回 null 值)
 */
cel_value_t cel_value_string_n(const char *str, size_t length);

/**
 * @brief 创建 bytes 值 (复制字节数组)
 *
 * @param data 字节数组
 * @param length 字节数组长度
 * @return 新创建的 bytes 值 (失败返回 null 值)
 */
cel_value_t cel_value_bytes(const unsigned char *data, size_t length);

/**
 * @brief 创建 timestamp 值
 *
 * @param seconds 自 1970-01-01 00:00:00 UTC 的秒数
 * @param nanoseconds 纳秒部分 (0-999999999)
 * @param offset_minutes UTC 偏移量 (分钟，-720 到 +840)
 * @return 新创建的 timestamp 值
 */
cel_value_t cel_value_timestamp(int64_t seconds, int32_t nanoseconds,
				  int16_t offset_minutes);

/**
 * @brief 创建 duration 值
 *
 * @param seconds 秒数 (可为负)
 * @param nanoseconds 纳秒部分 (0-999999999)
 * @return 新创建的 duration 值
 */
cel_value_t cel_value_duration(int64_t seconds, int32_t nanoseconds);

/* ========== 值销毁 API ========== */

/**
 * @brief 销毁值 (减少引用计数)
 *
 * 对于引用计数类型 (string, bytes, list, map)，减少引用计数。
 * 当引用计数归零时，释放内存。
 *
 * @param value 要销毁的值
 */
void cel_value_destroy(cel_value_t *value);

/* ========== 值访问 API ========== */

/**
 * @brief 获取 bool 值
 *
 * @param value CEL 值
 * @param out 输出参数 (可选)
 * @return 成功返回 true，类型不匹配返回 false
 */
bool cel_value_get_bool(const cel_value_t *value, bool *out);

/**
 * @brief 获取 int 值
 */
bool cel_value_get_int(const cel_value_t *value, int64_t *out);

/**
 * @brief 获取 uint 值
 */
bool cel_value_get_uint(const cel_value_t *value, uint64_t *out);

/**
 * @brief 获取 double 值
 */
bool cel_value_get_double(const cel_value_t *value, double *out);

/**
 * @brief 获取 string 值 (返回内部指针，不需要释放)
 *
 * @param value CEL 值
 * @param out_str 输出字符串指针 (可选)
 * @param out_len 输出字符串长度 (可选)
 * @return 成功返回 true
 */
bool cel_value_get_string(const cel_value_t *value, const char **out_str,
			   size_t *out_len);

/**
 * @brief 获取 bytes 值 (返回内部指针，不需要释放)
 */
bool cel_value_get_bytes(const cel_value_t *value,
			  const unsigned char **out_data, size_t *out_len);

/**
 * @brief 获取 timestamp 值
 *
 * @param value CEL 值
 * @param out 输出 timestamp 指针 (可选)
 * @return 成功返回 true
 */
bool cel_value_get_timestamp(const cel_value_t *value, cel_timestamp_t *out);

/**
 * @brief 获取 duration 值
 *
 * @param value CEL 值
 * @param out 输出 duration 指针 (可选)
 * @return 成功返回 true
 */
bool cel_value_get_duration(const cel_value_t *value, cel_duration_t *out);

/* ========== 类型检查 API ========== */

/**
 * @brief 检查值是否为 null
 */
bool cel_value_is_null(const cel_value_t *value);

/**
 * @brief 检查值是否为 bool
 */
bool cel_value_is_bool(const cel_value_t *value);

/**
 * @brief 检查值是否为 int
 */
bool cel_value_is_int(const cel_value_t *value);

/**
 * @brief 检查值是否为 uint
 */
bool cel_value_is_uint(const cel_value_t *value);

/**
 * @brief 检查值是否为 double
 */
bool cel_value_is_double(const cel_value_t *value);

/**
 * @brief 检查值是否为 string
 */
bool cel_value_is_string(const cel_value_t *value);

/**
 * @brief 检查值是否为 bytes
 */
bool cel_value_is_bytes(const cel_value_t *value);

/**
 * @brief 检查值是否为 timestamp
 */
bool cel_value_is_timestamp(const cel_value_t *value);

/**
 * @brief 检查值是否为 duration
 */
bool cel_value_is_duration(const cel_value_t *value);

/**
 * @brief 获取值的类型
 */
cel_type_e cel_value_type(const cel_value_t *value);

/**
 * @brief 获取类型的字符串表示
 */
const char *cel_type_name(cel_type_e type);

/* ========== 值比较 API ========== */

/**
 * @brief 比较两个值是否相等
 *
 * @return true 表示相等，false 表示不相等或类型不同
 */
bool cel_value_equals(const cel_value_t *a, const cel_value_t *b);

/* ========== 字符串引用计数 API ========== */

/**
 * @brief 增加字符串引用计数
 */
cel_string_t *cel_string_retain(cel_string_t *str);

/**
 * @brief 减少字符串引用计数
 */
void cel_string_release(cel_string_t *str);

/**
 * @brief 创建字符串 (内部使用)
 */
cel_string_t *cel_string_create(const char *str, size_t length);

/* ========== 字节数组引用计数 API ========== */

/**
 * @brief 增加字节数组引用计数
 */
cel_bytes_t *cel_bytes_retain(cel_bytes_t *bytes);

/**
 * @brief 减少字节数组引用计数
 */
void cel_bytes_release(cel_bytes_t *bytes);

/**
 * @brief 创建字节数组 (内部使用)
 */
cel_bytes_t *cel_bytes_create(const unsigned char *data, size_t length);

/* ========== 列表 API ========== */

/**
 * @brief 创建空列表
 *
 * @param initial_capacity 初始容量 (0 表示使用默认容量)
 * @return 新创建的列表 (引用计数 = 1)，失败返回 NULL
 */
cel_list_t *cel_list_create(size_t initial_capacity);

/**
 * @brief 增加列表引用计数
 */
cel_list_t *cel_list_retain(cel_list_t *list);

/**
 * @brief 减少列表引用计数
 */
void cel_list_release(cel_list_t *list);

/**
 * @brief 向列表末尾添加元素
 *
 * @param list 列表
 * @param value 要添加的值 (会增加引用计数)
 * @return true 成功，false 失败
 */
bool cel_list_append(cel_list_t *list, cel_value_t *value);

/**
 * @brief 获取列表元素
 *
 * @param list 列表
 * @param index 索引 (0-based)
 * @return 元素指针，失败返回 NULL
 */
cel_value_t *cel_list_get(const cel_list_t *list, size_t index);

/**
 * @brief 设置列表元素
 *
 * @param list 列表
 * @param index 索引 (0-based)
 * @param value 新值 (会增加引用计数，旧值会减少引用计数)
 * @return true 成功，false 失败
 */
bool cel_list_set(cel_list_t *list, size_t index, cel_value_t *value);

/**
 * @brief 获取列表大小
 */
size_t cel_list_size(const cel_list_t *list);

/**
 * @brief 创建列表值
 *
 * @param list 列表 (所有权转移给 value)
 * @return 新创建的 list 值
 */
cel_value_t cel_value_list(cel_list_t *list);

/**
 * @brief 获取 list 值
 *
 * @param value CEL 值
 * @param out 输出 list 指针 (可选)
 * @return 成功返回 true
 */
bool cel_value_get_list(const cel_value_t *value, cel_list_t **out);

/**
 * @brief 检查值是否为 list
 */
bool cel_value_is_list(const cel_value_t *value);

/* ========== 映射 API ========== */

/**
 * @brief 创建空映射
 *
 * @param initial_bucket_count 初始桶数量 (0 表示使用默认值)
 * @return 新创建的映射 (引用计数 = 1)，失败返回 NULL
 */
cel_map_t *cel_map_create(size_t initial_bucket_count);

/**
 * @brief 增加映射引用计数
 */
cel_map_t *cel_map_retain(cel_map_t *map);

/**
 * @brief 减少映射引用计数
 */
void cel_map_release(cel_map_t *map);

/**
 * @brief 向映射中插入键值对
 *
 * @param map 映射
 * @param key 键 (会增加引用计数)
 * @param value 值 (会增加引用计数)
 * @return true 成功，false 失败
 */
bool cel_map_put(cel_map_t *map, cel_value_t *key, cel_value_t *value);

/**
 * @brief 从映射中获取值
 *
 * @param map 映射
 * @param key 键
 * @return 值指针，不存在返回 NULL
 */
cel_value_t *cel_map_get(const cel_map_t *map, const cel_value_t *key);

/**
 * @brief 检查映射是否包含键
 *
 * @param map 映射
 * @param key 键
 * @return true 包含，false 不包含
 */
bool cel_map_contains(const cel_map_t *map, const cel_value_t *key);

/**
 * @brief 从映射中删除键值对
 *
 * @param map 映射
 * @param key 键
 * @return true 成功删除，false 键不存在
 */
bool cel_map_remove(cel_map_t *map, const cel_value_t *key);

/**
 * @brief 获取映射大小
 */
size_t cel_map_size(const cel_map_t *map);

/**
 * @brief 创建映射值
 *
 * @param map 映射 (所有权转移给 value)
 * @return 新创建的 map 值
 */
cel_value_t cel_value_map(cel_map_t *map);

/**
 * @brief 获取 map 值
 *
 * @param value CEL 值
 * @param out 输出 map 指针 (可选)
 * @return 成功返回 true
 */
bool cel_value_get_map(const cel_value_t *value, cel_map_t **out);

/**
 * @brief 检查值是否为 map
 */
bool cel_value_is_map(const cel_value_t *value);

/* ========== 类型转换 API ========== */

/**
 * @brief 将值转换为 int (int64_t)
 *
 * 支持的转换:
 * - int -> int (直接返回)
 * - uint -> int (检查溢出)
 * - double -> int (截断小数)
 * - bool -> int (true=1, false=0)
 * - string -> int (解析十进制字符串)
 * - timestamp -> int (返回秒数)
 * - duration -> int (返回秒数)
 *
 * @param value 输入值
 * @param out 输出 int 值 (必须非 NULL)
 * @return true 转换成功, false 转换失败
 */
bool cel_value_to_int(const cel_value_t *value, int64_t *out);

/**
 * @brief 将值转换为 uint (uint64_t)
 *
 * 支持的转换:
 * - uint -> uint (直接返回)
 * - int -> uint (检查负数)
 * - double -> uint (检查负数，截断小数)
 * - bool -> uint (true=1, false=0)
 * - string -> uint (解析十进制无符号字符串)
 *
 * @param value 输入值
 * @param out 输出 uint 值 (必须非 NULL)
 * @return true 转换成功, false 转换失败
 */
bool cel_value_to_uint(const cel_value_t *value, uint64_t *out);

/**
 * @brief 将值转换为 double
 *
 * 支持的转换:
 * - double -> double (直接返回)
 * - int -> double
 * - uint -> double
 * - bool -> double (true=1.0, false=0.0)
 * - string -> double (解析浮点数字符串)
 *
 * @param value 输入值
 * @param out 输出 double 值 (必须非 NULL)
 * @return true 转换成功, false 转换失败
 */
bool cel_value_to_double(const cel_value_t *value, double *out);

/**
 * @brief 将值转换为 string
 *
 * 支持所有类型的转换:
 * - null -> "null"
 * - bool -> "true" / "false"
 * - int/uint/double -> 数字字符串
 * - string -> 原字符串 (复制)
 * - bytes -> base64 编码字符串
 * - list/map -> JSON 表示
 * - timestamp -> RFC3339 字符串
 * - duration -> 时长字符串 (如 "1h30m")
 *
 * @param value 输入值
 * @return 新创建的 string 值 (失败返回 null 值)
 */
cel_value_t cel_value_to_string(const cel_value_t *value);

/**
 * @brief 将字符串转换为 bytes
 *
 * 如果输入是字符串，复制其 UTF-8 字节；
 * 如果输入已经是 bytes，复制它。
 *
 * @param value 输入值
 * @return 新创建的 bytes 值 (失败返回 null 值)
 */
cel_value_t cel_value_to_bytes(const cel_value_t *value);

/* ========== 字符串操作 API ========== */

/**
 * @brief 检查字符串是否以指定前缀开头
 *
 * @param str 字符串值 (必须是 string 类型)
 * @param prefix 前缀字符串 (必须是 string 类型)
 * @param out 输出结果 (可选)
 * @return true 操作成功, false 类型不匹配
 */
bool cel_string_starts_with(const cel_value_t *str, const cel_value_t *prefix,
			     bool *out);

/**
 * @brief 检查字符串是否以指定后缀结尾
 *
 * @param str 字符串值 (必须是 string 类型)
 * @param suffix 后缀字符串 (必须是 string 类型)
 * @param out 输出结果 (可选)
 * @return true 操作成功, false 类型不匹配
 */
bool cel_string_ends_with(const cel_value_t *str, const cel_value_t *suffix,
			   bool *out);

/**
 * @brief 检查字符串是否包含子串
 *
 * @param str 字符串值 (必须是 string 类型)
 * @param substr 子串 (必须是 string 类型)
 * @param out 输出结果 (可选)
 * @return true 操作成功, false 类型不匹配
 */
bool cel_string_contains(const cel_value_t *str, const cel_value_t *substr,
			  bool *out);

/**
 * @brief 字符串连接
 *
 * @param a 字符串 A (必须是 string 类型)
 * @param b 字符串 B (必须是 string 类型)
 * @return 新创建的连接后的字符串 (失败返回 null 值)
 */
cel_value_t cel_string_concat(const cel_value_t *a, const cel_value_t *b);

/**
 * @brief 获取字符串长度
 *
 * @param str 字符串值
 * @return 字符串长度 (字节数), 非 string 类型返回 0
 */
size_t cel_string_length(const cel_value_t *str);

/* ========== 便捷宏 ========== */

/**
 * @brief 快速创建值的便捷宏
 */
#define CEL_NULL cel_value_null()
#define CEL_TRUE cel_value_bool(true)
#define CEL_FALSE cel_value_bool(false)
#define CEL_INT(x) cel_value_int(x)
#define CEL_UINT(x) cel_value_uint(x)
#define CEL_DOUBLE(x) cel_value_double(x)
#define CEL_STRING(x) cel_value_string(x)
#define CEL_TIMESTAMP(sec, nsec, offset) \
	cel_value_timestamp((sec), (nsec), (offset))
#define CEL_DURATION(sec, nsec) cel_value_duration((sec), (nsec))

#ifdef __cplusplus
}
#endif

#endif /* CEL_VALUE_H */
