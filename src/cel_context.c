/**
 * @file cel_context.c
 * @brief CEL 执行上下文实现
 */

#include "cel/cel_context.h"
#include "uthash/uthash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========== 辅助函数 - 引用计数 ========== */

/**
 * @brief 增加 cel_value_t 的引用计数
 *
 * 对于引用计数类型 (string, bytes, list, map)，增加引用计数。
 * 对于值类型 (bool, int, etc.)，不做处理。
 */
static void cel_value_retain_internal(cel_value_t *value)
{
	if (!value) {
		return;
	}

	switch (value->type) {
	case CEL_TYPE_STRING:
		if (value->value.string_value) {
#ifdef CEL_THREAD_SAFE
			atomic_fetch_add(&value->value.string_value->ref_count,
					 1);
#else
			value->value.string_value->ref_count++;
#endif
		}
		break;
	case CEL_TYPE_BYTES:
		if (value->value.bytes_value) {
#ifdef CEL_THREAD_SAFE
			atomic_fetch_add(&value->value.bytes_value->ref_count,
					 1);
#else
			value->value.bytes_value->ref_count++;
#endif
		}
		break;
	case CEL_TYPE_LIST:
		if (value->value.list_value) {
#ifdef CEL_THREAD_SAFE
			atomic_fetch_add(&value->value.list_value->ref_count,
					 1);
#else
			value->value.list_value->ref_count++;
#endif
		}
		break;
	case CEL_TYPE_MAP:
		if (value->value.map_value) {
#ifdef CEL_THREAD_SAFE
			atomic_fetch_add(&value->value.map_value->ref_count,
					 1);
#else
			value->value.map_value->ref_count++;
#endif
		}
		break;
	default:
		/* 值类型不需要引用计数 */
		break;
	}
}

/**
 * @brief 减少 cel_value_t 的引用计数并在必要时释放
 */
static void cel_value_release_internal(cel_value_t *value)
{
	if (!value) {
		return;
	}

	switch (value->type) {
	case CEL_TYPE_STRING:
		if (value->value.string_value) {
			cel_string_release(value->value.string_value);
		}
		break;
	case CEL_TYPE_BYTES:
		if (value->value.bytes_value) {
			cel_bytes_release(value->value.bytes_value);
		}
		break;
	case CEL_TYPE_LIST:
		if (value->value.list_value) {
			cel_list_release(value->value.list_value);
		}
		break;
	case CEL_TYPE_MAP:
		if (value->value.map_value) {
			cel_map_release(value->value.map_value);
		}
		break;
	default:
		/* 值类型不需要释放 */
		break;
	}
}

/* ========== 内部结构 ========== */

/**
 * @brief 变量表项 (uthash)
 */
typedef struct {
	char *name;	     /* 键 (变量名) */
	cel_value_t *value;  /* 值 */
	UT_hash_handle hh;   /* uthash 句柄 */
} cel_variable_entry_t;

/**
 * @brief 函数注册表项 (uthash)
 */
typedef struct {
	char *name;	      /* 键 (函数名) */
	cel_function_t *func; /* 值 (函数元数据) */
	UT_hash_handle hh;    /* uthash 句柄 */
} cel_function_entry_t;

/**
 * @brief 执行上下文结构 (内部实现)
 */
struct cel_context {
	cel_context_t *parent; /* 父上下文 (作用域链) */

	/* 变量表 */
	cel_variable_entry_t *variables; /* uthash 哈希表 */

	/* 函数注册表 */
	cel_function_entry_t *functions; /* uthash 哈希表 */

	/* 自定义变量解析器 */
	cel_var_resolver_fn resolver;
	void *resolver_user_data;

	/* 配置 */
	size_t max_recursion_depth; /* 最大递归深度 */
	size_t current_depth;	    /* 当前递归深度 */
};

/* ========== 辅助函数 ========== */

static char *strdup_safe(const char *s)
{
	if (!s) {
		return NULL;
	}
	size_t len = strlen(s) + 1;
	char *dup = malloc(len);
	if (dup) {
		memcpy(dup, s, len);
	}
	return dup;
}

/* ========== 上下文管理 ========== */

cel_context_t *cel_context_create_empty(void)
{
	cel_context_t *ctx = calloc(1, sizeof(cel_context_t));
	if (!ctx) {
		return NULL;
	}

	ctx->parent = NULL;
	ctx->variables = NULL;
	ctx->functions = NULL;
	ctx->resolver = NULL;
	ctx->resolver_user_data = NULL;
	ctx->max_recursion_depth = 100;
	ctx->current_depth = 0;

	return ctx;
}

cel_context_t *cel_context_create(void)
{
	cel_context_t *ctx = cel_context_create_empty();
	if (!ctx) {
		return NULL;
	}

	/* TODO: 注册所有内置函数 */
	/* 这将在 Task 4.5 (内置函数库) 中实现 */

	return ctx;
}

cel_context_t *cel_context_create_child(cel_context_t *parent)
{
	if (!parent) {
		return NULL;
	}

	cel_context_t *ctx = cel_context_create_empty();
	if (!ctx) {
		return NULL;
	}

	ctx->parent = parent;
	ctx->max_recursion_depth = parent->max_recursion_depth;

	return ctx;
}

void cel_context_destroy(cel_context_t *ctx)
{
	if (!ctx) {
		return;
	}

	/* 销毁所有变量 */
	cel_variable_entry_t *var_entry, *var_tmp;
	HASH_ITER(hh, ctx->variables, var_entry, var_tmp)
	{
		HASH_DEL(ctx->variables, var_entry);
		free(var_entry->name);
		if (var_entry->value) {
			cel_value_release_internal(var_entry->value);
			free(var_entry->value);
		}
		free(var_entry);
	}

	/* 销毁所有函数 */
	cel_function_entry_t *func_entry, *func_tmp;
	HASH_ITER(hh, ctx->functions, func_entry, func_tmp)
	{
		HASH_DEL(ctx->functions, func_entry);
		free(func_entry->name);
		if (func_entry->func) {
			free(func_entry->func->name);
			free(func_entry->func->arg_types);
			free(func_entry->func);
		}
		free(func_entry);
	}

	free(ctx);
}

/* ========== 变量操作 ========== */

cel_error_code_e cel_context_add_variable(cel_context_t *ctx, const char *name,
					   cel_value_t *value)
{
	if (!ctx || !name || !value) {
		return CEL_ERROR_INVALID_ARGUMENT;
	}

	/* 查找是否已存在 */
	cel_variable_entry_t *entry = NULL;
	HASH_FIND_STR(ctx->variables, name, entry);

	if (entry) {
		/* 已存在,更新值 */
		if (entry->value) {
			cel_value_release_internal(entry->value);
			free(entry->value);
		}
		entry->value = malloc(sizeof(cel_value_t));
		if (!entry->value) {
			return CEL_ERROR_OUT_OF_MEMORY;
		}
		*entry->value = *value;
		cel_value_retain_internal(entry->value);
	} else {
		/* 不存在,创建新条目 */
		entry = malloc(sizeof(cel_variable_entry_t));
		if (!entry) {
			return CEL_ERROR_OUT_OF_MEMORY;
		}

		entry->name = strdup_safe(name);
		if (!entry->name) {
			free(entry);
			return CEL_ERROR_OUT_OF_MEMORY;
		}

		entry->value = malloc(sizeof(cel_value_t));
		if (!entry->value) {
			free(entry->name);
			free(entry);
			return CEL_ERROR_OUT_OF_MEMORY;
		}
		*entry->value = *value;
		cel_value_retain_internal(entry->value);
		HASH_ADD_KEYPTR(hh, ctx->variables, entry->name,
				strlen(entry->name), entry);
	}

	return CEL_OK;
}

cel_value_t *cel_context_get_variable(const cel_context_t *ctx,
				       const char *name)
{
	if (!ctx || !name) {
		return NULL;
	}

	/* 在当前上下文中查找 */
	cel_variable_entry_t *entry = NULL;
	HASH_FIND_STR(ctx->variables, name, entry);
	if (entry) {
		return entry->value;
	}

	/* 在父上下文中查找 */
	if (ctx->parent) {
		return cel_context_get_variable(ctx->parent, name);
	}

	/* 尝试使用 resolver */
	if (ctx->resolver) {
		return ctx->resolver(name, ctx->resolver_user_data);
	}

	return NULL;
}

bool cel_context_has_variable(const cel_context_t *ctx, const char *name)
{
	return cel_context_get_variable(ctx, name) != NULL;
}

bool cel_context_remove_variable(cel_context_t *ctx, const char *name)
{
	if (!ctx || !name) {
		return false;
	}

	cel_variable_entry_t *entry = NULL;
	HASH_FIND_STR(ctx->variables, name, entry);
	if (!entry) {
		return false;
	}

	HASH_DEL(ctx->variables, entry);
	free(entry->name);
	if (entry->value) {
		cel_value_release_internal(entry->value);
		free(entry->value);
	}
	free(entry);

	return true;
}

/* ========== 函数操作 ========== */

cel_error_code_e cel_context_add_function(cel_context_t *ctx, const char *name,
					   cel_function_fn func, size_t min_args,
					   size_t max_args)
{
	if (!ctx || !name || !func) {
		return CEL_ERROR_INVALID_ARGUMENT;
	}

	/* 创建函数元数据 */
	cel_function_t *func_meta = calloc(1, sizeof(cel_function_t));
	if (!func_meta) {
		return CEL_ERROR_OUT_OF_MEMORY;
	}

	func_meta->name = strdup_safe(name);
	if (!func_meta->name) {
		free(func_meta);
		return CEL_ERROR_OUT_OF_MEMORY;
	}

	func_meta->func = func;
	func_meta->min_args = min_args;
	func_meta->max_args = max_args;
	func_meta->arg_types = NULL;
	func_meta->return_type = CEL_TYPE_NULL;
	func_meta->user_data = NULL;

	cel_error_code_e result = cel_context_add_function_full(ctx, func_meta);

	/* 如果失败,释放我们创建的元数据 */
	if (result != CEL_OK) {
		free(func_meta->name);
		free(func_meta);
	}

	return result;
}

cel_error_code_e cel_context_add_function_full(cel_context_t *ctx,
						const cel_function_t *func)
{
	if (!ctx || !func || !func->name) {
		return CEL_ERROR_INVALID_ARGUMENT;
	}

	/* 查找是否已存在 */
	cel_function_entry_t *entry = NULL;
	HASH_FIND_STR(ctx->functions, func->name, entry);

	if (entry) {
		/* 已存在,更新函数 */
		if (entry->func) {
			free(entry->func->name);
			free(entry->func->arg_types);
			free(entry->func);
		}
		/* 复制函数元数据 */
		entry->func = malloc(sizeof(cel_function_t));
		if (!entry->func) {
			return CEL_ERROR_OUT_OF_MEMORY;
		}
		memcpy(entry->func, func, sizeof(cel_function_t));
		entry->func->name = strdup_safe(func->name);
	} else {
		/* 不存在,创建新条目 */
		entry = malloc(sizeof(cel_function_entry_t));
		if (!entry) {
			return CEL_ERROR_OUT_OF_MEMORY;
		}

		entry->name = strdup_safe(func->name);
		if (!entry->name) {
			free(entry);
			return CEL_ERROR_OUT_OF_MEMORY;
		}

		entry->func = malloc(sizeof(cel_function_t));
		if (!entry->func) {
			free(entry->name);
			free(entry);
			return CEL_ERROR_OUT_OF_MEMORY;
		}

		memcpy(entry->func, func, sizeof(cel_function_t));
		entry->func->name = strdup_safe(func->name);

		HASH_ADD_KEYPTR(hh, ctx->functions, entry->name,
				strlen(entry->name), entry);
	}

	return CEL_OK;
}

cel_function_t *cel_context_get_function(const cel_context_t *ctx,
					  const char *name)
{
	if (!ctx || !name) {
		return NULL;
	}

	/* 在当前上下文中查找 */
	cel_function_entry_t *entry = NULL;
	HASH_FIND_STR(ctx->functions, name, entry);
	if (entry) {
		return entry->func;
	}

	/* 在父上下文中查找 */
	if (ctx->parent) {
		return cel_context_get_function(ctx->parent, name);
	}

	return NULL;
}

bool cel_context_has_function(const cel_context_t *ctx, const char *name)
{
	return cel_context_get_function(ctx, name) != NULL;
}

bool cel_context_remove_function(cel_context_t *ctx, const char *name)
{
	if (!ctx || !name) {
		return false;
	}

	cel_function_entry_t *entry = NULL;
	HASH_FIND_STR(ctx->functions, name, entry);
	if (!entry) {
		return false;
	}

	HASH_DEL(ctx->functions, entry);
	free(entry->name);
	if (entry->func) {
		free(entry->func->name);
		free(entry->func->arg_types);
		free(entry->func);
	}
	free(entry);

	return true;
}

/* ========== 变量解析器 ========== */

void cel_context_set_resolver(cel_context_t *ctx, cel_var_resolver_fn resolver,
			       void *user_data)
{
	if (!ctx) {
		return;
	}

	ctx->resolver = resolver;
	ctx->resolver_user_data = user_data;
}

/* ========== 配置 API ========== */

void cel_context_set_max_recursion(cel_context_t *ctx, size_t max_depth)
{
	if (!ctx) {
		return;
	}

	ctx->max_recursion_depth = max_depth;
}

size_t cel_context_get_max_recursion(const cel_context_t *ctx)
{
	return ctx ? ctx->max_recursion_depth : 0;
}

size_t cel_context_get_current_depth(const cel_context_t *ctx)
{
	return ctx ? ctx->current_depth : 0;
}

cel_context_t *cel_context_get_parent(const cel_context_t *ctx)
{
	return ctx ? ctx->parent : NULL;
}
