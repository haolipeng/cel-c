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

/* ========== 值联合体 ========== */

/**
 * @brief CEL 值结构
 */
typedef struct {
	cel_type_e type;  /* 值类型 */
	union {
		bool bool_value;
		int64_t int_value;
		uint64_t uint_value;
		double double_value;
		cel_string_t *string_value;  /* 指针，引用计数 */
		cel_bytes_t *bytes_value;    /* 指针，引用计数 */
		void *ptr_value;             /* 泛型指针 (list, map 等) */
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

#ifdef __cplusplus
}
#endif

#endif /* CEL_VALUE_H */
