/**
 * @file cel_program.c
 * @brief CEL 程序对象 API 实现
 */

#define _POSIX_C_SOURCE 200809L  /* for strdup */

#include "cel/cel_program.h"
#include "cel/cel_eval.h"
#include <stdlib.h>
#include <string.h>

/* ========== 默认选项 ========== */

cel_compile_options_t cel_default_compile_options(void)
{
	cel_compile_options_t options = {
		.max_recursion_depth = 100,
		.enable_macros = true,
	};
	return options;
}

cel_execute_options_t cel_default_execute_options(void)
{
	cel_execute_options_t options = {
		.max_eval_recursion = 100,
		.timeout_ms = 0,
	};
	return options;
}

/* ========== 编译 API ========== */

cel_compile_result_t cel_compile(const char *source)
{
	return cel_compile_with_options(source, NULL);
}

cel_compile_result_t cel_compile_with_options(const char *source,
					       const cel_compile_options_t *options)
{
	cel_compile_result_t result = {0};

	if (!source) {
		result.has_errors = true;
		result.error_count = 1;
		result.errors = malloc(sizeof(cel_parse_error_t));
		if (result.errors) {
			result.errors->message = strdup("Source code is NULL");
			result.errors->next = NULL;
			memset(&result.errors->location, 0, sizeof(result.errors->location));
		}
		return result;
	}

	/* 使用解析选项 */
	size_t max_recursion = options ? options->max_recursion_depth : 100;
	cel_parse_result_t parse_result = cel_parse_with_options(source, max_recursion);

	if (parse_result.has_errors) {
		result.has_errors = true;
		result.errors = parse_result.errors;
		result.error_count = parse_result.error_count;
		/* AST 不需要，保留错误信息 */
		if (parse_result.ast) {
			cel_ast_destroy(parse_result.ast);
		}
		return result;
	}

	/* 创建程序对象 */
	cel_program_t *program = malloc(sizeof(cel_program_t));
	if (!program) {
		result.has_errors = true;
		result.error_count = 1;
		result.errors = malloc(sizeof(cel_parse_error_t));
		if (result.errors) {
			result.errors->message = strdup("Out of memory");
			result.errors->next = NULL;
		}
		cel_ast_destroy(parse_result.ast);
		return result;
	}

	program->ast = parse_result.ast;
	program->source = strdup(source);
	program->source_length = strlen(source);

	result.program = program;
	result.has_errors = false;
	result.error_count = 0;
	result.errors = NULL;

	return result;
}

void cel_compile_result_destroy(cel_compile_result_t *result)
{
	if (!result) {
		return;
	}

	/* 销毁程序 */
	if (result->program) {
		cel_program_destroy(result->program);
		result->program = NULL;
	}

	/* 销毁错误链表 */
	cel_parse_error_t *err = result->errors;
	while (err) {
		cel_parse_error_t *next = err->next;
		free(err->message);
		free(err);
		err = next;
	}
	result->errors = NULL;
	result->error_count = 0;
	result->has_errors = false;
}

/* ========== 程序管理 ========== */

void cel_program_destroy(cel_program_t *program)
{
	if (!program) {
		return;
	}

	if (program->ast) {
		cel_ast_destroy(program->ast);
		program->ast = NULL;
	}

	if (program->source) {
		free(program->source);
		program->source = NULL;
	}

	free(program);
}

const char *cel_program_get_source(const cel_program_t *program)
{
	if (!program) {
		return NULL;
	}
	return program->source;
}

/* ========== 执行 API ========== */

cel_execute_result_t cel_execute(const cel_program_t *program, cel_context_t *ctx)
{
	return cel_execute_with_options(program, ctx, NULL);
}

cel_execute_result_t cel_execute_with_options(const cel_program_t *program,
					       cel_context_t *ctx,
					       const cel_execute_options_t *options)
{
	cel_execute_result_t result = {0};

	if (!program || !program->ast) {
		result.success = false;
		result.error = cel_error_create(CEL_ERROR_INVALID_ARGUMENT,
						"Program is NULL or invalid");
		return result;
	}

	if (!ctx) {
		result.success = false;
		result.error = cel_error_create(CEL_ERROR_INVALID_ARGUMENT,
						"Context is NULL");
		return result;
	}

	/* 设置执行选项 */
	if (options && options->max_eval_recursion > 0) {
		cel_context_set_max_recursion(ctx, options->max_eval_recursion);
	}

	/* TODO: 实现超时机制 (options->timeout_ms) */

	/* 执行求值 */
	cel_value_t eval_result;
	bool success = cel_eval(program->ast, ctx, &eval_result);

	if (success) {
		result.success = true;
		result.value = eval_result;
		result.error = NULL;
	} else {
		result.success = false;
		result.error = cel_error_create(CEL_ERROR_INTERNAL,
						"Expression evaluation failed");
		result.value = cel_value_null();
	}

	return result;
}

void cel_execute_result_destroy(cel_execute_result_t *result)
{
	if (!result) {
		return;
	}

	if (result->error) {
		cel_error_destroy(result->error);
		result->error = NULL;
	}

	/* 销毁值中的复杂类型 */
	cel_value_destroy(&result->value);
	result->success = false;
}

/* ========== 便捷 API ========== */

cel_execute_result_t cel_eval_expression(const char *source, cel_context_t *ctx)
{
	cel_execute_result_t result = {0};

	/* 编译 */
	cel_compile_result_t compile_result = cel_compile(source);
	if (compile_result.has_errors) {
		result.success = false;
		/* 转换第一个编译错误为执行错误 */
		if (compile_result.errors && compile_result.errors->message) {
			result.error = cel_error_create(CEL_ERROR_SYNTAX,
							compile_result.errors->message);
		} else {
			result.error = cel_error_create(CEL_ERROR_SYNTAX,
							"Compilation failed");
		}
		cel_compile_result_destroy(&compile_result);
		return result;
	}

	/* 执行 */
	result = cel_execute(compile_result.program, ctx);

	/* 清理程序 (不需要保留) */
	cel_program_destroy(compile_result.program);
	compile_result.program = NULL;

	return result;
}

bool cel_check_syntax(const char *source)
{
	if (!source) {
		return false;
	}

	cel_parse_result_t result = cel_parse(source);
	bool valid = !result.has_errors && result.ast != NULL;
	cel_parse_result_destroy(&result);

	return valid;
}
