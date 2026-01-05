/**
 * @file cel_error.h
 * @brief CEL-C 错误处理模块
 *
 * 提供统一的错误码定义、错误结构体和 Result 类型。
 * 支持错误传播宏 (CEL_TRY, CEL_UNWRAP)。
 */

#ifndef CEL_ERROR_H
#define CEL_ERROR_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CEL 错误码枚举
 */
typedef enum {
	CEL_OK = 0,                    /* 成功 */
	CEL_ERROR_SYNTAX,              /* 语法错误 */
	CEL_ERROR_PARSE,               /* 解析错误 */
	CEL_ERROR_TYPE_MISMATCH,       /* 类型不匹配 */
	CEL_ERROR_UNKNOWN_IDENTIFIER,  /* 未知标识符 */
	CEL_ERROR_DIVISION_BY_ZERO,    /* 除零错误 */
	CEL_ERROR_OUT_OF_RANGE,        /* 数值越界 */
	CEL_ERROR_OVERFLOW,            /* 溢出错误 */
	CEL_ERROR_NULL_POINTER,        /* 空指针错误 */
	CEL_ERROR_INVALID_ARGUMENT,    /* 无效参数 */
	CEL_ERROR_OUT_OF_MEMORY,       /* 内存不足 */
	CEL_ERROR_NOT_FOUND,           /* 未找到 */
	CEL_ERROR_ALREADY_EXISTS,      /* 已存在 */
	CEL_ERROR_UNSUPPORTED,         /* 不支持的操作 */
	CEL_ERROR_INTERNAL,            /* 内部错误 */
	CEL_ERROR_UNKNOWN              /* 未知错误 */
} cel_error_code_e;

/**
 * @brief CEL 错误结构体
 */
typedef struct {
	cel_error_code_e code;  /* 错误码 */
	char *message;          /* 错误消息 (动态分配) */
} cel_error_t;

/**
 * @brief CEL Result 类型 (泛型指针版本)
 *
 * 用于返回可能失败的操作结果。
 * - 成功时: is_ok = true, value = 结果值, error = NULL
 * - 失败时: is_ok = false, value = NULL, error = 错误对象
 */
typedef struct {
	bool is_ok;        /* 是否成功 */
	void *value;       /* 成功时的值 (调用者负责管理) */
	cel_error_t *error;  /* 失败时的错误 (需要释放) */
} cel_result_t;

/* ========== 错误对象管理 ========== */

/**
 * @brief 创建错误对象
 *
 * @param code 错误码
 * @param message 错误消息 (会被复制)
 * @return 新创建的错误对象 (调用者负责释放)
 */
cel_error_t *cel_error_create(cel_error_code_e code, const char *message);

/**
 * @brief 销毁错误对象
 *
 * @param error 要销毁的错误对象 (可以为 NULL)
 */
void cel_error_destroy(cel_error_t *error);

/**
 * @brief 获取错误码的字符串表示
 *
 * @param code 错误码
 * @return 错误码对应的字符串 (静态字符串，不需要释放)
 */
const char *cel_error_code_string(cel_error_code_e code);

/* ========== Result 类型管理 ========== */

/**
 * @brief 创建成功的 Result
 *
 * @param value 成功的值 (调用者负责管理生命周期)
 * @return Result 对象
 */
cel_result_t cel_ok_result(void *value);

/**
 * @brief 创建失败的 Result
 *
 * @param error 错误对象 (Result 会接管所有权)
 * @return Result 对象
 */
cel_result_t cel_error_result(cel_error_t *error);

/**
 * @brief 销毁 Result 对象
 *
 * 只释放 error，不释放 value (由调用者管理)
 *
 * @param result 要销毁的 Result 对象
 */
void cel_result_destroy(cel_result_t *result);

/* ========== 错误传播宏 ========== */

/**
 * @brief 错误传播宏 (用于 Result 类型)
 *
 * 如果 expr 返回的 Result 是错误，直接返回该错误。
 * 否则，将成功值赋给 var。
 *
 * 示例:
 *   cel_result_t result = some_function();
 *   CEL_TRY(value, result);  // 如果失败则返回错误，成功则 value 为结果
 */
#define CEL_TRY(var, expr)                           \
	do {                                         \
		cel_result_t _tmp_result = (expr);   \
		if (!_tmp_result.is_ok) {            \
			return _tmp_result;          \
		}                                    \
		(var) = _tmp_result.value;           \
	} while (0)

/**
 * @brief 强制解包 Result (断言成功)
 *
 * 如果 Result 是错误，程序会终止。
 * 仅用于确信不会失败的场景。
 *
 * 示例:
 *   cel_result_t result = some_function();
 *   void *value = CEL_UNWRAP(result);
 */
#define CEL_UNWRAP(expr)                                                      \
	({                                                                    \
		cel_result_t _tmp_result = (expr);                           \
		if (!_tmp_result.is_ok) {                                     \
			fprintf(stderr, "UNWRAP failed: %s at %s:%d\n",       \
				_tmp_result.error->message, __FILE__, __LINE__); \
			cel_error_destroy(_tmp_result.error);                 \
			abort();                                              \
		}                                                             \
		_tmp_result.value;                                            \
	})

/**
 * @brief 创建错误并返回 (便捷宏)
 *
 * 示例:
 *   if (ptr == NULL) {
 *       CEL_RETURN_ERROR(CEL_ERROR_NULL_POINTER, "Pointer is NULL");
 *   }
 */
#define CEL_RETURN_ERROR(code, msg) \
	return cel_error_result(cel_error_create((code), (msg)))

#ifdef __cplusplus
}
#endif

#endif /* CEL_ERROR_H */
