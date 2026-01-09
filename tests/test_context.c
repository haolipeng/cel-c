/**
 * @file test_context.c
 * @brief CEL 执行上下文单元测试
 *
 * 测试 cel_context 的完整 API。
 */

#include "cel/cel_context.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>

/* ========== Unity 回调 ========== */

void setUp(void)
{
	/* 每个测试前调用 */
}

void tearDown(void)
{
	/* 每个测试后调用 */
}

/* ========== 上下文创建/销毁测试 ========== */

void test_context_create_empty(void)
{
	cel_context_t *ctx = cel_context_create_empty();
	TEST_ASSERT_NOT_NULL(ctx);

	/* 空上下文应该没有任何变量和函数 */
	TEST_ASSERT_FALSE(cel_context_has_variable(ctx, "x"));
	TEST_ASSERT_FALSE(cel_context_has_function(ctx, "foo"));

	cel_context_destroy(ctx);
}

void test_context_create(void)
{
	cel_context_t *ctx = cel_context_create();
	TEST_ASSERT_NOT_NULL(ctx);

	/* 标准上下文将来会包含内置函数 (Task 4.5) */
	/* 目前应该和 create_empty 一样 */

	cel_context_destroy(ctx);
}

void test_context_create_child(void)
{
	cel_context_t *parent = cel_context_create();
	TEST_ASSERT_NOT_NULL(parent);

	cel_context_t *child = cel_context_create_child(parent);
	TEST_ASSERT_NOT_NULL(child);

	TEST_ASSERT_EQUAL_PTR(parent, cel_context_get_parent(child));
	TEST_ASSERT_NULL(cel_context_get_parent(parent));

	cel_context_destroy(child);
	cel_context_destroy(parent);
}

void test_context_destroy_null(void)
{
	/* 销毁 NULL 指针应该安全 */
	cel_context_destroy(NULL);
}

/* ========== 变量操作测试 ========== */

void test_context_add_variable_int(void)
{
	cel_context_t *ctx = cel_context_create();
	TEST_ASSERT_NOT_NULL(ctx);

	cel_value_t value = cel_value_int(42);
	cel_error_code_e err = cel_context_add_variable(ctx, "x", &value);

	TEST_ASSERT_EQUAL(CEL_OK, err);
	TEST_ASSERT_TRUE(cel_context_has_variable(ctx, "x"));

	cel_context_destroy(ctx);
}

void test_context_get_variable(void)
{
	cel_context_t *ctx = cel_context_create();
	TEST_ASSERT_NOT_NULL(ctx);

	cel_value_t value = cel_value_int(123);
	cel_context_add_variable(ctx, "num", &value);

	cel_value_t *retrieved = cel_context_get_variable(ctx, "num");
	TEST_ASSERT_NOT_NULL(retrieved);
	TEST_ASSERT_EQUAL(CEL_TYPE_INT, retrieved->type);
	TEST_ASSERT_EQUAL_INT64(123, retrieved->value.int_value);

	cel_context_destroy(ctx);
}

void test_context_get_variable_not_found(void)
{
	cel_context_t *ctx = cel_context_create();
	TEST_ASSERT_NOT_NULL(ctx);

	cel_value_t *retrieved = cel_context_get_variable(ctx, "nonexistent");
	TEST_ASSERT_NULL(retrieved);

	cel_context_destroy(ctx);
}

void test_context_update_variable(void)
{
	cel_context_t *ctx = cel_context_create();
	TEST_ASSERT_NOT_NULL(ctx);

	/* 添加初始值 */
	cel_value_t value1 = cel_value_int(10);
	cel_context_add_variable(ctx, "x", &value1);

	/* 更新为新值 */
	cel_value_t value2 = cel_value_int(20);
	cel_context_add_variable(ctx, "x", &value2);

	cel_value_t *retrieved = cel_context_get_variable(ctx, "x");
	TEST_ASSERT_NOT_NULL(retrieved);
	TEST_ASSERT_EQUAL_INT64(20, retrieved->value.int_value);

	cel_context_destroy(ctx);
}

void test_context_remove_variable(void)
{
	cel_context_t *ctx = cel_context_create();
	TEST_ASSERT_NOT_NULL(ctx);

	cel_value_t value = cel_value_bool(true);
	cel_context_add_variable(ctx, "flag", &value);

	TEST_ASSERT_TRUE(cel_context_has_variable(ctx, "flag"));

	bool removed = cel_context_remove_variable(ctx, "flag");
	TEST_ASSERT_TRUE(removed);
	TEST_ASSERT_FALSE(cel_context_has_variable(ctx, "flag"));

	/* 再次移除应该失败 */
	removed = cel_context_remove_variable(ctx, "flag");
	TEST_ASSERT_FALSE(removed);

	cel_context_destroy(ctx);
}

void test_context_multiple_variables(void)
{
	cel_context_t *ctx = cel_context_create();
	TEST_ASSERT_NOT_NULL(ctx);

	cel_value_t v1 = cel_value_int(1);
	cel_value_t v2 = cel_value_int(2);
	cel_value_t v3 = cel_value_int(3);

	cel_context_add_variable(ctx, "a", &v1);
	cel_context_add_variable(ctx, "b", &v2);
	cel_context_add_variable(ctx, "c", &v3);

	TEST_ASSERT_TRUE(cel_context_has_variable(ctx, "a"));
	TEST_ASSERT_TRUE(cel_context_has_variable(ctx, "b"));
	TEST_ASSERT_TRUE(cel_context_has_variable(ctx, "c"));

	cel_value_t *va = cel_context_get_variable(ctx, "a");
	cel_value_t *vb = cel_context_get_variable(ctx, "b");
	cel_value_t *vc = cel_context_get_variable(ctx, "c");

	TEST_ASSERT_EQUAL_INT64(1, va->value.int_value);
	TEST_ASSERT_EQUAL_INT64(2, vb->value.int_value);
	TEST_ASSERT_EQUAL_INT64(3, vc->value.int_value);

	cel_context_destroy(ctx);
}

/* ========== 作用域链测试 ========== */

void test_context_scope_chain_lookup(void)
{
	cel_context_t *parent = cel_context_create();
	cel_context_t *child = cel_context_create_child(parent);

	/* 在父上下文中添加变量 */
	cel_value_t parent_var = cel_value_int(100);
	cel_context_add_variable(parent, "parent_var", &parent_var);

	/* 在子上下文中添加变量 */
	cel_value_t child_var = cel_value_int(200);
	cel_context_add_variable(child, "child_var", &child_var);

	/* 子上下文可以访问自己的变量 */
	cel_value_t *cv = cel_context_get_variable(child, "child_var");
	TEST_ASSERT_NOT_NULL(cv);
	TEST_ASSERT_EQUAL_INT64(200, cv->value.int_value);

	/* 子上下文可以访问父上下文的变量 */
	cel_value_t *pv = cel_context_get_variable(child, "parent_var");
	TEST_ASSERT_NOT_NULL(pv);
	TEST_ASSERT_EQUAL_INT64(100, pv->value.int_value);

	/* 父上下文不能访问子上下文的变量 */
	cel_value_t *cv_from_parent =
		cel_context_get_variable(parent, "child_var");
	TEST_ASSERT_NULL(cv_from_parent);

	cel_context_destroy(child);
	cel_context_destroy(parent);
}

void test_context_scope_chain_shadowing(void)
{
	cel_context_t *parent = cel_context_create();
	cel_context_t *child = cel_context_create_child(parent);

	/* 在父上下文中添加变量 */
	cel_value_t parent_x = cel_value_int(10);
	cel_context_add_variable(parent, "x", &parent_x);

	/* 在子上下文中添加同名变量 (遮蔽) */
	cel_value_t child_x = cel_value_int(20);
	cel_context_add_variable(child, "x", &child_x);

	/* 子上下文应该看到自己的 x */
	cel_value_t *child_value = cel_context_get_variable(child, "x");
	TEST_ASSERT_NOT_NULL(child_value);
	TEST_ASSERT_EQUAL_INT64(20, child_value->value.int_value);

	/* 父上下文应该仍然看到自己的 x */
	cel_value_t *parent_value = cel_context_get_variable(parent, "x");
	TEST_ASSERT_NOT_NULL(parent_value);
	TEST_ASSERT_EQUAL_INT64(10, parent_value->value.int_value);

	cel_context_destroy(child);
	cel_context_destroy(parent);
}

/* ========== 配置 API 测试 ========== */

void test_context_recursion_depth(void)
{
	cel_context_t *ctx = cel_context_create();
	TEST_ASSERT_NOT_NULL(ctx);

	/* 默认递归深度应该是 100 */
	size_t default_depth = cel_context_get_max_recursion(ctx);
	TEST_ASSERT_EQUAL_UINT(100, default_depth);

	/* 设置新的递归深度 */
	cel_context_set_max_recursion(ctx, 200);
	size_t new_depth = cel_context_get_max_recursion(ctx);
	TEST_ASSERT_EQUAL_UINT(200, new_depth);

	/* 当前深度应该是 0 */
	size_t current = cel_context_get_current_depth(ctx);
	TEST_ASSERT_EQUAL_UINT(0, current);

	cel_context_destroy(ctx);
}

void test_context_recursion_depth_inheritance(void)
{
	cel_context_t *parent = cel_context_create();
	cel_context_set_max_recursion(parent, 150);

	cel_context_t *child = cel_context_create_child(parent);

	/* 子上下文应该继承父上下文的递归深度 */
	size_t child_depth = cel_context_get_max_recursion(child);
	TEST_ASSERT_EQUAL_UINT(150, child_depth);

	cel_context_destroy(child);
	cel_context_destroy(parent);
}

/* ========== 参数验证测试 ========== */

void test_context_add_variable_null_ctx(void)
{
	cel_value_t value = cel_value_int(1);
	cel_error_code_e err = cel_context_add_variable(NULL, "x", &value);
	TEST_ASSERT_EQUAL(CEL_ERROR_INVALID_ARGUMENT, err);
}

void test_context_add_variable_null_name(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t value = cel_value_int(1);
	cel_error_code_e err = cel_context_add_variable(ctx, NULL, &value);
	TEST_ASSERT_EQUAL(CEL_ERROR_INVALID_ARGUMENT, err);
	cel_context_destroy(ctx);
}

void test_context_add_variable_null_value(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_error_code_e err = cel_context_add_variable(ctx, "x", NULL);
	TEST_ASSERT_EQUAL(CEL_ERROR_INVALID_ARGUMENT, err);
	cel_context_destroy(ctx);
}

void test_context_get_variable_null_ctx(void)
{
	cel_value_t *value = cel_context_get_variable(NULL, "x");
	TEST_ASSERT_NULL(value);
}

void test_context_get_variable_null_name(void)
{
	cel_context_t *ctx = cel_context_create();
	cel_value_t *value = cel_context_get_variable(ctx, NULL);
	TEST_ASSERT_NULL(value);
	cel_context_destroy(ctx);
}

/* ========== 主函数 ========== */

int main(void)
{
	UNITY_BEGIN();

	/* 上下文创建/销毁测试 */
	RUN_TEST(test_context_create_empty);
	RUN_TEST(test_context_create);
	RUN_TEST(test_context_create_child);
	RUN_TEST(test_context_destroy_null);

	/* 变量操作测试 */
	RUN_TEST(test_context_add_variable_int);
	RUN_TEST(test_context_get_variable);
	RUN_TEST(test_context_get_variable_not_found);
	RUN_TEST(test_context_update_variable);
	RUN_TEST(test_context_remove_variable);
	RUN_TEST(test_context_multiple_variables);

	/* 作用域链测试 */
	RUN_TEST(test_context_scope_chain_lookup);
	RUN_TEST(test_context_scope_chain_shadowing);

	/* 配置 API 测试 */
	RUN_TEST(test_context_recursion_depth);
	RUN_TEST(test_context_recursion_depth_inheritance);

	/* 参数验证测试 */
	RUN_TEST(test_context_add_variable_null_ctx);
	RUN_TEST(test_context_add_variable_null_name);
	RUN_TEST(test_context_add_variable_null_value);
	RUN_TEST(test_context_get_variable_null_ctx);
	RUN_TEST(test_context_get_variable_null_name);

	return UNITY_END();
}
