/**
 * @file test_helpers.h
 * @brief CEL-C 测试辅助宏和工具
 *
 * 提供便捷的测试宏，简化测试代码编写。
 */

#ifndef CEL_TEST_HELPERS_H
#define CEL_TEST_HELPERS_H

#include "unity.h"
#include <math.h>
#include <string.h>

/* ========== 字符串测试宏 ========== */

/**
 * @brief 断言两个字符串相等 (支持 NULL)
 *
 * 如果两个都是 NULL，则认为相等。
 * 如果一个是 NULL 另一个不是，则失败。
 */
#define TEST_ASSERT_STRING_EQUAL_OR_NULL(expected, actual)                   \
	do {                                                                 \
		if ((expected) == NULL && (actual) == NULL) {                \
			/* 都是 NULL，通过 */                                   \
		} else if ((expected) == NULL || (actual) == NULL) {         \
			TEST_FAIL_MESSAGE("One string is NULL, other is not"); \
		} else {                                                     \
			TEST_ASSERT_EQUAL_STRING(expected, actual);          \
		}                                                            \
	} while (0)

/**
 * @brief 断言字符串包含子串
 */
#define TEST_ASSERT_STRING_CONTAINS(haystack, needle)                       \
	do {                                                                \
		TEST_ASSERT_NOT_NULL(haystack);                             \
		TEST_ASSERT_NOT_NULL(needle);                               \
		if (strstr(haystack, needle) == NULL) {                     \
			TEST_FAIL_MESSAGE("String does not contain substring"); \
		}                                                           \
	} while (0)

/**
 * @brief 断言字符串以指定前缀开头
 */
#define TEST_ASSERT_STRING_STARTS_WITH(str, prefix)                           \
	do {                                                                  \
		TEST_ASSERT_NOT_NULL(str);                                    \
		TEST_ASSERT_NOT_NULL(prefix);                                 \
		size_t prefix_len = strlen(prefix);                           \
		if (strncmp(str, prefix, prefix_len) != 0) {                  \
			TEST_FAIL_MESSAGE("String does not start with prefix"); \
		}                                                             \
	} while (0)

/* ========== 浮点数测试宏 ========== */

/**
 * @brief 断言两个浮点数近似相等 (相对误差)
 *
 * @param expected 期望值
 * @param actual 实际值
 * @param epsilon 相对误差阈值 (例如 0.001 表示 0.1%)
 */
#define TEST_ASSERT_DOUBLE_APPROX(expected, actual, epsilon)                   \
	do {                                                                   \
		double _exp = (double)(expected);                              \
		double _act = (double)(actual);                                \
		double _eps = (double)(epsilon);                               \
		if (fabs(_exp) < 1e-10) {                                      \
			/* 期望值接近 0，使用绝对误差 */                     \
			if (fabs(_act) > _eps) {                               \
				TEST_FAIL_MESSAGE("Double not approximately equal"); \
			}                                                      \
		} else {                                                       \
			/* 使用相对误差 */                                   \
			double _rel_err = fabs((_act - _exp) / _exp);         \
			if (_rel_err > _eps) {                                 \
				TEST_FAIL_MESSAGE("Double not approximately equal"); \
			}                                                      \
		}                                                              \
	} while (0)

/* ========== 内存测试宏 ========== */

/**
 * @brief 断言两块内存内容相等
 */
#define TEST_ASSERT_MEMORY_EQUAL(expected, actual, size)         \
	do {                                                     \
		TEST_ASSERT_NOT_NULL(expected);                  \
		TEST_ASSERT_NOT_NULL(actual);                    \
		if (memcmp(expected, actual, size) != 0) {       \
			TEST_FAIL_MESSAGE("Memory contents differ"); \
		}                                                \
	} while (0)

/**
 * @brief 断言内存块全为零
 */
#define TEST_ASSERT_MEMORY_ZERO(ptr, size)                    \
	do {                                                  \
		TEST_ASSERT_NOT_NULL(ptr);                    \
		const unsigned char *_p = (const unsigned char *)(ptr); \
		for (size_t _i = 0; _i < (size); _i++) {     \
			if (_p[_i] != 0) {                    \
				TEST_FAIL_MESSAGE("Memory not zero"); \
			}                                     \
		}                                             \
	} while (0)

/* ========== 指针测试宏 ========== */

/**
 * @brief 断言两个指针指向同一地址
 */
#define TEST_ASSERT_SAME_PTR(expected, actual) \
	TEST_ASSERT_EQUAL_PTR(expected, actual)

/**
 * @brief 断言两个指针不同
 */
#define TEST_ASSERT_DIFFERENT_PTR(ptr1, ptr2) \
	TEST_ASSERT_NOT_EQUAL(ptr1, ptr2)

/* ========== 范围测试宏 ========== */

/**
 * @brief 断言值在指定范围内 [min, max]
 */
#define TEST_ASSERT_IN_RANGE(value, min, max)                              \
	do {                                                               \
		if ((value) < (min) || (value) > (max)) {                  \
			TEST_FAIL_MESSAGE("Value not in expected range"); \
		}                                                          \
	} while (0)

/**
 * @brief 断言值不在指定范围内
 */
#define TEST_ASSERT_NOT_IN_RANGE(value, min, max)             \
	do {                                                  \
		if ((value) >= (min) && (value) <= (max)) {   \
			TEST_FAIL_MESSAGE("Value unexpectedly in range"); \
		}                                             \
	} while (0)

/* ========== 数组测试宏 ========== */

/**
 * @brief 断言数组的所有元素相等
 */
#define TEST_ASSERT_INT_ARRAY_EQUAL(expected, actual, count)      \
	do {                                                      \
		TEST_ASSERT_NOT_NULL(expected);                   \
		TEST_ASSERT_NOT_NULL(actual);                     \
		for (size_t _i = 0; _i < (count); _i++) {        \
			if ((expected)[_i] != (actual)[_i]) {     \
				TEST_FAIL_MESSAGE("Array elements differ"); \
			}                                         \
		}                                                 \
	} while (0)

/* ========== 布尔测试宏 ========== */

/**
 * @brief 断言条件为真 (带自定义消息)
 */
#define TEST_ASSERT_TRUE_MESSAGE(condition, message) \
	do {                                         \
		if (!(condition)) {                  \
			TEST_FAIL_MESSAGE(message);  \
		}                                    \
	} while (0)

/**
 * @brief 断言条件为假 (带自定义消息)
 */
#define TEST_ASSERT_FALSE_MESSAGE(condition, message) \
	do {                                          \
		if (condition) {                      \
			TEST_FAIL_MESSAGE(message);   \
		}                                     \
	} while (0)

/* ========== 错误测试宏 (CEL 特定) ========== */

#ifdef CEL_ERROR_H

/**
 * @brief 断言 Result 是成功的
 */
#define TEST_ASSERT_RESULT_OK(result)                             \
	do {                                                      \
		if (!(result).is_ok) {                            \
			if ((result).error) {                     \
				char _msg[256];                   \
				snprintf(_msg, sizeof(_msg),      \
					 "Result is error: %s",   \
					 (result).error->message); \
				TEST_FAIL_MESSAGE(_msg);          \
			} else {                                  \
				TEST_FAIL_MESSAGE("Result is error"); \
			}                                         \
		}                                                 \
	} while (0)

/**
 * @brief 断言 Result 是错误的
 */
#define TEST_ASSERT_RESULT_ERROR(result)                   \
	do {                                               \
		if ((result).is_ok) {                      \
			TEST_FAIL_MESSAGE("Result is OK"); \
		}                                          \
	} while (0)

/**
 * @brief 断言 Result 是指定错误码
 */
#define TEST_ASSERT_RESULT_ERROR_CODE(result, expected_code)              \
	do {                                                              \
		TEST_ASSERT_FALSE((result).is_ok);                        \
		TEST_ASSERT_NOT_NULL((result).error);                     \
		if ((result).error->code != (expected_code)) {            \
			TEST_FAIL_MESSAGE("Result has wrong error code"); \
		}                                                         \
	} while (0)

#endif /* CEL_ERROR_H */

#endif /* CEL_TEST_HELPERS_H */
