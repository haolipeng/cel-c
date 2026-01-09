/**
 * @file cel_token.h
 * @brief CEL Token 定义
 *
 * 定义 CEL 表达式的所有 Token 类型和结构。
 */

#ifndef CEL_TOKEN_H
#define CEL_TOKEN_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Token 类型枚举 ========== */

/**
 * @brief CEL Token 类型
 */
typedef enum {
	/* 特殊 Token */
	CEL_TOKEN_EOF = 0,    /* 文件结束 */
	CEL_TOKEN_ERROR,      /* 词法错误 */

	/* 字面量 */
	CEL_TOKEN_INT,        /* 整数字面量: 123, 0x1A */
	CEL_TOKEN_UINT,       /* 无符号整数: 123u */
	CEL_TOKEN_DOUBLE,     /* 浮点数: 3.14, 1.23e10 */
	CEL_TOKEN_STRING,     /* 字符串: "hello" */
	CEL_TOKEN_BYTES,      /* 字节数组: b"hello" */
	CEL_TOKEN_TRUE,       /* 布尔值: true */
	CEL_TOKEN_FALSE,      /* 布尔值: false */
	CEL_TOKEN_NULL,       /* null */

	/* 标识符 */
	CEL_TOKEN_IDENTIFIER, /* 标识符: foo, bar_baz */

	/* 算术运算符 */
	CEL_TOKEN_PLUS,       /* + */
	CEL_TOKEN_MINUS,      /* - */
	CEL_TOKEN_STAR,       /* * */
	CEL_TOKEN_SLASH,      /* / */
	CEL_TOKEN_PERCENT,    /* % */

	/* 比较运算符 */
	CEL_TOKEN_EQUAL_EQUAL,      /* == */
	CEL_TOKEN_BANG_EQUAL,       /* != */
	CEL_TOKEN_LESS,             /* < */
	CEL_TOKEN_LESS_EQUAL,       /* <= */
	CEL_TOKEN_GREATER,          /* > */
	CEL_TOKEN_GREATER_EQUAL,    /* >= */

	/* 逻辑运算符 */
	CEL_TOKEN_AND_AND,    /* && */
	CEL_TOKEN_OR_OR,      /* || */
	CEL_TOKEN_BANG,       /* ! */

	/* 三元运算符 */
	CEL_TOKEN_QUESTION,   /* ? */
	CEL_TOKEN_COLON,      /* : */

	/* 字段访问和索引 */
	CEL_TOKEN_DOT,               /* . */
	CEL_TOKEN_DOT_QUESTION,      /* .? (可选字段访问) */
	CEL_TOKEN_LBRACKET,          /* [ */
	CEL_TOKEN_RBRACKET,          /* ] */
	CEL_TOKEN_LBRACKET_QUESTION, /* [? (可选索引访问) */

	/* 括号和大括号 */
	CEL_TOKEN_LPAREN,     /* ( */
	CEL_TOKEN_RPAREN,     /* ) */
	CEL_TOKEN_LBRACE,     /* { */
	CEL_TOKEN_RBRACE,     /* } */

	/* 其他 */
	CEL_TOKEN_COMMA,      /* , */

	/* 关键字 (保留用于扩展) */
	CEL_TOKEN_IN,         /* in (宏: has, all, exists) */

} cel_token_type_e;

/* ========== Token 位置信息 ========== */

/**
 * @brief Token 源码位置
 */
typedef struct {
	const char *source; /* 源代码文本 */
	size_t line;        /* 行号 (1-based) */
	size_t column;      /* 列号 (1-based) */
	size_t offset;      /* 字节偏移 (0-based) */
	size_t length;      /* Token 长度 (字节数) */
} cel_token_location_t;

/* ========== Token 结构 ========== */

/**
 * @brief CEL Token
 *
 * 表示词法分析器产生的一个 Token。
 */
typedef struct {
	cel_token_type_e type;      /* Token 类型 */
	cel_token_location_t loc;   /* 源码位置 */

	/* Token 值 (根据类型使用不同字段) */
	union {
		int64_t int_value;       /* CEL_TOKEN_INT */
		uint64_t uint_value;     /* CEL_TOKEN_UINT */
		double double_value;     /* CEL_TOKEN_DOUBLE */
		struct {
			const char *str_value;   /* CEL_TOKEN_STRING, CEL_TOKEN_BYTES, CEL_TOKEN_IDENTIFIER, CEL_TOKEN_ERROR */
			size_t str_length;       /* 字符串/字节长度 */
		} str;
	} value;
} cel_token_t;

/* ========== Token 辅助函数 ========== */

/**
 * @brief 获取 Token 类型名称
 *
 * @param type Token 类型
 * @return Token 类型的字符串表示
 */
const char *cel_token_type_name(cel_token_type_e type);

/**
 * @brief 判断 Token 是否为字面量
 *
 * @param type Token 类型
 * @return true 如果是字面量
 */
bool cel_token_is_literal(cel_token_type_e type);

/**
 * @brief 判断 Token 是否为运算符
 *
 * @param type Token 类型
 * @return true 如果是运算符
 */
bool cel_token_is_operator(cel_token_type_e type);

/**
 * @brief 判断 Token 是否为关键字
 *
 * @param type Token 类型
 * @return true 如果是关键字
 */
bool cel_token_is_keyword(cel_token_type_e type);

#ifdef __cplusplus
}
#endif

#endif /* CEL_TOKEN_H */
