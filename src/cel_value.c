/**
 * @file cel_value.c
 * @brief CEL-C 基础值类型实现
 */

#include "cel/cel_value.h"
#include "cel/cel_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========== 字符串实现 ========== */

cel_string_t *cel_string_create(const char *str, size_t length)
{
	if (!str && length > 0) {
		return NULL;
	}

	/* 分配字符串结构 + 数据 + \0 */
	cel_string_t *string = (cel_string_t *)malloc(
		sizeof(cel_string_t) + length + 1);
	if (!string) {
		return NULL;
	}

	string->ref_count = 1;
	string->length = length;

	if (length > 0) {
		memcpy(string->data, str, length);
	}
	string->data[length] = '\0';

	return string;
}

cel_string_t *cel_string_retain(cel_string_t *str)
{
	if (!str) {
		return NULL;
	}

#ifdef CEL_THREAD_SAFE
	atomic_fetch_add(&str->ref_count, 1);
#else
	str->ref_count++;
#endif

	return str;
}

void cel_string_release(cel_string_t *str)
{
	if (!str) {
		return;
	}

#ifdef CEL_THREAD_SAFE
	if (atomic_fetch_sub(&str->ref_count, 1) == 1) {
		free(str);
	}
#else
	str->ref_count--;
	if (str->ref_count == 0) {
		free(str);
	}
#endif
}

/* ========== 字节数组实现 ========== */

cel_bytes_t *cel_bytes_create(const unsigned char *data, size_t length)
{
	if (!data && length > 0) {
		return NULL;
	}

	/* 分配字节数组结构 + 数据 */
	cel_bytes_t *bytes = (cel_bytes_t *)malloc(
		sizeof(cel_bytes_t) + length);
	if (!bytes) {
		return NULL;
	}

	bytes->ref_count = 1;
	bytes->length = length;

	if (length > 0) {
		memcpy(bytes->data, data, length);
	}

	return bytes;
}

cel_bytes_t *cel_bytes_retain(cel_bytes_t *bytes)
{
	if (!bytes) {
		return NULL;
	}

#ifdef CEL_THREAD_SAFE
	atomic_fetch_add(&bytes->ref_count, 1);
#else
	bytes->ref_count++;
#endif

	return bytes;
}

void cel_bytes_release(cel_bytes_t *bytes)
{
	if (!bytes) {
		return;
	}

#ifdef CEL_THREAD_SAFE
	if (atomic_fetch_sub(&bytes->ref_count, 1) == 1) {
		free(bytes);
	}
#else
	bytes->ref_count--;
	if (bytes->ref_count == 0) {
		free(bytes);
	}
#endif
}

/* ========== 值创建 API ========== */

cel_value_t cel_value_null(void)
{
	cel_value_t value;
	value.type = CEL_TYPE_NULL;
	value.value.ptr_value = NULL;
	return value;
}

cel_value_t cel_value_bool(bool val)
{
	cel_value_t value;
	value.type = CEL_TYPE_BOOL;
	value.value.bool_value = val;
	return value;
}

cel_value_t cel_value_int(int64_t val)
{
	cel_value_t value;
	value.type = CEL_TYPE_INT;
	value.value.int_value = val;
	return value;
}

cel_value_t cel_value_uint(uint64_t val)
{
	cel_value_t value;
	value.type = CEL_TYPE_UINT;
	value.value.uint_value = val;
	return value;
}

cel_value_t cel_value_double(double val)
{
	cel_value_t value;
	value.type = CEL_TYPE_DOUBLE;
	value.value.double_value = val;
	return value;
}

cel_value_t cel_value_string(const char *str)
{
	if (!str) {
		return cel_value_null();
	}

	return cel_value_string_n(str, strlen(str));
}

cel_value_t cel_value_string_n(const char *str, size_t length)
{
	cel_string_t *string = cel_string_create(str, length);
	if (!string) {
		return cel_value_null();
	}

	cel_value_t value;
	value.type = CEL_TYPE_STRING;
	value.value.string_value = string;
	return value;
}

cel_value_t cel_value_bytes(const unsigned char *data, size_t length)
{
	cel_bytes_t *bytes = cel_bytes_create(data, length);
	if (!bytes) {
		return cel_value_null();
	}

	cel_value_t value;
	value.type = CEL_TYPE_BYTES;
	value.value.bytes_value = bytes;
	return value;
}

/* ========== 值销毁 API ========== */

void cel_value_destroy(cel_value_t *value)
{
	if (!value) {
		return;
	}

	switch (value->type) {
	case CEL_TYPE_STRING:
		cel_string_release(value->value.string_value);
		break;

	case CEL_TYPE_BYTES:
		cel_bytes_release(value->value.bytes_value);
		break;

	case CEL_TYPE_LIST:
	case CEL_TYPE_MAP:
		/* TODO: 在 Task 2.3 实现 */
		break;

	default:
		/* 基本类型无需释放 */
		break;
	}

	/* 重置为 null */
	value->type = CEL_TYPE_NULL;
	value->value.ptr_value = NULL;
}

/* ========== 值访问 API ========== */

bool cel_value_get_bool(const cel_value_t *value, bool *out)
{
	if (!value || value->type != CEL_TYPE_BOOL) {
		return false;
	}

	if (out) {
		*out = value->value.bool_value;
	}
	return true;
}

bool cel_value_get_int(const cel_value_t *value, int64_t *out)
{
	if (!value || value->type != CEL_TYPE_INT) {
		return false;
	}

	if (out) {
		*out = value->value.int_value;
	}
	return true;
}

bool cel_value_get_uint(const cel_value_t *value, uint64_t *out)
{
	if (!value || value->type != CEL_TYPE_UINT) {
		return false;
	}

	if (out) {
		*out = value->value.uint_value;
	}
	return true;
}

bool cel_value_get_double(const cel_value_t *value, double *out)
{
	if (!value || value->type != CEL_TYPE_DOUBLE) {
		return false;
	}

	if (out) {
		*out = value->value.double_value;
	}
	return true;
}

bool cel_value_get_string(const cel_value_t *value, const char **out_str,
			   size_t *out_len)
{
	if (!value || value->type != CEL_TYPE_STRING) {
		return false;
	}

	cel_string_t *str = value->value.string_value;
	if (!str) {
		return false;
	}

	if (out_str) {
		*out_str = str->data;
	}
	if (out_len) {
		*out_len = str->length;
	}
	return true;
}

bool cel_value_get_bytes(const cel_value_t *value,
			  const unsigned char **out_data, size_t *out_len)
{
	if (!value || value->type != CEL_TYPE_BYTES) {
		return false;
	}

	cel_bytes_t *bytes = value->value.bytes_value;
	if (!bytes) {
		return false;
	}

	if (out_data) {
		*out_data = bytes->data;
	}
	if (out_len) {
		*out_len = bytes->length;
	}
	return true;
}

/* ========== 类型检查 API ========== */

bool cel_value_is_null(const cel_value_t *value)
{
	return value && value->type == CEL_TYPE_NULL;
}

bool cel_value_is_bool(const cel_value_t *value)
{
	return value && value->type == CEL_TYPE_BOOL;
}

bool cel_value_is_int(const cel_value_t *value)
{
	return value && value->type == CEL_TYPE_INT;
}

bool cel_value_is_uint(const cel_value_t *value)
{
	return value && value->type == CEL_TYPE_UINT;
}

bool cel_value_is_double(const cel_value_t *value)
{
	return value && value->type == CEL_TYPE_DOUBLE;
}

bool cel_value_is_string(const cel_value_t *value)
{
	return value && value->type == CEL_TYPE_STRING;
}

bool cel_value_is_bytes(const cel_value_t *value)
{
	return value && value->type == CEL_TYPE_BYTES;
}

cel_type_e cel_value_type(const cel_value_t *value)
{
	return value ? value->type : CEL_TYPE_NULL;
}

const char *cel_type_name(cel_type_e type)
{
	switch (type) {
	case CEL_TYPE_NULL:
		return "null";
	case CEL_TYPE_BOOL:
		return "bool";
	case CEL_TYPE_INT:
		return "int";
	case CEL_TYPE_UINT:
		return "uint";
	case CEL_TYPE_DOUBLE:
		return "double";
	case CEL_TYPE_STRING:
		return "string";
	case CEL_TYPE_BYTES:
		return "bytes";
	case CEL_TYPE_LIST:
		return "list";
	case CEL_TYPE_MAP:
		return "map";
	case CEL_TYPE_TIMESTAMP:
		return "timestamp";
	case CEL_TYPE_DURATION:
		return "duration";
	case CEL_TYPE_TYPE:
		return "type";
	case CEL_TYPE_ERROR:
		return "error";
	default:
		return "unknown";
	}
}

/* ========== 值比较 API ========== */

bool cel_value_equals(const cel_value_t *a, const cel_value_t *b)
{
	if (!a || !b) {
		return false;
	}

	/* 类型必须相同 */
	if (a->type != b->type) {
		return false;
	}

	switch (a->type) {
	case CEL_TYPE_NULL:
		return true;

	case CEL_TYPE_BOOL:
		return a->value.bool_value == b->value.bool_value;

	case CEL_TYPE_INT:
		return a->value.int_value == b->value.int_value;

	case CEL_TYPE_UINT:
		return a->value.uint_value == b->value.uint_value;

	case CEL_TYPE_DOUBLE:
		return a->value.double_value == b->value.double_value;

	case CEL_TYPE_STRING: {
		cel_string_t *str_a = a->value.string_value;
		cel_string_t *str_b = b->value.string_value;

		if (!str_a || !str_b) {
			return str_a == str_b;
		}

		if (str_a->length != str_b->length) {
			return false;
		}

		return memcmp(str_a->data, str_b->data, str_a->length) == 0;
	}

	case CEL_TYPE_BYTES: {
		cel_bytes_t *bytes_a = a->value.bytes_value;
		cel_bytes_t *bytes_b = b->value.bytes_value;

		if (!bytes_a || !bytes_b) {
			return bytes_a == bytes_b;
		}

		if (bytes_a->length != bytes_b->length) {
			return false;
		}

		return memcmp(bytes_a->data, bytes_b->data, bytes_a->length) == 0;
	}

	default:
		/* 其他类型暂不支持 */
		return false;
	}
}
