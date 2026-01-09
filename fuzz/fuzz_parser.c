/**
 * @file fuzz_parser.c
 * @brief CEL Parser 模糊测试
 *
 * 使用 libFuzzer 或 AFL 进行模糊测试。
 * 编译: clang -g -fsanitize=fuzzer,address fuzz_parser.c -I../include -L../build/src -lcel_static -lm -lsds
 */

#include "cel/cel_parser.h"
#include "cel/cel_program.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __AFL_FUZZ_TESTCASE_LEN
/* AFL 模式 */
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

		cel_compile_result_t result = cel_compile(input);
		cel_compile_result_destroy(&result);

		free(input);
	}
	return 0;
}

#else
/* libFuzzer 模式 */
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

	cel_compile_result_t result = cel_compile(input);
	cel_compile_result_destroy(&result);

	free(input);
	return 0;
}
#endif
