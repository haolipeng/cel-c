/**
 * @file cel_ast.h
 * @brief CEL 抽象语法树 (AST) 定义
 *
 * 定义 CEL 表达式的 AST 节点类型和结构。
 */

#ifndef CEL_AST_H
#define CEL_AST_H

#include "cel/cel_token.h"
#include "cel/cel_value.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 前向声明 */
typedef struct cel_ast_node cel_ast_node_t;

/* ========== AST 节点类型 ========== */

typedef enum {
	/* 字面量 */
	CEL_AST_LITERAL,      /* 字面量: 123, "hello", true, null */

	/* 标识符 */
	CEL_AST_IDENT,        /* 标识符: foo, bar */

	/* 一元运算 */
	CEL_AST_UNARY,        /* 一元运算: -x, !x */

	/* 二元运算 */
	CEL_AST_BINARY,       /* 二元运算: x + y, x == y */

	/* 三元条件 */
	CEL_AST_TERNARY,      /* 三元条件: x ? y : z */

	/* 字段访问 */
	CEL_AST_SELECT,       /* 字段访问: obj.field */

	/* 索引访问 */
	CEL_AST_INDEX,        /* 索引访问: list[0], map["key"] */

	/* 函数调用 */
	CEL_AST_CALL,         /* 函数调用: func(arg1, arg2) */

	/* 列表字面量 */
	CEL_AST_LIST,         /* 列表: [1, 2, 3] */

	/* Map 字面量 */
	CEL_AST_MAP,          /* Map: {key: value} */

	/* 结构体字面量 */
	CEL_AST_STRUCT,       /* 结构体: Message{field: value} */

	/* 推导式表达式 (由宏展开生成) */
	CEL_AST_COMPREHENSION, /* 推导式: list.all(...), list.map(...) 等 */

} cel_ast_node_type_e;

/* ========== 一元运算符 ========== */

typedef enum {
	CEL_UNARY_NEG,   /* -x (取负) */
	CEL_UNARY_NOT,   /* !x (逻辑非) */
} cel_unary_op_e;

/* ========== 二元运算符 ========== */

typedef enum {
	/* 算术运算 */
	CEL_BINARY_ADD,      /* + */
	CEL_BINARY_SUB,      /* - */
	CEL_BINARY_MUL,      /* * */
	CEL_BINARY_DIV,      /* / */
	CEL_BINARY_MOD,      /* % */

	/* 比较运算 */
	CEL_BINARY_EQ,       /* == */
	CEL_BINARY_NE,       /* != */
	CEL_BINARY_LT,       /* < */
	CEL_BINARY_LE,       /* <= */
	CEL_BINARY_GT,       /* > */
	CEL_BINARY_GE,       /* >= */

	/* 逻辑运算 */
	CEL_BINARY_AND,      /* && */
	CEL_BINARY_OR,       /* || */

	/* 成员测试 */
	CEL_BINARY_IN,       /* in */

} cel_binary_op_e;

/* ========== AST 节点结构 ========== */

/**
 * @brief 字面量节点
 */
typedef struct {
	cel_value_t value; /* 字面量值 */
} cel_ast_literal_t;

/**
 * @brief 标识符节点
 */
typedef struct {
	const char *name;  /* 标识符名称 */
	size_t length;     /* 名称长度 */
} cel_ast_ident_t;

/**
 * @brief 一元运算节点
 */
typedef struct {
	cel_unary_op_e op;       /* 运算符 */
	cel_ast_node_t *operand; /* 操作数 */
} cel_ast_unary_t;

/**
 * @brief 二元运算节点
 */
typedef struct {
	cel_binary_op_e op;    /* 运算符 */
	cel_ast_node_t *left;  /* 左操作数 */
	cel_ast_node_t *right; /* 右操作数 */
} cel_ast_binary_t;

/**
 * @brief 三元条件节点
 */
typedef struct {
	cel_ast_node_t *condition; /* 条件 */
	cel_ast_node_t *if_true;   /* 真分支 */
	cel_ast_node_t *if_false;  /* 假分支 */
} cel_ast_ternary_t;

/**
 * @brief 字段访问节点
 */
typedef struct {
	cel_ast_node_t *operand; /* 对象 */
	const char *field;       /* 字段名 */
	size_t field_length;     /* 字段名长度 */
	bool optional;           /* 是否可选访问 (.?) */
} cel_ast_select_t;

/**
 * @brief 索引访问节点
 */
typedef struct {
	cel_ast_node_t *operand; /* 容器 */
	cel_ast_node_t *index;   /* 索引 */
	bool optional;           /* 是否可选访问 ([?]) */
} cel_ast_index_t;

/**
 * @brief 函数调用节点
 */
typedef struct {
	const char *function;       /* 函数名 */
	size_t function_length;     /* 函数名长度 */
	cel_ast_node_t *target;     /* 目标对象 (方法调用时使用) */
	cel_ast_node_t **args;      /* 参数列表 */
	size_t arg_count;           /* 参数数量 */
} cel_ast_call_t;

/**
 * @brief 列表字面量节点
 */
typedef struct {
	cel_ast_node_t **elements; /* 元素列表 */
	size_t element_count;      /* 元素数量 */
} cel_ast_list_t;

/**
 * @brief Map 条目
 */
typedef struct {
	cel_ast_node_t *key;   /* 键 */
	cel_ast_node_t *value; /* 值 */
} cel_ast_map_entry_t;

/**
 * @brief Map 字面量节点
 */
typedef struct {
	cel_ast_map_entry_t *entries; /* 条目列表 */
	size_t entry_count;           /* 条目数量 */
} cel_ast_map_t;

/**
 * @brief 结构体字段
 */
typedef struct {
	const char *name;      /* 字段名 */
	size_t name_length;    /* 字段名长度 */
	cel_ast_node_t *value; /* 字段值 */
} cel_ast_struct_field_t;

/**
 * @brief 结构体字面量节点
 */
typedef struct {
	const char *type_name;           /* 类型名 */
	size_t type_name_length;         /* 类型名长度 */
	cel_ast_struct_field_t *fields;  /* 字段列表 */
	size_t field_count;              /* 字段数量 */
} cel_ast_struct_t;

/**
 * @brief 推导式表达式节点 (Comprehension)
 *
 * 推导式用于实现宏展开后的迭代逻辑，例如：
 * - list.all(x, predicate)
 * - list.exists(x, predicate)
 * - list.map(x, transform)
 * - list.filter(x, predicate)
 *
 * 执行模型：
 * 1. 初始化累加器: accu_var = accu_init
 * 2. 对 iter_range 中每个元素迭代:
 *    a. 将元素绑定到 iter_var
 *    b. 检查 loop_cond，如果为 false 则中断
 *    c. 执行 loop_step，更新 accu_var
 * 3. 返回 result
 */
typedef struct {
	const char *iter_var;        /* 循环变量名 (例如: "x") */
	size_t iter_var_length;      /* 循环变量名长度 */

	const char *iter_var2;       /* 第二个循环变量 (Map迭代时使用, 可为NULL) */
	size_t iter_var2_length;     /* 第二个循环变量名长度 */

	cel_ast_node_t *iter_range;  /* 迭代范围 (列表或Map表达式) */

	const char *accu_var;        /* 累加器变量名 (通常是 "@result") */
	size_t accu_var_length;      /* 累加器变量名长度 */

	cel_ast_node_t *accu_init;   /* 累加器初始值 */
	cel_ast_node_t *loop_cond;   /* 循环条件 (false 时中断) */
	cel_ast_node_t *loop_step;   /* 循环步骤 (更新累加器) */
	cel_ast_node_t *result;      /* 结果表达式 */
} cel_ast_comprehension_t;

/**
 * @brief AST 节点
 */
struct cel_ast_node {
	cel_ast_node_type_e type; /* 节点类型 */
	cel_token_location_t loc; /* 源码位置 */

	union {
		cel_ast_literal_t literal;
		cel_ast_ident_t ident;
		cel_ast_unary_t unary;
		cel_ast_binary_t binary;
		cel_ast_ternary_t ternary;
		cel_ast_select_t select;
		cel_ast_index_t index;
		cel_ast_call_t call;
		cel_ast_list_t list;
		cel_ast_map_t map;
		cel_ast_struct_t struct_lit;
		cel_ast_comprehension_t comprehension;
	} as;
};

/* ========== AST 创建 API ========== */

cel_ast_node_t *cel_ast_create_literal(cel_value_t value,
					cel_token_location_t loc);
cel_ast_node_t *cel_ast_create_ident(const char *name, size_t length,
				      cel_token_location_t loc);
cel_ast_node_t *cel_ast_create_unary(cel_unary_op_e op,
				      cel_ast_node_t *operand,
				      cel_token_location_t loc);
cel_ast_node_t *cel_ast_create_binary(cel_binary_op_e op,
				       cel_ast_node_t *left,
				       cel_ast_node_t *right,
				       cel_token_location_t loc);
cel_ast_node_t *cel_ast_create_ternary(cel_ast_node_t *condition,
					cel_ast_node_t *if_true,
					cel_ast_node_t *if_false,
					cel_token_location_t loc);
cel_ast_node_t *cel_ast_create_select(cel_ast_node_t *operand,
				       const char *field, size_t field_length,
				       bool optional, cel_token_location_t loc);
cel_ast_node_t *cel_ast_create_index(cel_ast_node_t *operand,
				      cel_ast_node_t *index, bool optional,
				      cel_token_location_t loc);
cel_ast_node_t *cel_ast_create_call(const char *function,
				     size_t function_length,
				     cel_ast_node_t *target,
				     cel_ast_node_t **args, size_t arg_count,
				     cel_token_location_t loc);
cel_ast_node_t *cel_ast_create_list(cel_ast_node_t **elements,
				     size_t element_count,
				     cel_token_location_t loc);
cel_ast_node_t *cel_ast_create_map(cel_ast_map_entry_t *entries,
				    size_t entry_count,
				    cel_token_location_t loc);
cel_ast_node_t *cel_ast_create_struct(const char *type_name,
				       size_t type_name_length,
				       cel_ast_struct_field_t *fields,
				       size_t field_count,
				       cel_token_location_t loc);
cel_ast_node_t *cel_ast_create_comprehension(
	const char *iter_var, size_t iter_var_length,
	const char *iter_var2, size_t iter_var2_length,
	cel_ast_node_t *iter_range,
	const char *accu_var, size_t accu_var_length,
	cel_ast_node_t *accu_init,
	cel_ast_node_t *loop_cond,
	cel_ast_node_t *loop_step,
	cel_ast_node_t *result,
	cel_token_location_t loc);

/* ========== AST 销毁 API ========== */

void cel_ast_destroy(cel_ast_node_t *node);

/* ========== AST 辅助函数 ========== */

const char *cel_ast_node_type_name(cel_ast_node_type_e type);
const char *cel_unary_op_name(cel_unary_op_e op);
const char *cel_binary_op_name(cel_binary_op_e op);

#ifdef __cplusplus
}
#endif

#endif /* CEL_AST_H */
