/**
 * @file test_comprehension.c
 * @brief CEL Comprehension 求值单元测试
 */

#include "cel/cel_eval.h"
#include "cel/cel_ast.h"
#include "cel/cel_value.h"
#include "unity.h"
#include <string.h>

/* ========== Unity 设置 ========== */

static cel_context_t *ctx = NULL;

void setUp(void)
{
	ctx = cel_context_create();
	TEST_ASSERT_NOT_NULL(ctx);
}

void tearDown(void)
{
	if (ctx) {
		cel_context_destroy(ctx);
		ctx = NULL;
	}
}

/* ========== 辅助函数 ========== */

/**
 * @brief 创建标识符节点
 */
static cel_ast_node_t *create_ident(const char *name)
{
	cel_token_location_t loc = {0};
	return cel_ast_create_ident(name, strlen(name), loc);
}

/**
 * @brief 创建整数字面量节点
 */
static cel_ast_node_t *create_int(int64_t value)
{
	cel_token_location_t loc = {0};
	cel_value_t val = cel_value_int(value);
	return cel_ast_create_literal(val, loc);
}

/**
 * @brief 创建布尔字面量节点
 */
static cel_ast_node_t *create_bool(bool value)
{
	cel_token_location_t loc = {0};
	cel_value_t val = cel_value_bool(value);
	return cel_ast_create_literal(val, loc);
}

/**
 * @brief 创建二元运算节点
 */
static cel_ast_node_t *create_binary(cel_binary_op_e op, cel_ast_node_t *left,
				      cel_ast_node_t *right)
{
	cel_token_location_t loc = {0};
	return cel_ast_create_binary(op, left, right, loc);
}

/**
 * @brief 创建列表字面量节点
 */
static cel_ast_node_t *create_list(cel_ast_node_t **elements, size_t count)
{
	cel_token_location_t loc = {0};
	return cel_ast_create_list(elements, count, loc);
}

/**
 * @brief 创建列表值 (用于上下文绑定)
 */
static cel_value_t create_list_value(int64_t *values, size_t count)
{
	cel_list_t *list = cel_list_create(count);
	for (size_t i = 0; i < count; i++) {
		cel_value_t val = cel_value_int(values[i]);
		cel_list_append(list, &val);
	}
	cel_value_t result;
	result.type = CEL_TYPE_LIST;
	result.value.list_value = list;
	return result;
}

/* ========== all() Comprehension 测试 ========== */

void test_comprehension_all_true(void)
{
	/* 测试: [1, 2, 3].all(x, x > 0) => true */

	/* 创建列表并绑定到上下文 */
	int64_t values[] = {1, 2, 3};
	cel_value_t list_val = create_list_value(values, 3);
	cel_context_add_binding(ctx, "mylist", 6, list_val);

	/* 创建 Comprehension:
	 *   iter_var: x
	 *   iter_range: mylist (标识符)
	 *   accu_init: true
	 *   loop_cond: @result
	 *   loop_step: @result && (x > 0)
	 *   result: @result
	 */
	cel_token_location_t loc = {0};

	cel_ast_node_t *comp = cel_ast_create_comprehension(
		"x", 1,                           /* iter_var */
		NULL, 0,                          /* iter_var2 (unused) */
		create_ident("mylist"),           /* iter_range */
		"@result", 7,                     /* accu_var */
		create_bool(true),                /* accu_init: true */
		create_ident("@result"),          /* loop_cond: @result */
		create_binary(CEL_BINARY_AND,     /* loop_step: @result && (x > 0) */
			      create_ident("@result"),
			      create_binary(CEL_BINARY_GT,
					    create_ident("x"),
					    create_int(0))),
		create_ident("@result"),          /* result: @result */
		loc
	);

	/* 求值 */
	cel_value_t result;
	bool success = cel_eval(comp, ctx, &result);

	TEST_ASSERT_TRUE(success);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);

	cel_ast_destroy(comp);
}

void test_comprehension_all_false(void)
{
	/* 测试: [1, -2, 3].all(x, x > 0) => false */

	int64_t values[] = {1, -2, 3};
	cel_value_t list_val = create_list_value(values, 3);
	cel_context_add_binding(ctx, "mylist", 6, list_val);

	cel_token_location_t loc = {0};

	cel_ast_node_t *comp = cel_ast_create_comprehension(
		"x", 1,
		NULL, 0,
		create_ident("mylist"),
		"@result", 7,
		create_bool(true),
		create_ident("@result"),          /* 短路: @result 为 false 时停止 */
		create_binary(CEL_BINARY_AND,
			      create_ident("@result"),
			      create_binary(CEL_BINARY_GT,
					    create_ident("x"),
					    create_int(0))),
		create_ident("@result"),
		loc
	);

	cel_value_t result;
	bool success = cel_eval(comp, ctx, &result);

	TEST_ASSERT_TRUE(success);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_FALSE(result.value.bool_value);

	cel_ast_destroy(comp);
}

/* ========== exists() Comprehension 测试 ========== */

void test_comprehension_exists_true(void)
{
	/* 测试: [1, 2, 3].exists(x, x > 2) => true */

	int64_t values[] = {1, 2, 3};
	cel_value_t list_val = create_list_value(values, 3);
	cel_context_add_binding(ctx, "mylist", 6, list_val);

	cel_token_location_t loc = {0};

	cel_ast_node_t *comp = cel_ast_create_comprehension(
		"x", 1,
		NULL, 0,
		create_ident("mylist"),
		"@result", 7,
		create_bool(false),               /* accu_init: false */
		cel_ast_create_unary(CEL_UNARY_NOT,  /* loop_cond: !@result */
				     create_ident("@result"),
				     loc),
		create_binary(CEL_BINARY_OR,      /* loop_step: @result || (x > 2) */
			      create_ident("@result"),
			      create_binary(CEL_BINARY_GT,
					    create_ident("x"),
					    create_int(2))),
		create_ident("@result"),
		loc
	);

	cel_value_t result;
	bool success = cel_eval(comp, ctx, &result);

	TEST_ASSERT_TRUE(success);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);

	cel_ast_destroy(comp);
}

void test_comprehension_exists_false(void)
{
	/* 测试: [1, 2, 3].exists(x, x > 5) => false */

	int64_t values[] = {1, 2, 3};
	cel_value_t list_val = create_list_value(values, 3);
	cel_context_add_binding(ctx, "mylist", 6, list_val);

	cel_token_location_t loc = {0};

	cel_ast_node_t *comp = cel_ast_create_comprehension(
		"x", 1,
		NULL, 0,
		create_ident("mylist"),
		"@result", 7,
		create_bool(false),
		cel_ast_create_unary(CEL_UNARY_NOT,
				     create_ident("@result"),
				     loc),
		create_binary(CEL_BINARY_OR,
			      create_ident("@result"),
			      create_binary(CEL_BINARY_GT,
					    create_ident("x"),
					    create_int(5))),
		create_ident("@result"),
		loc
	);

	cel_value_t result;
	bool success = cel_eval(comp, ctx, &result);

	TEST_ASSERT_TRUE(success);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_FALSE(result.value.bool_value);

	cel_ast_destroy(comp);
}

/* ========== exists_one() Comprehension 测试 ========== */

void test_comprehension_exists_one_true(void)
{
	/* 测试: [1, 2, 3].exists_one(x, x == 2) => true (只有一个元素等于 2) */

	int64_t values[] = {1, 2, 3};
	cel_value_t list_val = create_list_value(values, 3);
	cel_context_add_binding(ctx, "mylist", 6, list_val);

	cel_token_location_t loc = {0};

	/* exists_one 使用计数器累加器 */
	cel_ast_node_t *comp = cel_ast_create_comprehension(
		"x", 1,
		NULL, 0,
		create_ident("mylist"),
		"@result", 7,
		create_int(0),                    /* accu_init: 0 */
		create_bool(true),                /* loop_cond: true (不短路) */
		/* loop_step: (x == 2) ? (@result + 1) : @result */
		cel_ast_create_ternary(
			create_binary(CEL_BINARY_EQ, create_ident("x"), create_int(2)),
			create_binary(CEL_BINARY_ADD, create_ident("@result"), create_int(1)),
			create_ident("@result"),
			loc
		),
		/* result: @result == 1 */
		create_binary(CEL_BINARY_EQ, create_ident("@result"), create_int(1)),
		loc
	);

	cel_value_t result;
	bool success = cel_eval(comp, ctx, &result);

	TEST_ASSERT_TRUE(success);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);

	cel_ast_destroy(comp);
}

void test_comprehension_exists_one_false_multiple(void)
{
	/* 测试: [2, 2, 3].exists_one(x, x == 2) => false (有两个元素等于 2) */

	int64_t values[] = {2, 2, 3};
	cel_value_t list_val = create_list_value(values, 3);
	cel_context_add_binding(ctx, "mylist", 6, list_val);

	cel_token_location_t loc = {0};

	cel_ast_node_t *comp = cel_ast_create_comprehension(
		"x", 1,
		NULL, 0,
		create_ident("mylist"),
		"@result", 7,
		create_int(0),
		create_bool(true),
		cel_ast_create_ternary(
			create_binary(CEL_BINARY_EQ, create_ident("x"), create_int(2)),
			create_binary(CEL_BINARY_ADD, create_ident("@result"), create_int(1)),
			create_ident("@result"),
			loc
		),
		create_binary(CEL_BINARY_EQ, create_ident("@result"), create_int(1)),
		loc
	);

	cel_value_t result;
	bool success = cel_eval(comp, ctx, &result);

	TEST_ASSERT_TRUE(success);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_FALSE(result.value.bool_value);

	cel_ast_destroy(comp);
}

/* ========== map() Comprehension 测试 ========== */

void test_comprehension_map_basic(void)
{
	/* 测试: [1, 2, 3].map(x, x * 2) => [2, 4, 6] */

	int64_t values[] = {1, 2, 3};
	cel_value_t list_val = create_list_value(values, 3);
	cel_context_add_binding(ctx, "mylist", 6, list_val);

	cel_token_location_t loc = {0};

	/* map 使用空列表作为累加器,逐步追加元素 */
	cel_ast_node_t *comp = cel_ast_create_comprehension(
		"x", 1,
		NULL, 0,
		create_ident("mylist"),
		"@result", 7,
		create_list(NULL, 0),             /* accu_init: [] */
		create_bool(true),                /* loop_cond: true */
		/* loop_step: @result + [x * 2] */
		create_binary(CEL_BINARY_ADD,
			      create_ident("@result"),
			      create_list((cel_ast_node_t*[]){
					      create_binary(CEL_BINARY_MUL,
							    create_ident("x"),
							    create_int(2))
				      }, 1)),
		create_ident("@result"),
		loc
	);

	cel_value_t result;
	bool success = cel_eval(comp, ctx, &result);

	TEST_ASSERT_TRUE(success);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_LIST, result.type);
	TEST_ASSERT_EQUAL_size_t(3, cel_list_size(result.value.list_value));

	/* 验证结果列表内容 */
	cel_value_t elem;
	cel_list_get(result.value.list_value, 0, &elem);
	TEST_ASSERT_EQUAL_INT64(2, elem.value.int_value);

	cel_list_get(result.value.list_value, 1, &elem);
	TEST_ASSERT_EQUAL_INT64(4, elem.value.int_value);

	cel_list_get(result.value.list_value, 2, &elem);
	TEST_ASSERT_EQUAL_INT64(6, elem.value.int_value);

	cel_value_destroy(&result);
	cel_ast_destroy(comp);
}

/* ========== filter() Comprehension 测试 ========== */

void test_comprehension_filter_basic(void)
{
	/* 测试: [1, 2, 3, 4].filter(x, x > 2) => [3, 4] */

	int64_t values[] = {1, 2, 3, 4};
	cel_value_t list_val = create_list_value(values, 4);
	cel_context_add_binding(ctx, "mylist", 6, list_val);

	cel_token_location_t loc = {0};

	cel_ast_node_t *comp = cel_ast_create_comprehension(
		"x", 1,
		NULL, 0,
		create_ident("mylist"),
		"@result", 7,
		create_list(NULL, 0),             /* accu_init: [] */
		create_bool(true),                /* loop_cond: true */
		/* loop_step: (x > 2) ? (@result + [x]) : @result */
		cel_ast_create_ternary(
			create_binary(CEL_BINARY_GT, create_ident("x"), create_int(2)),
			create_binary(CEL_BINARY_ADD,
				      create_ident("@result"),
				      create_list((cel_ast_node_t*[]){create_ident("x")}, 1)),
			create_ident("@result"),
			loc
		),
		create_ident("@result"),
		loc
	);

	cel_value_t result;
	bool success = cel_eval(comp, ctx, &result);

	TEST_ASSERT_TRUE(success);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_LIST, result.type);
	TEST_ASSERT_EQUAL_size_t(2, cel_list_size(result.value.list_value));

	/* 验证结果列表内容 */
	cel_value_t elem;
	cel_list_get(result.value.list_value, 0, &elem);
	TEST_ASSERT_EQUAL_INT64(3, elem.value.int_value);

	cel_list_get(result.value.list_value, 1, &elem);
	TEST_ASSERT_EQUAL_INT64(4, elem.value.int_value);

	cel_value_destroy(&result);
	cel_ast_destroy(comp);
}

/* ========== 边界条件测试 ========== */

void test_comprehension_empty_list(void)
{
	/* 测试空列表: [].all(x, x > 0) => true */

	cel_value_t list_val = create_list_value(NULL, 0);
	cel_context_add_binding(ctx, "mylist", 6, list_val);

	cel_token_location_t loc = {0};

	cel_ast_node_t *comp = cel_ast_create_comprehension(
		"x", 1,
		NULL, 0,
		create_ident("mylist"),
		"@result", 7,
		create_bool(true),
		create_ident("@result"),
		create_binary(CEL_BINARY_AND,
			      create_ident("@result"),
			      create_bool(true)),
		create_ident("@result"),
		loc
	);

	cel_value_t result;
	bool success = cel_eval(comp, ctx, &result);

	TEST_ASSERT_TRUE(success);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);

	cel_ast_destroy(comp);
}

void test_comprehension_single_element(void)
{
	/* 测试单元素列表: [42].exists_one(x, x == 42) => true */

	int64_t values[] = {42};
	cel_value_t list_val = create_list_value(values, 1);
	cel_context_add_binding(ctx, "mylist", 6, list_val);

	cel_token_location_t loc = {0};

	cel_ast_node_t *comp = cel_ast_create_comprehension(
		"x", 1,
		NULL, 0,
		create_ident("mylist"),
		"@result", 7,
		create_int(0),
		create_bool(true),
		cel_ast_create_ternary(
			create_binary(CEL_BINARY_EQ, create_ident("x"), create_int(42)),
			create_binary(CEL_BINARY_ADD, create_ident("@result"), create_int(1)),
			create_ident("@result"),
			loc
		),
		create_binary(CEL_BINARY_EQ, create_ident("@result"), create_int(1)),
		loc
	);

	cel_value_t result;
	bool success = cel_eval(comp, ctx, &result);

	TEST_ASSERT_TRUE(success);
	TEST_ASSERT_EQUAL_INT(CEL_TYPE_BOOL, result.type);
	TEST_ASSERT_TRUE(result.value.bool_value);

	cel_ast_destroy(comp);
}

/* ========== 错误处理测试 ========== */

void test_comprehension_invalid_iter_range(void)
{
	/* 测试非列表/Map 的迭代范围 */

	cel_token_location_t loc = {0};

	cel_ast_node_t *comp = cel_ast_create_comprehension(
		"x", 1,
		NULL, 0,
		create_int(123),                  /* 错误: 整数不能作为迭代范围 */
		"@result", 7,
		create_bool(true),
		create_bool(true),
		create_ident("@result"),
		create_ident("@result"),
		loc
	);

	cel_value_t result;
	bool success = cel_eval(comp, ctx, &result);

	TEST_ASSERT_FALSE(success);
	TEST_ASSERT_NOT_NULL(cel_context_get_error(ctx));

	cel_ast_destroy(comp);
}

/* ========== Main 测试运行器 ========== */

int main(void)
{
	UNITY_BEGIN();

	/* all() Comprehension 测试 */
	RUN_TEST(test_comprehension_all_true);
	RUN_TEST(test_comprehension_all_false);

	/* exists() Comprehension 测试 */
	RUN_TEST(test_comprehension_exists_true);
	RUN_TEST(test_comprehension_exists_false);

	/* exists_one() Comprehension 测试 */
	RUN_TEST(test_comprehension_exists_one_true);
	RUN_TEST(test_comprehension_exists_one_false_multiple);

	/* map() Comprehension 测试 */
	RUN_TEST(test_comprehension_map_basic);

	/* filter() Comprehension 测试 */
	RUN_TEST(test_comprehension_filter_basic);

	/* 边界条件测试 */
	RUN_TEST(test_comprehension_empty_list);
	RUN_TEST(test_comprehension_single_element);

	/* 错误处理测试 */
	RUN_TEST(test_comprehension_invalid_iter_range);

	return UNITY_END();
}
