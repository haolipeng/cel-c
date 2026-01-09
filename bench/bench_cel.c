/**
 * @file bench_cel.c
 * @brief CEL-C 性能基准测试
 */

#define _POSIX_C_SOURCE 199309L

#include "cel/cel_value.h"
#include "cel/cel_program.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ITERATIONS 100000
#define WARMUP 1000

static double get_time_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

static void bench_value_creation(void)
{
	printf("\n=== Value Creation Benchmark ===\n");

	/* Warmup */
	for (int i = 0; i < WARMUP; i++) {
		cel_value_t v = cel_value_int(i);
		(void)v;
	}

	/* Int creation */
	double start = get_time_ms();
	for (int i = 0; i < ITERATIONS; i++) {
		cel_value_t v = cel_value_int(i);
		(void)v;
	}
	double elapsed = get_time_ms() - start;
	printf("int creation: %.2f ms for %d ops (%.0f ops/sec)\n",
	       elapsed, ITERATIONS, ITERATIONS / (elapsed / 1000.0));

	/* String creation */
	start = get_time_ms();
	for (int i = 0; i < ITERATIONS; i++) {
		cel_value_t v = cel_value_string("hello world");
		cel_value_destroy(&v);
	}
	elapsed = get_time_ms() - start;
	printf("string creation: %.2f ms for %d ops (%.0f ops/sec)\n",
	       elapsed, ITERATIONS, ITERATIONS / (elapsed / 1000.0));

	/* List creation */
	start = get_time_ms();
	for (int i = 0; i < ITERATIONS / 10; i++) {
		cel_list_t *list = cel_list_create(10);
		for (int j = 0; j < 10; j++) {
			cel_value_t v = cel_value_int(j);
			cel_list_append(list, &v);
		}
		cel_list_release(list);
	}
	elapsed = get_time_ms() - start;
	printf("list creation (10 items): %.2f ms for %d ops (%.0f ops/sec)\n",
	       elapsed, ITERATIONS / 10, (ITERATIONS / 10) / (elapsed / 1000.0));
}

static void bench_expression_eval(void)
{
	printf("\n=== Expression Evaluation Benchmark ===\n");

	const char *expressions[] = {
		"1 + 2",
		"1 + 2 * 3",
		"x + y",
		"x > 0 && y < 100",
	};
	int num_exprs = sizeof(expressions) / sizeof(expressions[0]);

	for (int e = 0; e < num_exprs; e++) {
		cel_compile_result_t compile_result = cel_compile(expressions[e]);
		if (compile_result.has_errors || !compile_result.program) {
			printf("Failed to compile: %s\n", expressions[e]);
			cel_compile_result_destroy(&compile_result);
			continue;
		}

		cel_context_t *ctx = cel_context_create();
		cel_value_t x = cel_value_int(42);
		cel_value_t y = cel_value_int(10);
		cel_context_add_variable(ctx, "x", &x);
		cel_context_add_variable(ctx, "y", &y);

		/* Warmup */
		for (int i = 0; i < WARMUP; i++) {
			cel_execute_result_t result = cel_execute(compile_result.program, ctx);
			cel_execute_result_destroy(&result);
		}

		double start = get_time_ms();
		for (int i = 0; i < ITERATIONS; i++) {
			cel_execute_result_t result = cel_execute(compile_result.program, ctx);
			cel_execute_result_destroy(&result);
		}
		double elapsed = get_time_ms() - start;
		printf("\"%s\": %.2f ms for %d ops (%.0f ops/sec)\n",
		       expressions[e], elapsed, ITERATIONS,
		       ITERATIONS / (elapsed / 1000.0));

		cel_context_destroy(ctx);
		cel_compile_result_destroy(&compile_result);
	}
}

static void bench_string_ops(void)
{
	printf("\n=== String Operations Benchmark ===\n");

	/* String comparison */
	cel_value_t s1 = cel_value_string("hello world");
	cel_value_t s2 = cel_value_string("hello world");

	double start = get_time_ms();
	for (int i = 0; i < ITERATIONS; i++) {
		bool eq = cel_value_equals(&s1, &s2);
		(void)eq;
	}
	double elapsed = get_time_ms() - start;
	printf("string equals: %.2f ms for %d ops (%.0f ops/sec)\n",
	       elapsed, ITERATIONS, ITERATIONS / (elapsed / 1000.0));

	cel_value_destroy(&s1);
	cel_value_destroy(&s2);
}

static void bench_list_ops(void)
{
	printf("\n=== List Operations Benchmark ===\n");

	cel_list_t *list = cel_list_create(1000);
	for (int i = 0; i < 1000; i++) {
		cel_value_t v = cel_value_int(i);
		cel_list_append(list, &v);
	}

	/* List access */
	double start = get_time_ms();
	for (int i = 0; i < ITERATIONS; i++) {
		cel_value_t *v = cel_list_get(list, i % 1000);
		(void)v;
	}
	double elapsed = get_time_ms() - start;
	printf("list get: %.2f ms for %d ops (%.0f ops/sec)\n",
	       elapsed, ITERATIONS, ITERATIONS / (elapsed / 1000.0));

	/* List size */
	start = get_time_ms();
	for (int i = 0; i < ITERATIONS; i++) {
		size_t sz = cel_list_size(list);
		(void)sz;
	}
	elapsed = get_time_ms() - start;
	printf("list size: %.2f ms for %d ops (%.0f ops/sec)\n",
	       elapsed, ITERATIONS, ITERATIONS / (elapsed / 1000.0));

	cel_list_release(list);
}

static void bench_map_ops(void)
{
	printf("\n=== Map Operations Benchmark ===\n");

	cel_map_t *map = cel_map_create(100);
	for (int i = 0; i < 100; i++) {
		char key[32];
		snprintf(key, sizeof(key), "key%d", i);
		cel_value_t k = cel_value_string(key);
		cel_value_t v = cel_value_int(i);
		cel_map_put(map, &k, &v);
		cel_value_destroy(&k);
	}

	/* Map lookup */
	cel_value_t lookup_key = cel_value_string("key50");
	double start = get_time_ms();
	for (int i = 0; i < ITERATIONS; i++) {
		cel_value_t *v = cel_map_get(map, &lookup_key);
		(void)v;
	}
	double elapsed = get_time_ms() - start;
	printf("map get: %.2f ms for %d ops (%.0f ops/sec)\n",
	       elapsed, ITERATIONS, ITERATIONS / (elapsed / 1000.0));

	cel_value_destroy(&lookup_key);
	cel_map_release(map);
}

int main(void)
{
	printf("CEL-C Performance Benchmark\n");
	printf("============================\n");
	printf("Iterations: %d\n", ITERATIONS);

	bench_value_creation();
	bench_string_ops();
	bench_list_ops();
	bench_map_ops();
	bench_expression_eval();

	printf("\n=== Benchmark Complete ===\n");
	return 0;
}
