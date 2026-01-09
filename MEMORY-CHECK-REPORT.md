=== CEL-C 内存检测报告 ===

## 测试结果摘要

- ✅ test_memory: PASS (14 Tests 0 Failures)
- ✅ test_list_map: PASS (22 Tests 0 Failures)
- ✅ test_conversions: PASS (45 Tests 0 Failures)
- ✅ test_lexer: PASS (44 Tests 0 Failures)
- ✅ test_parser: PASS (29 Tests 0 Failures)
- ✅ test_parser_integration: PASS (28 Tests 0 Failures)
- ⚠️ test_macros: LEAK (14 Tests 0 Failures, ==156845== ERROR SUMMARY: 8 errors from 8 contexts (suppressed: 0 from 0))
- ⚠️ test_comprehension: LEAK (11 Tests 0 Failures, ==156858== ERROR SUMMARY: 16 errors from 16 contexts (suppressed: 0 from 0))
- ⚠️ test_functions: LEAK (28 Tests 0 Failures, ==156872== ERROR SUMMARY: 8 errors from 8 contexts (suppressed: 0 from 0))
- ✅ test_program: PASS (15 Tests 0 Failures)
- ✅ test_time: PASS (22 Tests 0 Failures)
- ⚠️ test_regex: LEAK (19 Tests 0 Failures, ==156906== ERROR SUMMARY: 1 errors from 1 contexts (suppressed: 0 from 0))
- ✅ test_context: PASS (19 Tests 0 Failures)

## 已修复的问题

1. test_list_map: 嵌套容器引用计数问题 - 已修复
2. test_parser: Parser 错误对象未释放 - 已添加 cel_parser_cleanup()

## 待修复的问题

1. 求值器中函数参数的生命周期管理 - 临时 list/map 对象未释放
2. 测试代码需要在使用完 cel_value_t 后调用 cel_value_destroy()
