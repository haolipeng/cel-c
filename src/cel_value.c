/**
 * @file cel_value.c
 * @brief CEL-C 基础值类型实现
 */

#define _POSIX_C_SOURCE 200809L

#include "cel/cel_value.h"
#include "cel/cel_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

cel_value_t cel_value_timestamp(int64_t seconds, int32_t nanoseconds,
				  int16_t offset_minutes)
{
	cel_value_t value;
	value.type = CEL_TYPE_TIMESTAMP;
	value.value.timestamp_value.seconds = seconds;
	value.value.timestamp_value.nanoseconds = nanoseconds;
	value.value.timestamp_value.offset_minutes = offset_minutes;
	return value;
}

cel_value_t cel_value_duration(int64_t seconds, int32_t nanoseconds)
{
	cel_value_t value;
	value.type = CEL_TYPE_DURATION;
	value.value.duration_value.seconds = seconds;
	value.value.duration_value.nanoseconds = nanoseconds;
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
		cel_list_release(value->value.list_value);
		break;

	case CEL_TYPE_MAP:
		cel_map_release(value->value.map_value);
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

bool cel_value_get_timestamp(const cel_value_t *value, cel_timestamp_t *out)
{
	if (!value || value->type != CEL_TYPE_TIMESTAMP) {
		return false;
	}

	if (out) {
		*out = value->value.timestamp_value;
	}
	return true;
}

bool cel_value_get_duration(const cel_value_t *value, cel_duration_t *out)
{
	if (!value || value->type != CEL_TYPE_DURATION) {
		return false;
	}

	if (out) {
		*out = value->value.duration_value;
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

bool cel_value_is_timestamp(const cel_value_t *value)
{
	return value && value->type == CEL_TYPE_TIMESTAMP;
}

bool cel_value_is_duration(const cel_value_t *value)
{
	return value && value->type == CEL_TYPE_DURATION;
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

	case CEL_TYPE_TIMESTAMP: {
		const cel_timestamp_t *ts_a = &a->value.timestamp_value;
		const cel_timestamp_t *ts_b = &b->value.timestamp_value;

		return ts_a->seconds == ts_b->seconds &&
		       ts_a->nanoseconds == ts_b->nanoseconds &&
		       ts_a->offset_minutes == ts_b->offset_minutes;
	}

	case CEL_TYPE_DURATION: {
		const cel_duration_t *dur_a = &a->value.duration_value;
		const cel_duration_t *dur_b = &b->value.duration_value;

		return dur_a->seconds == dur_b->seconds &&
		       dur_a->nanoseconds == dur_b->nanoseconds;
	}

	case CEL_TYPE_LIST: {
		cel_list_t *list_a = a->value.list_value;
		cel_list_t *list_b = b->value.list_value;

		if (!list_a || !list_b) {
			return list_a == list_b;
		}

		if (list_a->length != list_b->length) {
			return false;
		}

		/* 比较每个元素 */
		for (size_t i = 0; i < list_a->length; i++) {
			if (!cel_value_equals(list_a->items[i],
					      list_b->items[i])) {
				return false;
			}
		}

		return true;
	}

	case CEL_TYPE_MAP: {
		cel_map_t *map_a = a->value.map_value;
		cel_map_t *map_b = b->value.map_value;

		if (!map_a || !map_b) {
			return map_a == map_b;
		}

		if (map_a->size != map_b->size) {
			return false;
		}

		/* 检查 map_a 的所有键在 map_b 中且值相等 */
		for (size_t i = 0; i < map_a->bucket_count; i++) {
			cel_map_entry_t *entry = map_a->buckets[i];
			while (entry) {
				cel_value_t *value_b =
					cel_map_get(map_b, entry->key);
				if (!value_b ||
				    !cel_value_equals(entry->value, value_b)) {
					return false;
				}
				entry = entry->next;
			}
		}

		return true;
	}

	default:
		/* 其他类型暂不支持 */
		return false;
	}
}

/* ========== 容器值 API ========== */

cel_value_t cel_value_list(cel_list_t *list)
{
	cel_value_t value;
	if (!list) {
		return cel_value_null();
	}

	value.type = CEL_TYPE_LIST;
	value.value.list_value = list;
	return value;
}

bool cel_value_get_list(const cel_value_t *value, cel_list_t **out)
{
	if (!value || value->type != CEL_TYPE_LIST) {
		return false;
	}

	if (out) {
		*out = value->value.list_value;
	}
	return true;
}

bool cel_value_is_list(const cel_value_t *value)
{
	return value && value->type == CEL_TYPE_LIST;
}

cel_value_t cel_value_map(cel_map_t *map)
{
	cel_value_t value;
	if (!map) {
		return cel_value_null();
	}

	value.type = CEL_TYPE_MAP;
	value.value.map_value = map;
	return value;
}

bool cel_value_get_map(const cel_value_t *value, cel_map_t **out)
{
	if (!value || value->type != CEL_TYPE_MAP) {
		return false;
	}

	if (out) {
		*out = value->value.map_value;
	}
	return true;
}

bool cel_value_is_map(const cel_value_t *value)
{
	return value && value->type == CEL_TYPE_MAP;
}

/* ========== 类型转换实现 ========== */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>

bool cel_value_to_int(const cel_value_t *value, int64_t *out)
{
	if (!value || !out) {
		return false;
	}

	switch (value->type) {
	case CEL_TYPE_INT:
		*out = value->value.int_value;
		return true;

	case CEL_TYPE_UINT:
		/* 检查 uint64 是否能安全转换为 int64 */
		if (value->value.uint_value > (uint64_t)INT64_MAX) {
			return false; /* 溢出 */
		}
		*out = (int64_t)value->value.uint_value;
		return true;

	case CEL_TYPE_DOUBLE:
		/* 截断小数部分 */
		if (value->value.double_value > (double)INT64_MAX ||
		    value->value.double_value < (double)INT64_MIN) {
			return false; /* 溢出 */
		}
		*out = (int64_t)value->value.double_value;
		return true;

	case CEL_TYPE_BOOL:
		*out = value->value.bool_value ? 1 : 0;
		return true;

	case CEL_TYPE_STRING: {
		cel_string_t *str = value->value.string_value;
		if (!str || str->length == 0) {
			return false;
		}

		/* 解析十进制字符串 */
		char *endptr;
		errno = 0;
		long long result = strtoll(str->data, &endptr, 10);

		/* 检查解析是否成功 */
		if (errno == ERANGE || endptr == str->data ||
		    *endptr != '\0') {
			return false;
		}

		*out = (int64_t)result;
		return true;
	}

	case CEL_TYPE_TIMESTAMP:
		*out = value->value.timestamp_value.seconds;
		return true;

	case CEL_TYPE_DURATION:
		*out = value->value.duration_value.seconds;
		return true;

	default:
		return false;
	}
}

bool cel_value_to_uint(const cel_value_t *value, uint64_t *out)
{
	if (!value || !out) {
		return false;
	}

	switch (value->type) {
	case CEL_TYPE_UINT:
		*out = value->value.uint_value;
		return true;

	case CEL_TYPE_INT:
		/* 检查是否为负数 */
		if (value->value.int_value < 0) {
			return false;
		}
		*out = (uint64_t)value->value.int_value;
		return true;

	case CEL_TYPE_DOUBLE:
		/* 检查是否为负数或溢出 */
		if (value->value.double_value < 0 ||
		    value->value.double_value > (double)UINT64_MAX) {
			return false;
		}
		*out = (uint64_t)value->value.double_value;
		return true;

	case CEL_TYPE_BOOL:
		*out = value->value.bool_value ? 1 : 0;
		return true;

	case CEL_TYPE_STRING: {
		cel_string_t *str = value->value.string_value;
		if (!str || str->length == 0) {
			return false;
		}

		/* 检查负号 */
		if (str->data[0] == '-') {
			return false;
		}

		/* 解析无符号十进制字符串 */
		char *endptr;
		errno = 0;
		unsigned long long result = strtoull(str->data, &endptr, 10);

		if (errno == ERANGE || endptr == str->data ||
		    *endptr != '\0') {
			return false;
		}

		*out = (uint64_t)result;
		return true;
	}

	default:
		return false;
	}
}

bool cel_value_to_double(const cel_value_t *value, double *out)
{
	if (!value || !out) {
		return false;
	}

	switch (value->type) {
	case CEL_TYPE_DOUBLE:
		*out = value->value.double_value;
		return true;

	case CEL_TYPE_INT:
		*out = (double)value->value.int_value;
		return true;

	case CEL_TYPE_UINT:
		*out = (double)value->value.uint_value;
		return true;

	case CEL_TYPE_BOOL:
		*out = value->value.bool_value ? 1.0 : 0.0;
		return true;

	case CEL_TYPE_STRING: {
		cel_string_t *str = value->value.string_value;
		if (!str || str->length == 0) {
			return false;
		}

		/* 解析浮点数字符串 */
		char *endptr;
		errno = 0;
		double result = strtod(str->data, &endptr);

		if (errno == ERANGE || endptr == str->data ||
		    *endptr != '\0') {
			return false;
		}

		*out = result;
		return true;
	}

	default:
		return false;
	}
}

cel_value_t cel_value_to_string(const cel_value_t *value)
{
	if (!value) {
		return cel_value_null();
	}

	char buffer[128];
	int len;

	switch (value->type) {
	case CEL_TYPE_NULL:
		return cel_value_string("null");

	case CEL_TYPE_BOOL:
		return cel_value_string(value->value.bool_value ? "true" :
								   "false");

	case CEL_TYPE_INT:
		len = snprintf(buffer, sizeof(buffer), "%lld",
			       (long long)value->value.int_value);
		if (len < 0 || len >= (int)sizeof(buffer)) {
			return cel_value_null();
		}
		return cel_value_string_n(buffer, len);

	case CEL_TYPE_UINT:
		len = snprintf(buffer, sizeof(buffer), "%llu",
			       (unsigned long long)value->value.uint_value);
		if (len < 0 || len >= (int)sizeof(buffer)) {
			return cel_value_null();
		}
		return cel_value_string_n(buffer, len);

	case CEL_TYPE_DOUBLE:
		len = snprintf(buffer, sizeof(buffer), "%.15g",
			       value->value.double_value);
		if (len < 0 || len >= (int)sizeof(buffer)) {
			return cel_value_null();
		}
		return cel_value_string_n(buffer, len);

	case CEL_TYPE_STRING: {
		/* 复制字符串 */
		cel_string_t *str = value->value.string_value;
		if (!str) {
			return cel_value_null();
		}
		return cel_value_string_n(str->data, str->length);
	}

	case CEL_TYPE_BYTES: {
		/* 简化实现: 返回十六进制表示 */
		cel_bytes_t *bytes = value->value.bytes_value;
		if (!bytes) {
			return cel_value_null();
		}

		/* 需要 2 个字符表示一个字节 + null 终止符 */
		size_t str_len = bytes->length * 2;
		char *hex_str = (char *)malloc(str_len + 1);
		if (!hex_str) {
			return cel_value_null();
		}

		static const char hex_chars[] = "0123456789abcdef";
		for (size_t i = 0; i < bytes->length; i++) {
			hex_str[i * 2] = hex_chars[bytes->data[i] >> 4];
			hex_str[i * 2 + 1] = hex_chars[bytes->data[i] & 0x0F];
		}
		hex_str[str_len] = '\0';

		cel_value_t result = cel_value_string_n(hex_str, str_len);
		free(hex_str);
		return result;
	}

	case CEL_TYPE_TIMESTAMP: {
		/* RFC3339 格式: 2025-01-05T12:30:45+08:00 */
		const cel_timestamp_t *ts = &value->value.timestamp_value;

		/* 转换为本地时间 */
		time_t t = (time_t)ts->seconds;
		struct tm tm;
		gmtime_r(&t, &tm);

		/* 应用时区偏移 */
		int offset_hours = ts->offset_minutes / 60;
		int offset_mins = abs(ts->offset_minutes % 60);

		len = snprintf(
			buffer, sizeof(buffer),
			"%04d-%02d-%02dT%02d:%02d:%02d%+03d:%02d",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, offset_hours,
			offset_mins);

		if (len < 0 || len >= (int)sizeof(buffer)) {
			return cel_value_null();
		}
		return cel_value_string_n(buffer, len);
	}

	case CEL_TYPE_DURATION: {
		/* 时长格式: 1h30m45s */
		const cel_duration_t *dur = &value->value.duration_value;
		int64_t total_secs = dur->seconds;

		if (total_secs < 0) {
			buffer[0] = '-';
			total_secs = -total_secs;
			len = 1;
		} else {
			len = 0;
		}

		int64_t hours = total_secs / 3600;
		int64_t mins = (total_secs % 3600) / 60;
		int64_t secs = total_secs % 60;

		int written = 0;
		if (hours > 0) {
			written = snprintf(buffer + len,
					   sizeof(buffer) - len, "%lldh",
					   (long long)hours);
			if (written < 0) {
				return cel_value_null();
			}
			len += written;
		}

		if (mins > 0) {
			written = snprintf(buffer + len,
					   sizeof(buffer) - len, "%lldm",
					   (long long)mins);
			if (written < 0) {
				return cel_value_null();
			}
			len += written;
		}

		if (secs > 0 || len == 0) {
			written = snprintf(buffer + len,
					   sizeof(buffer) - len, "%llds",
					   (long long)secs);
			if (written < 0) {
				return cel_value_null();
			}
			len += written;
		}

		return cel_value_string_n(buffer, len);
	}

	case CEL_TYPE_LIST:
		/* 简化实现: 返回 [list] */
		return cel_value_string("[list]");

	case CEL_TYPE_MAP:
		/* 简化实现: 返回 {map} */
		return cel_value_string("{map}");

	default:
		return cel_value_string("<unknown>");
	}
}

cel_value_t cel_value_to_bytes(const cel_value_t *value)
{
	if (!value) {
		return cel_value_null();
	}

	switch (value->type) {
	case CEL_TYPE_BYTES: {
		/* 复制字节数组 */
		cel_bytes_t *bytes = value->value.bytes_value;
		if (!bytes) {
			return cel_value_null();
		}
		return cel_value_bytes(bytes->data, bytes->length);
	}

	case CEL_TYPE_STRING: {
		/* 字符串的 UTF-8 字节 */
		cel_string_t *str = value->value.string_value;
		if (!str) {
			return cel_value_null();
		}
		return cel_value_bytes((const unsigned char *)str->data,
				       str->length);
	}

	default:
		return cel_value_null();
	}
}

/* ========== 字符串操作实现 ========== */

bool cel_string_starts_with(const cel_value_t *str, const cel_value_t *prefix,
			     bool *out)
{
	if (!str || !prefix || str->type != CEL_TYPE_STRING ||
	    prefix->type != CEL_TYPE_STRING) {
		return false;
	}

	cel_string_t *s = str->value.string_value;
	cel_string_t *p = prefix->value.string_value;

	if (!s || !p) {
		return false;
	}

	/* 检查长度 */
	bool result = false;
	if (s->length >= p->length) {
		result = (memcmp(s->data, p->data, p->length) == 0);
	}

	if (out) {
		*out = result;
	}
	return true;
}

bool cel_string_ends_with(const cel_value_t *str, const cel_value_t *suffix,
			   bool *out)
{
	if (!str || !suffix || str->type != CEL_TYPE_STRING ||
	    suffix->type != CEL_TYPE_STRING) {
		return false;
	}

	cel_string_t *s = str->value.string_value;
	cel_string_t *x = suffix->value.string_value;

	if (!s || !x) {
		return false;
	}

	/* 检查长度 */
	bool result = false;
	if (s->length >= x->length) {
		result = (memcmp(s->data + (s->length - x->length), x->data,
				 x->length) == 0);
	}

	if (out) {
		*out = result;
	}
	return true;
}

bool cel_string_contains(const cel_value_t *str, const cel_value_t *substr,
			  bool *out)
{
	if (!str || !substr || str->type != CEL_TYPE_STRING ||
	    substr->type != CEL_TYPE_STRING) {
		return false;
	}

	cel_string_t *s = str->value.string_value;
	cel_string_t *sub = substr->value.string_value;

	if (!s || !sub) {
		return false;
	}

	/* 空子串总是包含 */
	if (sub->length == 0) {
		if (out) {
			*out = true;
		}
		return true;
	}

	/* 子串比主串长，不可能包含 */
	if (sub->length > s->length) {
		if (out) {
			*out = false;
		}
		return true;
	}

	/* 使用简单的暴力查找算法 */
	bool result = false;
	for (size_t i = 0; i <= s->length - sub->length; i++) {
		if (memcmp(s->data + i, sub->data, sub->length) == 0) {
			result = true;
			break;
		}
	}

	if (out) {
		*out = result;
	}
	return true;
}

cel_value_t cel_string_concat(const cel_value_t *a, const cel_value_t *b)
{
	if (!a || !b || a->type != CEL_TYPE_STRING ||
	    b->type != CEL_TYPE_STRING) {
		return cel_value_null();
	}

	cel_string_t *str_a = a->value.string_value;
	cel_string_t *str_b = b->value.string_value;

	if (!str_a || !str_b) {
		return cel_value_null();
	}

	/* 分配新字符串 */
	size_t new_length = str_a->length + str_b->length;
	cel_string_t *result = (cel_string_t *)malloc(
		sizeof(cel_string_t) + new_length + 1);
	if (!result) {
		return cel_value_null();
	}

	result->ref_count = 1;
	result->length = new_length;

	/* 复制两个字符串 */
	if (str_a->length > 0) {
		memcpy(result->data, str_a->data, str_a->length);
	}
	if (str_b->length > 0) {
		memcpy(result->data + str_a->length, str_b->data, str_b->length);
	}
	result->data[new_length] = '\0';

	cel_value_t value;
	value.type = CEL_TYPE_STRING;
	value.value.string_value = result;
	return value;
}

size_t cel_string_length(const cel_value_t *str)
{
	if (!str || str->type != CEL_TYPE_STRING) {
		return 0;
	}

	cel_string_t *s = str->value.string_value;
	return s ? s->length : 0;
}

/* ========== JSON 转换实现 ========== */

#ifdef CEL_ENABLE_JSON
#include <cJSON.h>

static cJSON *cel_value_to_cjson(const cel_value_t *value);
static cel_value_t cjson_to_cel_value(const cJSON *json);

static cJSON *cel_value_to_cjson(const cel_value_t *value)
{
	if (!value) {
		return cJSON_CreateNull();
	}

	switch (value->type) {
	case CEL_TYPE_NULL:
		return cJSON_CreateNull();

	case CEL_TYPE_BOOL:
		return cJSON_CreateBool(value->value.bool_value);

	case CEL_TYPE_INT:
		return cJSON_CreateNumber((double)value->value.int_value);

	case CEL_TYPE_UINT:
		return cJSON_CreateNumber((double)value->value.uint_value);

	case CEL_TYPE_DOUBLE:
		return cJSON_CreateNumber(value->value.double_value);

	case CEL_TYPE_STRING:
		if (value->value.string_value) {
			return cJSON_CreateString(value->value.string_value->data);
		}
		return cJSON_CreateNull();

	case CEL_TYPE_LIST: {
		cJSON *arr = cJSON_CreateArray();
		if (!arr) return NULL;
		cel_list_t *list = value->value.list_value;
		if (list) {
			for (size_t i = 0; i < list->length; i++) {
				cel_value_t *item = cel_list_get(list, i);
				cJSON *json_item = cel_value_to_cjson(item);
				if (json_item) {
					cJSON_AddItemToArray(arr, json_item);
				}
			}
		}
		return arr;
	}

	case CEL_TYPE_MAP: {
		cJSON *obj = cJSON_CreateObject();
		if (!obj) return NULL;
		cel_map_t *map = value->value.map_value;
		if (map && map->buckets) {
			for (size_t i = 0; i < map->bucket_count; i++) {
				cel_map_entry_t *entry = map->buckets[i];
				while (entry) {
					cel_value_t *key = entry->key;
					cel_value_t *val = entry->value;
					if (key && key->type == CEL_TYPE_STRING && key->value.string_value) {
						cJSON *json_val = cel_value_to_cjson(val);
						if (json_val) {
							cJSON_AddItemToObject(obj, key->value.string_value->data, json_val);
						}
					}
					entry = entry->next;
				}
			}
		}
		return obj;
	}

	default:
		return cJSON_CreateNull();
	}
}

static cel_value_t cjson_to_cel_value(const cJSON *json)
{
	if (!json) {
		return cel_value_null();
	}

	if (cJSON_IsNull(json)) {
		return cel_value_null();
	}

	if (cJSON_IsBool(json)) {
		return cel_value_bool(cJSON_IsTrue(json));
	}

	if (cJSON_IsNumber(json)) {
		double d = json->valuedouble;
		if (d == (int64_t)d && d >= INT64_MIN && d <= INT64_MAX) {
			return cel_value_int((int64_t)d);
		}
		return cel_value_double(d);
	}

	if (cJSON_IsString(json)) {
		return cel_value_string(json->valuestring);
	}

	if (cJSON_IsArray(json)) {
		cel_list_t *list = cel_list_create(cJSON_GetArraySize(json));
		if (!list) {
			return cel_value_null();
		}
		cJSON *item;
		cJSON_ArrayForEach(item, json) {
			cel_value_t val = cjson_to_cel_value(item);
			cel_list_append(list, &val);
			cel_value_destroy(&val);
		}
		cel_value_t result = cel_value_list(list);
		return result;
	}

	if (cJSON_IsObject(json)) {
		cel_map_t *map = cel_map_create(cJSON_GetArraySize(json));
		if (!map) {
			return cel_value_null();
		}
		cJSON *item;
		cJSON_ArrayForEach(item, json) {
			cel_value_t key = cel_value_string(item->string);
			cel_value_t val = cjson_to_cel_value(item);
			cel_map_put(map, &key, &val);
			cel_value_destroy(&key);
			cel_value_destroy(&val);
		}
		cel_value_t result = cel_value_map(map);
		return result;
	}

	return cel_value_null();
}

char *cel_value_to_json(const cel_value_t *value)
{
	cJSON *json = cel_value_to_cjson(value);
	if (!json) {
		return NULL;
	}

	char *str = cJSON_PrintUnformatted(json);
	cJSON_Delete(json);
	return str;
}

cel_value_t cel_value_from_json(const char *json_str)
{
	if (!json_str) {
		return cel_value_null();
	}

	cJSON *json = cJSON_Parse(json_str);
	if (!json) {
		return cel_value_null();
	}

	cel_value_t result = cjson_to_cel_value(json);
	cJSON_Delete(json);
	return result;
}

#endif /* CEL_ENABLE_JSON */