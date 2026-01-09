/**
 * @file cel_context.h
 * @brief CEL 执行上下文
 *
 * 提供变量和函数的管理,支持作用域链。
 */

#ifndef CEL_CONTEXT_H
#define CEL_CONTEXT_H

#include "cel/cel_error.h"
#include "cel/cel_value.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 前向声明 */
typedef struct cel_context cel_context_t;
typedef struct cel_ast_node cel_ast_node_t;
typedef struct cel_function cel_function_t;
typedef struct cel_func_context cel_func_context_t;

/* ========== 函数类型定义 ========== */

/**
 * @brief 函数指针类型
 *
 * CEL 函数的实现接口。
 *
 * @param ctx 函数执行上下文
 * @param args 参数数组
 * @param arg_count 参数数量
 * @return 执行结果 (成功或错误)
 */
typedef cel_result_t (*cel_function_fn)(cel_func_context_t *ctx,
					 cel_value_t **args, size_t arg_count);

/**
 * @brief 变量解析器接口
 *
 * 用于动态解析变量值,当变量在上下文中不存在时调用。
 *
 * @param name 变量名
 * @param user_data 用户数据
 * @return 变量值,失败返回 NULL
 */
typedef cel_value_t *(*cel_var_resolver_fn)(const char *name, void *user_data);

/* ========== 函数元数据 ========== */

/**
 * @brief 函数元数据
 *
 * 描述一个 CEL 函数的所有信息。
 */
struct cel_function {
	char *name;		     /* 函数名 */
	cel_function_fn func;	     /* 函数指针 */
	size_t min_args;	     /* 最少参数数量 */
	size_t max_args;	     /* 最多参数数量 (SIZE_MAX = 可变参数) */
	cel_type_e *arg_types; /* 参数类型数组 (可选, NULL = 不检查) */
	cel_type_e return_type;/* 返回类型 */
	void *user_data;	     /* 用户数据 */
};

/**
 * @brief 函数执行上下文
 *
 * 传递给函数实现的上下文信息。
 */
struct cel_func_context {
	cel_context_t *context;	  /* 执行上下文 */
	const char *func_name;	  /* 当前函数名 */
	cel_ast_node_t *call_site; /* 调用点 AST 节点 (用于错误报告) */
};

/* ========== 上下文管理 API ========== */

/**
 * @brief 创建新的执行上下文
 *
 * 创建一个包含所有标准内置函数的上下文。
 *
 * @return 新创建的上下文,失败返回 NULL
 */
cel_context_t *cel_context_create(void);

/**
 * @brief 创建空的执行上下文
 *
 * 创建一个不包含任何内置函数的空上下文,用于自定义环境。
 *
 * @return 新创建的上下文,失败返回 NULL
 */
cel_context_t *cel_context_create_empty(void);

/**
 * @brief 创建子上下文
 *
 * 创建一个带有父上下文的子上下文,用于实现作用域链。
 * 子上下文可以访问父上下文的变量和函数。
 *
 * @param parent 父上下文
 * @return 新创建的子上下文,失败返回 NULL
 */
cel_context_t *cel_context_create_child(cel_context_t *parent);

/**
 * @brief 销毁执行上下文
 *
 * 释放上下文及其所有资源 (变量、函数等)。
 *
 * @param ctx 要销毁的上下文 (可以为 NULL)
 */
void cel_context_destroy(cel_context_t *ctx);

/* ========== 变量操作 API ========== */

/**
 * @brief 添加变量到上下文
 *
 * 将变量添加到当前上下文。如果变量已存在,将被覆盖。
 * 上下文会持有对 value 的引用 (不会复制)。
 *
 * @param ctx 执行上下文
 * @param name 变量名
 * @param value 变量值 (上下文会增加引用计数)
 * @return CEL_OK 成功,其他值表示错误
 */
cel_error_code_e cel_context_add_variable(cel_context_t *ctx, const char *name,
					   cel_value_t *value);

/**
 * @brief 从上下文获取变量
 *
 * 在当前上下文及其父上下文链中查找变量。
 * 如果未找到且设置了 resolver,会调用 resolver。
 *
 * @param ctx 执行上下文
 * @param name 变量名
 * @return 变量值,未找到返回 NULL
 */
cel_value_t *cel_context_get_variable(const cel_context_t *ctx,
				       const char *name);

/**
 * @brief 检查变量是否存在
 *
 * @param ctx 执行上下文
 * @param name 变量名
 * @return true 变量存在, false 不存在
 */
bool cel_context_has_variable(const cel_context_t *ctx, const char *name);

/**
 * @brief 移除变量
 *
 * 从当前上下文中移除指定变量 (不影响父上下文)。
 *
 * @param ctx 执行上下文
 * @param name 变量名
 * @return true 成功移除, false 变量不存在
 */
bool cel_context_remove_variable(cel_context_t *ctx, const char *name);

/* ========== 函数操作 API ========== */

/**
 * @brief 注册函数到上下文
 *
 * 将函数添加到当前上下文。如果函数已存在,将被覆盖。
 *
 * @param ctx 执行上下文
 * @param name 函数名
 * @param func 函数指针
 * @param min_args 最少参数数量
 * @param max_args 最多参数数量 (SIZE_MAX = 可变参数)
 * @return CEL_OK 成功,其他值表示错误
 */
cel_error_code_e cel_context_add_function(cel_context_t *ctx, const char *name,
					   cel_function_fn func, size_t min_args,
					   size_t max_args);

/**
 * @brief 注册带完整元数据的函数
 *
 * @param ctx 执行上下文
 * @param func 函数元数据 (上下文会复制一份)
 * @return CEL_OK 成功,其他值表示错误
 */
cel_error_code_e cel_context_add_function_full(cel_context_t *ctx,
						const cel_function_t *func);

/**
 * @brief 从上下文获取函数
 *
 * 在当前上下文及其父上下文链中查找函数。
 *
 * @param ctx 执行上下文
 * @param name 函数名
 * @return 函数元数据,未找到返回 NULL
 */
cel_function_t *cel_context_get_function(const cel_context_t *ctx,
					  const char *name);

/**
 * @brief 检查函数是否存在
 *
 * @param ctx 执行上下文
 * @param name 函数名
 * @return true 函数存在, false 不存在
 */
bool cel_context_has_function(const cel_context_t *ctx, const char *name);

/**
 * @brief 移除函数
 *
 * 从当前上下文中移除指定函数 (不影响父上下文)。
 *
 * @param ctx 执行上下文
 * @param name 函数名
 * @return true 成功移除, false 函数不存在
 */
bool cel_context_remove_function(cel_context_t *ctx, const char *name);

/* ========== 变量解析器 ========== */

/**
 * @brief 设置变量解析器
 *
 * 当变量在上下文中不存在时,会调用 resolver 尝试动态解析。
 *
 * @param ctx 执行上下文
 * @param resolver 解析器函数
 * @param user_data 传递给解析器的用户数据
 */
void cel_context_set_resolver(cel_context_t *ctx, cel_var_resolver_fn resolver,
			       void *user_data);

/* ========== 配置 API ========== */

/**
 * @brief 设置最大递归深度
 *
 * @param ctx 执行上下文
 * @param max_depth 最大递归深度 (默认 100)
 */
void cel_context_set_max_recursion(cel_context_t *ctx, size_t max_depth);

/**
 * @brief 获取最大递归深度
 *
 * @param ctx 执行上下文
 * @return 最大递归深度
 */
size_t cel_context_get_max_recursion(const cel_context_t *ctx);

/**
 * @brief 获取当前递归深度
 *
 * @param ctx 执行上下文
 * @return 当前递归深度
 */
size_t cel_context_get_current_depth(const cel_context_t *ctx);

/**
 * @brief 获取父上下文
 *
 * @param ctx 执行上下文
 * @return 父上下文, 无父上下文返回 NULL
 */
cel_context_t *cel_context_get_parent(const cel_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* CEL_CONTEXT_H */
