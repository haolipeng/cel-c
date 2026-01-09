/**
 * @file test_parser.c
 * @brief CEL Parser 单元测试
 */

#include "cel/cel_parser.h"
#include "unity.h"
#include <string.h>

/* ========== Unity 设置 ========== */

void setUp(void)
{
}

void tearDown(void)
{
}

/* ========== 辅助函数 ========== */

static cel_ast_node_t *parse_expr(const char *source)
{
	cel_lexer_t lexer;
	cel_parser_t parser;

	cel_lexer_init(&lexer, source);
	cel_parser_init(&parser, &lexer);

	cel_ast_node_t *ast = cel_parser_parse(&parser);
	cel_parser_cleanup(&parser);
	return ast;
}

/* ========== 字面量测试 ========== */

void test_parse_int_literal(void)
{
	cel_ast_node_t *ast = parse_expr("123");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_LITERAL, ast->type);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_INT, ast->as.literal.value.type);
	TEST_ASSERT_EQUAL_INT64(123, ast->as.literal.value.value.int_value);

	cel_ast_destroy(ast);
}

void test_parse_double_literal(void)
{
	cel_ast_node_t *ast = parse_expr("3.14");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_LITERAL, ast->type);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_DOUBLE, ast->as.literal.value.type);
	TEST_ASSERT_DOUBLE_WITHIN(0.001, 3.14,
				  ast->as.literal.value.value.double_value);

	cel_ast_destroy(ast);
}

void test_parse_string_literal(void)
{
	cel_ast_node_t *ast = parse_expr("\"hello\"");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_LITERAL, ast->type);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_STRING, ast->as.literal.value.type);

	cel_ast_destroy(ast);
}

void test_parse_bool_literal(void)
{
	cel_ast_node_t *ast = parse_expr("true");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_LITERAL, ast->type);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, ast->as.literal.value.type);
	TEST_ASSERT_TRUE(ast->as.literal.value.value.bool_value);

	cel_ast_destroy(ast);
}

void test_parse_null_literal(void)
{
	cel_ast_node_t *ast = parse_expr("null");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_LITERAL, ast->type);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_NULL, ast->as.literal.value.type);

	cel_ast_destroy(ast);
}

/* ========== 标识符测试 ========== */

void test_parse_identifier(void)
{
	cel_ast_node_t *ast = parse_expr("foo");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_IDENT, ast->type);
	TEST_ASSERT_EQUAL_size_t(3, ast->as.ident.length);
	TEST_ASSERT_EQUAL_MEMORY("foo", ast->as.ident.name, 3);

	cel_ast_destroy(ast);
}

/* ========== 一元运算测试 ========== */

void test_parse_unary_neg(void)
{
	cel_ast_node_t *ast = parse_expr("-123");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_UNARY, ast->type);
	TEST_ASSERT_EQUAL_INT(CEL_UNARY_NEG, ast->as.unary.op);
	TEST_ASSERT_NOT_NULL(ast->as.unary.operand);
	TEST_ASSERT_EQUAL_INT(CEL_AST_LITERAL, ast->as.unary.operand->type);

	cel_ast_destroy(ast);
}

void test_parse_unary_not(void)
{
	cel_ast_node_t *ast = parse_expr("!true");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_UNARY, ast->type);
	TEST_ASSERT_EQUAL_INT(CEL_UNARY_NOT, ast->as.unary.op);

	cel_ast_destroy(ast);
}

/* ========== 二元运算测试 ========== */

void test_parse_binary_add(void)
{
	cel_ast_node_t *ast = parse_expr("1 + 2");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_BINARY, ast->type);
	TEST_ASSERT_EQUAL_INT(CEL_BINARY_ADD, ast->as.binary.op);
	TEST_ASSERT_NOT_NULL(ast->as.binary.left);
	TEST_ASSERT_NOT_NULL(ast->as.binary.right);

	cel_ast_destroy(ast);
}

void test_parse_binary_mul(void)
{
	cel_ast_node_t *ast = parse_expr("3 * 4");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_BINARY, ast->type);
	TEST_ASSERT_EQUAL_INT(CEL_BINARY_MUL, ast->as.binary.op);

	cel_ast_destroy(ast);
}

void test_parse_binary_comparison(void)
{
	cel_ast_node_t *ast = parse_expr("x == y");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_BINARY, ast->type);
	TEST_ASSERT_EQUAL_INT(CEL_BINARY_EQ, ast->as.binary.op);

	cel_ast_destroy(ast);
}

void test_parse_binary_logical(void)
{
	cel_ast_node_t *ast = parse_expr("a && b");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_BINARY, ast->type);
	TEST_ASSERT_EQUAL_INT(CEL_BINARY_AND, ast->as.binary.op);

	cel_ast_destroy(ast);
}

/* ========== 运算符优先级测试 ========== */

void test_parse_precedence_mul_add(void)
{
	cel_ast_node_t *ast = parse_expr("1 + 2 * 3");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_BINARY, ast->type);
	TEST_ASSERT_EQUAL_INT(CEL_BINARY_ADD, ast->as.binary.op);

	/* 右侧应该是 2 * 3 */
	TEST_ASSERT_EQUAL_INT(CEL_AST_BINARY, ast->as.binary.right->type);
	TEST_ASSERT_EQUAL_INT(CEL_BINARY_MUL,
			      ast->as.binary.right->as.binary.op);

	cel_ast_destroy(ast);
}

void test_parse_precedence_comparison_logical(void)
{
	cel_ast_node_t *ast = parse_expr("x < 5 && y > 10");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_BINARY, ast->type);
	TEST_ASSERT_EQUAL_INT(CEL_BINARY_AND, ast->as.binary.op);

	/* 左右两侧应该是比较运算 */
	TEST_ASSERT_EQUAL_INT(CEL_AST_BINARY, ast->as.binary.left->type);
	TEST_ASSERT_EQUAL_INT(CEL_BINARY_LT, ast->as.binary.left->as.binary.op);

	TEST_ASSERT_EQUAL_INT(CEL_AST_BINARY, ast->as.binary.right->type);
	TEST_ASSERT_EQUAL_INT(CEL_BINARY_GT,
			      ast->as.binary.right->as.binary.op);

	cel_ast_destroy(ast);
}

/* ========== 括号表达式测试 ========== */

void test_parse_parentheses(void)
{
	cel_ast_node_t *ast = parse_expr("(1 + 2) * 3");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_BINARY, ast->type);
	TEST_ASSERT_EQUAL_INT(CEL_BINARY_MUL, ast->as.binary.op);

	/* 左侧应该是 1 + 2 */
	TEST_ASSERT_EQUAL_INT(CEL_AST_BINARY, ast->as.binary.left->type);
	TEST_ASSERT_EQUAL_INT(CEL_BINARY_ADD,
			      ast->as.binary.left->as.binary.op);

	cel_ast_destroy(ast);
}

/* ========== 三元运算符测试 ========== */

void test_parse_ternary(void)
{
	cel_ast_node_t *ast = parse_expr("x > 0 ? 1 : -1");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_TERNARY, ast->type);
	TEST_ASSERT_NOT_NULL(ast->as.ternary.condition);
	TEST_ASSERT_NOT_NULL(ast->as.ternary.if_true);
	TEST_ASSERT_NOT_NULL(ast->as.ternary.if_false);

	cel_ast_destroy(ast);
}

/* ========== 字段访问测试 ========== */

void test_parse_field_access(void)
{
	cel_ast_node_t *ast = parse_expr("obj.field");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_SELECT, ast->type);
	TEST_ASSERT_NOT_NULL(ast->as.select.operand);
	TEST_ASSERT_EQUAL_size_t(5, ast->as.select.field_length);
	TEST_ASSERT_EQUAL_MEMORY("field", ast->as.select.field, 5);
	TEST_ASSERT_FALSE(ast->as.select.optional);

	cel_ast_destroy(ast);
}

void test_parse_optional_field_access(void)
{
	cel_ast_node_t *ast = parse_expr("obj.?field");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_SELECT, ast->type);
	TEST_ASSERT_TRUE(ast->as.select.optional);

	cel_ast_destroy(ast);
}

/* ========== 索引访问测试 ========== */

void test_parse_index_access(void)
{
	cel_ast_node_t *ast = parse_expr("list[0]");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_INDEX, ast->type);
	TEST_ASSERT_NOT_NULL(ast->as.index.operand);
	TEST_ASSERT_NOT_NULL(ast->as.index.index);
	TEST_ASSERT_FALSE(ast->as.index.optional);

	cel_ast_destroy(ast);
}

/* ========== 函数调用测试 ========== */

void test_parse_function_call_no_args(void)
{
	cel_ast_node_t *ast = parse_expr("func()");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_CALL, ast->type);
	TEST_ASSERT_EQUAL_size_t(4, ast->as.call.function_length);
	TEST_ASSERT_EQUAL_MEMORY("func", ast->as.call.function, 4);
	TEST_ASSERT_EQUAL_size_t(0, ast->as.call.arg_count);

	cel_ast_destroy(ast);
}

void test_parse_function_call_with_args(void)
{
	cel_ast_node_t *ast = parse_expr("func(1, 2, 3)");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_CALL, ast->type);
	TEST_ASSERT_EQUAL_size_t(3, ast->as.call.arg_count);

	cel_ast_destroy(ast);
}

/* ========== 列表字面量测试 ========== */

void test_parse_empty_list(void)
{
	cel_ast_node_t *ast = parse_expr("[]");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_LIST, ast->type);
	TEST_ASSERT_EQUAL_size_t(0, ast->as.list.element_count);

	cel_ast_destroy(ast);
}

void test_parse_list_with_elements(void)
{
	cel_ast_node_t *ast = parse_expr("[1, 2, 3]");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_LIST, ast->type);
	TEST_ASSERT_EQUAL_size_t(3, ast->as.list.element_count);

	cel_ast_destroy(ast);
}

/* ========== Map 字面量测试 ========== */

void test_parse_empty_map(void)
{
	cel_ast_node_t *ast = parse_expr("{}");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_MAP, ast->type);
	TEST_ASSERT_EQUAL_size_t(0, ast->as.map.entry_count);

	cel_ast_destroy(ast);
}

void test_parse_map_with_entries(void)
{
	cel_ast_node_t *ast = parse_expr("{\"a\": 1, \"b\": 2}");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_MAP, ast->type);
	TEST_ASSERT_EQUAL_size_t(2, ast->as.map.entry_count);

	cel_ast_destroy(ast);
}

/* ========== 复杂表达式测试 ========== */

void test_parse_complex_expression(void)
{
	cel_ast_node_t *ast = parse_expr("(x + y) * 2 > 10 ? true : false");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_TERNARY, ast->type);

	cel_ast_destroy(ast);
}

void test_parse_nested_field_access(void)
{
	cel_ast_node_t *ast = parse_expr("obj.field1.field2");

	TEST_ASSERT_NOT_NULL(ast);
	TEST_ASSERT_EQUAL_INT(CEL_AST_SELECT, ast->type);
	TEST_ASSERT_EQUAL_INT(CEL_AST_SELECT, ast->as.select.operand->type);

	cel_ast_destroy(ast);
}

/* ========== 错误处理测试 ========== */

void test_parse_error_empty(void)
{
	cel_ast_node_t *ast = parse_expr("");

	TEST_ASSERT_NULL(ast);
}

void test_parse_error_unexpected_token(void)
{
	cel_ast_node_t *ast = parse_expr("1 + + 2");

	TEST_ASSERT_NULL(ast);
}

/* ========== Unity 主函数 ========== */

int main(void)
{
	UNITY_BEGIN();

	/* 字面量测试 */
	RUN_TEST(test_parse_int_literal);
	RUN_TEST(test_parse_double_literal);
	RUN_TEST(test_parse_string_literal);
	RUN_TEST(test_parse_bool_literal);
	RUN_TEST(test_parse_null_literal);

	/* 标识符测试 */
	RUN_TEST(test_parse_identifier);

	/* 一元运算测试 */
	RUN_TEST(test_parse_unary_neg);
	RUN_TEST(test_parse_unary_not);

	/* 二元运算测试 */
	RUN_TEST(test_parse_binary_add);
	RUN_TEST(test_parse_binary_mul);
	RUN_TEST(test_parse_binary_comparison);
	RUN_TEST(test_parse_binary_logical);

	/* 运算符优先级测试 */
	RUN_TEST(test_parse_precedence_mul_add);
	RUN_TEST(test_parse_precedence_comparison_logical);

	/* 括号表达式测试 */
	RUN_TEST(test_parse_parentheses);

	/* 三元运算符测试 */
	RUN_TEST(test_parse_ternary);

	/* 字段访问测试 */
	RUN_TEST(test_parse_field_access);
	RUN_TEST(test_parse_optional_field_access);

	/* 索引访问测试 */
	RUN_TEST(test_parse_index_access);

	/* 函数调用测试 */
	RUN_TEST(test_parse_function_call_no_args);
	RUN_TEST(test_parse_function_call_with_args);

	/* 列表字面量测试 */
	RUN_TEST(test_parse_empty_list);
	RUN_TEST(test_parse_list_with_elements);

	/* Map 字面量测试 */
	RUN_TEST(test_parse_empty_map);
	RUN_TEST(test_parse_map_with_entries);

	/* 复杂表达式测试 */
	RUN_TEST(test_parse_complex_expression);
	RUN_TEST(test_parse_nested_field_access);

	/* 错误处理测试 */
	RUN_TEST(test_parse_error_empty);
	RUN_TEST(test_parse_error_unexpected_token);

	return UNITY_END();
}
