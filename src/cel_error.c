/**
 * @file cel_error.c
 * @brief CEL-C 错误处理模块实现
 */

#include "cel/cel_error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========== 错误对象管理 ========== */

cel_error_t *cel_error_create(cel_error_code_e code, const char *message)
{
	cel_error_t *error = (cel_error_t *)malloc(sizeof(cel_error_t));
	if (!error) {
		return NULL;
	}

	error->code = code;

	/* 复制错误消息 */
	if (message) {
		error->message = strdup(message);
		if (!error->message) {
			free(error);
			return NULL;
		}
	} else {
		error->message = NULL;
	}

	return error;
}

void cel_error_destroy(cel_error_t *error)
{
	if (!error) {
		return;
	}

	free(error->message);
	free(error);
}

const char *cel_error_code_string(cel_error_code_e code)
{
	switch (code) {
	case CEL_OK:
		return "CEL_OK";
	case CEL_ERROR_SYNTAX:
		return "CEL_ERROR_SYNTAX";
	case CEL_ERROR_PARSE:
		return "CEL_ERROR_PARSE";
	case CEL_ERROR_TYPE_MISMATCH:
		return "CEL_ERROR_TYPE_MISMATCH";
	case CEL_ERROR_UNKNOWN_IDENTIFIER:
		return "CEL_ERROR_UNKNOWN_IDENTIFIER";
	case CEL_ERROR_DIVISION_BY_ZERO:
		return "CEL_ERROR_DIVISION_BY_ZERO";
	case CEL_ERROR_OUT_OF_RANGE:
		return "CEL_ERROR_OUT_OF_RANGE";
	case CEL_ERROR_OVERFLOW:
		return "CEL_ERROR_OVERFLOW";
	case CEL_ERROR_NULL_POINTER:
		return "CEL_ERROR_NULL_POINTER";
	case CEL_ERROR_INVALID_ARGUMENT:
		return "CEL_ERROR_INVALID_ARGUMENT";
	case CEL_ERROR_OUT_OF_MEMORY:
		return "CEL_ERROR_OUT_OF_MEMORY";
	case CEL_ERROR_NOT_FOUND:
		return "CEL_ERROR_NOT_FOUND";
	case CEL_ERROR_ALREADY_EXISTS:
		return "CEL_ERROR_ALREADY_EXISTS";
	case CEL_ERROR_UNSUPPORTED:
		return "CEL_ERROR_UNSUPPORTED";
	case CEL_ERROR_INTERNAL:
		return "CEL_ERROR_INTERNAL";
	case CEL_ERROR_UNKNOWN:
		return "CEL_ERROR_UNKNOWN";
	default:
		return "UNKNOWN_ERROR_CODE";
	}
}

/* ========== Result 类型管理 ========== */

cel_result_t cel_ok_result(void *value)
{
	cel_result_t result;
	result.is_ok = true;
	result.value = value;
	result.error = NULL;
	return result;
}

cel_result_t cel_error_result(cel_error_t *error)
{
	cel_result_t result;
	result.is_ok = false;
	result.value = NULL;
	result.error = error;
	return result;
}

void cel_result_destroy(cel_result_t *result)
{
	if (!result) {
		return;
	}

	/* 只释放 error，不释放 value (由调用者管理) */
	if (!result->is_ok && result->error) {
		cel_error_destroy(result->error);
		result->error = NULL;
	}
}
