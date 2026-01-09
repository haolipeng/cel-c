/**
 * @file fuzz_eval.c
 * @brief CEL Evaluator 模糊测试
 */

#include "cel/cel_program.h"
#include "cel/cel_context.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __AFL_FUZZ_TESTCASE_LEN
__AFL_FUZZ_INIT();

int main(void)
{
	__AFL_INIT();
	unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF;

	while (__AFL_LOOP(10000)) {
		int len = __AFL_FUZZ_TESTCASE_LEN;
		char *input = malloc(len + 1);
		if (!input) continue;
		memcpy(input, buf, len);
		input[len] = '\0';

		cel_context_t *ctx = cel_context_create();
		cel_value_t x = cel_value_int(42);
		cel_value_t y = cel_value_string("test");
		cel_context_add_variable(ctx, "x", &x);
		cel_context_add_variable(ctx, "y", &y);

		cel_execute_result_t result = cel_eval_expression(input, ctx);
		cel_execute_result_destroy(&result);

		cel_value_destroy(&y);
		cel_context_destroy(ctx);
		free(input);
	}
	return 0;
}

#else
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size == 0 || size > 10000) {
		return 0;
	}

	char *input = malloc(size + 1);
	if (!input) {
		return 0;
	}
	memcpy(input, data, size);
	input[size] = '\0';

	cel_context_t *ctx = cel_context_create();
	cel_value_t x = cel_value_int(42);
	cel_value_t y = cel_value_string("test");
	cel_context_add_variable(ctx, "x", &x);
	cel_context_add_variable(ctx, "y", &y);

	cel_execute_result_t result = cel_eval_expression(input, ctx);
	cel_execute_result_destroy(&result);

	cel_value_destroy(&y);
	cel_context_destroy(ctx);
	free(input);
	return 0;
}
#endif
