# CEL-Rust 产品需求文档

## 1. 项目概述

### 1.1 CEL (Common Expression Language) 简介

CEL (Common Expression Language) 是一种**非图灵完备的表达式语言**,专为简单性、速度、安全性和可移植性而设计。CEL 由 Google 开发并开源,其语法类似于 C 语言家族(C++, Go, Java, TypeScript),使得开发者能够快速上手。

**核心特点:**
- **非图灵完备**: 不支持循环和递归,保证表达式执行必然终止
- **类型安全**: 具有强类型系统,支持运行时类型检查
- **跨语言**: 语法在多种语言中保持一致
- **轻量级**: 相比完整的脚本语言(如Lua, JavaScript),资源消耗更小

### 1.2 cel-rust 项目定位

cel-rust 是 CEL 规范的 Rust 语言实现,提供完整的表达式解析、编译和执行能力。项目采用 MIT 许可证开源,当前最新版本为 **0.12.0**。

**主要应用场景:**
1. **访问控制策略**: 验证用户是否有权访问资源
   ```cel
   resource.name.startsWith("/groups/" + auth.claims.group)
   ```

2. **数据验证**: 检查数据是否满足特定条件
   ```cel
   request.time - resource.age < duration("24h")
   ```

3. **配置规则**: 动态配置系统行为
   ```cel
   auth.claims.email_verified && resources.all(r, r.startsWith(auth.claims.email))
   ```

4. **轻量级脚本**: 在不需要完整脚本语言的场景替代 Lua/Python

### 1.3 核心设计目标

1. **简单性**: 语法简洁,易于理解和编写
2. **速度**: 编译一次,多次执行;采用高效的树遍历求值
3. **安全性**: 沙箱隔离,防止恶意代码;非图灵完备防止死循环
4. **可移植性**: 跨平台,与语言无关的规范

### 1.4 与其他表达式语言对比

| 特性 | CEL | Lua | Python eval() | JavaScript eval() |
|------|-----|-----|---------------|-------------------|
| 图灵完备 | 否 | 是 | 是 | 是 |
| 沙箱隔离 | 内建 | 需要额外配置 | 难以实现 | 难以实现 |
| 跨语言标准 | 是 | 否 | 否 | 否 |
| 类型安全 | 强类型 | 动态类型 | 动态类型 | 动态类型 |
| 性能开销 | 低 | 中 | 高 | 高 |
| 适用场景 | 表达式求值 | 通用脚本 | 通用脚本 | 通用脚本 |

---

## 2. 核心功能特性

### 2.1 表达式运算符支持

#### 2.1.1 算术运算符

| 运算符 | 名称 | 支持类型 | 示例 | 说明 |
|--------|------|----------|------|------|
| `+` | 加法/连接 | Int, UInt, Float, String, List, Bytes | `1 + 2` => `3`<br>`"hello" + " world"` => `"hello world"`<br>`[1, 2] + [3]` => `[1, 2, 3]` | 数值相加;字符串/字节/列表连接 |
| `-` | 减法 | Int, UInt, Float, Timestamp, Duration | `5 - 3` => `2`<br>`timestamp - duration` | 数值相减;时间戳减duration |
| `*` | 乘法 | Int, UInt, Float | `3 * 4` => `12` | 数值相乘 |
| `/` | 除法 | Int, UInt, Float | `10 / 2` => `5` | 数值相除(除零报错) |
| `%` | 取模 | Int, UInt | `10 % 3` => `1` | 整数取模(除零报错) |
| `-` (一元) | 取负 | Int, Float | `-5` => `-5` | 数值取反(UInt不支持) |

**类型提升规则:**
- `Int + UInt` => `Int` (如果UInt在Int范围内)
- `Int + Float` => `Float`
- `UInt + Float` => `Float`
- 不同数值类型运算时自动提升到更宽的类型

**示例:**
```cel
// 算术运算
2 + 3 * 4           // 14 (遵循运算符优先级)
(2 + 3) * 4         // 20
10 / 2 - 3          // 2
-5 + 10             // 5

// 字符串连接
"Hello, " + "World!"  // "Hello, World!"

// 列表连接
[1, 2] + [3, 4]      // [1, 2, 3, 4]

// 溢出检测
9223372036854775807 + 1  // 错误: overflow
```

#### 2.1.2 比较运算符

| 运算符 | 名称 | 支持类型 | 示例 | 说明 |
|--------|------|----------|------|------|
| `==` | 等于 | 所有类型 | `1 == 1` => `true` | 值相等比较 |
| `!=` | 不等于 | 所有类型 | `1 != 2` => `true` | 值不等比较 |
| `<` | 小于 | Int, UInt, Float, String, Bytes, Timestamp, Duration | `1 < 2` => `true` | 顺序比较 |
| `<=` | 小于等于 | 同上 | `2 <= 2` => `true` | 顺序比较 |
| `>` | 大于 | 同上 | `3 > 2` => `true` | 顺序比较 |
| `>=` | 大于等于 | 同上 | `3 >= 3` => `true` | 顺序比较 |

**跨类型比较:**
- `Int` 和 `UInt` 可以比较(自动类型兼容)
- `Int/UInt` 和 `Float` 可以比较
- `String` 按字典序比较
- `Bytes` 按字节值比较
- `Timestamp` 和 `Duration` 可比较大小

**示例:**
```cel
// 数值比较
1 == 1u             // true (Int vs UInt)
1.0 == 1            // true (Float vs Int)
1 < 2.5             // true

// 字符串比较
"apple" < "banana"  // true (字典序)
"abc" == "abc"      // true

// 类型不同返回 false
1 == "1"            // false (不同类型)
true == 1           // false

// Null 比较
null == null        // true
null != 1           // true
```

#### 2.1.3 逻辑运算符

| 运算符 | 名称 | 类型 | 示例 | 说明 |
|--------|------|------|------|------|
| `&&` | 逻辑与 | Bool | `true && false` => `false` | 短路求值 |
| `||` | 逻辑或 | Bool | `true || false` => `true` | 短路求值 |
| `!` | 逻辑非 | Bool | `!true` => `false` | 一元运算符 |

**短路求值行为:**
- `false && <expr>`: 不求值 `<expr>`,直接返回 `false`
- `true || <expr>`: 不求值 `<expr>`,直接返回 `true`
- 避免不必要的计算和潜在的错误

**示例:**
```cel
// 基本逻辑运算
true && true         // true
true && false        // false
true || false        // true
!false               // true

// 短路求值
false && (1 / 0 == 0)  // false (不会执行除零)
true || (1 / 0 == 0)   // true (不会执行除零)

// 链式逻辑
a > 0 && b > 0 && c > 0  // 所有条件为真
a == 1 || b == 1 || c == 1  // 至少一个为真
```

#### 2.1.4 成员运算符

| 运算符 | 名称 | 用法 | 示例 | 说明 |
|--------|------|------|------|------|
| `in` | 成员测试 | `<value> in <container>` | `1 in [1, 2, 3]` => `true`<br>`"key" in {"key": "val"}` => `true` | 检查元素是否在容器中 |

**支持的容器类型:**
- **List**: 检查值是否在列表中
- **Map**: 检查键是否在 Map 中
- **String**: 检查子串是否在字符串中

**示例:**
```cel
// 列表成员测试
1 in [1, 2, 3]           // true
4 in [1, 2, 3]           // false

// Map 键测试
"name" in {"name": "Alice", "age": 30}  // true
"email" in {"name": "Alice"}            // false

// 字符串子串测试
"hello" in "hello world"  // true
"xyz" in "hello world"    // false
```

#### 2.1.5 索引和选择运算符

| 运算符 | 名称 | 用法 | 示例 | 说明 |
|--------|------|------|------|------|
| `[]` | 索引访问 | `<container>[<key>]` | `list[0]`<br>`map["key"]` | 访问列表元素或 Map 值 |
| `[?]` | 可选索引 | `<container>[?<key>]` | `map[?"key"]` | 键不存在返回 `optional.none()` |
| `.` | 字段选择 | `<object>.<field>` | `obj.name` | 访问对象字段或方法调用 |
| `.?` | 可选选择 | `<object>.?<field>` | `obj.?name` | 字段不存在返回 `optional.none()` |

**索引访问规则:**
- **List**: 使用 Int/UInt 索引,从 0 开始;越界报错
- **Map**: 使用 Int, UInt, String, Bool 作为键;键不存在报错(除非使用 `[?]`)
- **String**: 不支持直接索引(会报错)

**可选访问 (Optional)**:
cel-rust 0.12.0+ 支持可选语法,需在解析器中启用 `enable_optional_syntax`。

**示例:**
```cel
// 列表索引
[10, 20, 30][0]        // 10
[10, 20, 30][2]        // 30
[10, 20, 30][3]        // 错误: index out of bounds

// Map 访问
{"name": "Alice", "age": 30}["name"]  // "Alice"
{"name": "Alice"}["email"]            // 错误: no such key

// 可选访问
{"name": "Alice"}[?"email"]           // optional.none()
obj.?missingField                     // optional.none()

// 字段选择
person.name                           // 访问 person 的 name 字段
"hello".size()                        // 5 (方法调用)
```

#### 2.1.6 三元条件运算符

| 运算符 | 用法 | 示例 | 说明 |
|--------|------|------|------|
| `? :` | `<cond> ? <true_expr> : <false_expr>` | `x > 0 ? "pos" : "neg"` | 条件表达式 |

**求值规则:**
- 先求值 `<cond>`
- 若为 `true`,求值 `<true_expr>`,返回其值
- 若为 `false`,求值 `<false_expr>`,返回其值
- **懒惰求值**: 只求值被选中的分支

**示例:**
```cel
// 基本条件
x > 0 ? "positive" : "non-positive"

// 嵌套条件
x > 0 ? "pos" : (x < 0 ? "neg" : "zero")

// 懒惰求值
true ? "ok" : (1 / 0)  // "ok" (不会执行除零)
```

### 2.2 数据类型系统

#### 2.2.1 基础类型

##### Int (有符号 64 位整数)

**范围**: `-9223372036854775808` 到 `9223372036854775807`

**字面量语法:**
```cel
42               // 十进制
0x2A             // 十六进制 (42)
-100             // 负数
```

**运算支持**: `+`, `-`, `*`, `/`, `%`, `-` (取负), 所有比较运算符

##### UInt (无符号 64 位整数)

**范围**: `0` 到 `18446744073709551615`

**字面量语法:**
```cel
42u              // 无符号整数(加 'u' 后缀)
0xFFu            // 无符号十六进制
```

**运算支持**: `+`, `-`, `*`, `/`, `%`, 所有比较运算符

**注意**: UInt 不支持一元取负 `-` 运算

##### Float (64 位浮点数)

**遵循 IEEE 754 双精度标准**

**字面量语法:**
```cel
3.14             // 小数
1.99e9           // 科学计数法 (1.99 * 10^9)
.5               // 等价于 0.5
```

**特殊值**: `NaN`, `Inf`, `-Inf`

**运算支持**: `+`, `-`, `*`, `/`, `-` (取负), 所有比较运算符

##### Bool (布尔值)

**字面量**: `true`, `false`

**运算支持**: `&&`, `||`, `!`, `==`, `!=`

##### String (UTF-8 字符串)

**字面量语法:**
```cel
"hello world"
"line1\nline2"      // 支持转义序列
"quote: \"hello\""   // 转义引号
```

**支持的转义序列:**
- `\n` - 换行
- `\r` - 回车
- `\t` - 制表符
- `\\` - 反斜杠
- `\"` - 双引号
- `\xHH` - 十六进制字节 (如 `\x41` => 'A')
- `\uHHHH` - Unicode 码点

**运算支持**:
- `+` (连接)
- `<`, `<=`, `>`, `>=` (字典序比较)
- `==`, `!=`
- `in` (子串测试)

**内置方法**: `.size()`, `.startsWith()`, `.endsWith()`, `.matches()`

##### Bytes (字节数组)

**字面量语法:**
```cel
b"hello"           // UTF-8 编码的字节
b"\x00\xFF"        // 原始字节
```

**运算支持**:
- `+` (连接)
- `<`, `<=`, `>`, `>=` (字节值比较)
- `==`, `!=`
- `in` (子序列测试)

**内置方法**: `.size()`, `.contains()`

##### Null

**字面量**: `null`

**说明**: 表示空值或不存在

**运算支持**: `==`, `!=`

#### 2.2.2 复合类型

##### List (列表)

**动态大小的有序集合,元素可以是任意类型**

**字面量语法:**
```cel
[]                    // 空列表
[1, 2, 3]            // 整数列表
["a", "b", "c"]      // 字符串列表
[1, "two", true]     // 混合类型列表
[1, [2, 3], 4]       // 嵌套列表
```

**可选元素语法 (需启用 optional syntax):**
```cel
[1, ?maybeValue, 3]  // maybeValue 为 none 时跳过
```

**运算支持**:
- `+` (连接列表)
- `==`, `!=` (元素逐一比较)
- `[]` (索引访问)
- `in` (成员测试)

**内置方法**: `.size()`, `.contains()`, `.all()`, `.exists()`, `.map()`, `.filter()`

##### Map (映射/字典)

**键值对集合,键必须是 Int, UInt, String, Bool**

**字面量语法:**
```cel
{}                          // 空 Map
{"name": "Alice", "age": 30}
{1: "one", 2: "two"}        // 整数键
{true: "yes", false: "no"}  // 布尔键
```

**可选键值对语法:**
```cel
{"name": "Alice", ?"email": optionalEmail}  // email 为 none 时跳过
```

**运算支持**:
- `==`, `!=` (键值对比较)
- `[]` (键访问)
- `in` (键存在性测试)

**内置方法**: `.size()`, `.contains()` (检查键)

**键的类型兼容性**: Int 和 UInt 可以互相作为键访问

#### 2.2.3 时间类型 (可选特性 `chrono`)

##### Timestamp (时间戳)

**RFC3339 格式的时间点**

**字面量/构造:**
```cel
timestamp("2025-01-01T00:00:00Z")
timestamp("2025-01-01T15:30:00+08:00")
```

**运算支持**:
- `-` (timestamp - timestamp => duration)
- `-` (timestamp - duration => timestamp)
- `+` (timestamp + duration => timestamp)
- `<`, `<=`, `>`, `>=`, `==`, `!=`

**内置方法**:
- `.getFullYear()` - 获取年份
- `.getMonth()` - 获取月份 (0-11)
- `.getDate()` - 获取日期 (1-31)
- `.getDayOfWeek()` - 获取星期几 (0=Sunday)
- `.getHours()` - 获取小时 (0-23)
- `.getMinutes()` - 获取分钟 (0-59)
- `.getSeconds()` - 获取秒数 (0-59)
- `.getMilliseconds()` - 获取毫秒 (0-999)

##### Duration (时长)

**时间间隔**

**字面量/构造:**
```cel
duration("1h")           // 1 小时
duration("30m")          // 30 分钟
duration("1h30m45s")     // 1 小时 30 分 45 秒
duration("72h3m0.5s")    // 72 小时 3 分 0.5 秒
```

**单位**: `h` (小时), `m` (分钟), `s` (秒), `ms` (毫秒), `us` (微秒), `ns` (纳秒)

**运算支持**:
- `+`, `-` (duration 加减)
- `<`, `<=`, `>`, `>=`, `==`, `!=`

**内置方法**:
- `.getHours()`, `.getMinutes()`, `.getSeconds()`, `.getMilliseconds()`

#### 2.2.4 扩展类型

##### Optional (可选值)

**表示可能存在或不存在的值**

**构造函数:**
```cel
optional.none()                  // 空值
optional.of(42)                  // 包含 42 的 optional
optional.ofNonZeroValue(0)       // none (0 为零值)
optional.ofNonZeroValue(42)      // optional.of(42)
```

**内置方法:**
```cel
optValue.hasValue()              // 检查是否有值
optValue.value()                 // 提取值(无值则报错)
optValue.or(otherOpt)            // 返回第一个有值的 optional
optValue.orValue(defaultVal)     // 返回值或默认值
```

**示例:**
```cel
// 安全访问
map[?"missing_key"].orValue("default")  // "default"

// 链式调用
obj.?field.?subfield.orValue(null)
```

##### Opaque (自定义不透明类型)

**用户定义的扩展类型,通过 Rust 端注册**

**特性:**
- 类型名称唯一标识
- 支持相等性比较(同类型且数据相同)
- 可序列化为 JSON (可选)

---

### 2.3 内置函数库

#### 2.3.1 集合操作函数

##### `size(collection) -> Int`

**功能**: 返回集合的大小

**参数**: List, Map, String, Bytes

**示例:**
```cel
size([1, 2, 3])              // 3
size({"a": 1, "b": 2})       // 2
size("hello")                // 5
size(b"\x00\xFF")            // 2
```

##### `contains(collection, value) -> Bool`

**功能**: 检查集合是否包含指定值

**参数**:
- **List**: 检查元素是否在列表中
- **Map**: 检查键是否存在
- **String**: 检查子串
- **Bytes**: 检查子序列

**示例:**
```cel
contains([1, 2, 3], 2)                    // true
contains({"a": 1}, "a")                   // true
contains("hello world", "world")          // true
```

#### 2.3.2 字符串操作函数

##### `startsWith(string, prefix) -> Bool`

**功能**: 检查字符串是否以指定前缀开头

**示例:**
```cel
"hello world".startsWith("hello")  // true
"hello".startsWith("world")        // false
```

##### `endsWith(string, suffix) -> Bool`

**功能**: 检查字符串是否以指定后缀结尾

**示例:**
```cel
"hello world".endsWith("world")    // true
"hello".endsWith("world")          // false
```

##### `matches(string, regex) -> Bool` (需要 `regex` 特性)

**功能**: 检查字符串是否匹配正则表达式

**示例:**
```cel
"hello123".matches("hello\\d+")    // true
"test@example.com".matches(".*@.*\\.com")  // true
```

##### `string(value) -> String`

**功能**: 将值转换为字符串

**支持类型**: Int, UInt, Float, Bool, Bytes, String

**示例:**
```cel
string(42)       // "42"
string(true)     // "true"
string(3.14)     // "3.14"
```

#### 2.3.3 类型转换函数

##### `int(value) -> Int`

**功能**: 转换为有符号整数

**支持**: String, UInt, Float, Int

**示例:**
```cel
int("42")        // 42
int(42u)         // 42
int(3.14)        // 3
int("abc")       // 错误: 无效格式
```

##### `uint(value) -> UInt`

**功能**: 转换为无符号整数

**支持**: String, Int, Float, UInt

**示例:**
```cel
uint("42")       // 42u
uint(42)         // 42u
uint(-1)         // 错误: 负数无法转换
```

##### `double(value) -> Float`

**功能**: 转换为浮点数

**支持**: String, Int, UInt, Float

**示例:**
```cel
double("3.14")   // 3.14
double(42)       // 42.0
```

##### `bytes(string) -> Bytes`

**功能**: 将字符串转换为字节数组 (UTF-8 编码)

**示例:**
```cel
bytes("hello")   // b"hello"
```

#### 2.3.4 聚合函数

##### `max(values...) -> Value`

**功能**: 返回最大值

**参数**: 可变参数,支持 Int, UInt, Float, String, 等可比较类型

**示例:**
```cel
max(1, 5, 3)                    // 5
max("apple", "banana", "cherry")  // "cherry"
```

##### `min(values...) -> Value`

**功能**: 返回最小值

**参数**: 同 `max`

**示例:**
```cel
min(1, 5, 3)      // 1
```

#### 2.3.5 Optional 操作函数

##### `optional.none() -> Optional`

**功能**: 创建空 Optional

##### `optional.of(value) -> Optional`

**功能**: 将值包装为 Optional

##### `optional.ofNonZeroValue(value) -> Optional`

**功能**: 仅当值非零时包装(零值返回 none)

##### `<optional>.hasValue() -> Bool`

**功能**: 检查 Optional 是否有值

##### `<optional>.value() -> Value`

**功能**: 提取 Optional 的值(无值则报错)

##### `<optional>.or(other) -> Optional`

**功能**: 返回第一个有值的 Optional

##### `<optional>.orValue(default) -> Value`

**功能**: 返回 Optional 的值或默认值

**示例:**
```cel
optional.of(42).hasValue()           // true
optional.none().hasValue()           // false
optional.of(42).value()              // 42
optional.none().orValue(100)         // 100
optional.of(42).or(optional.of(100))  // optional.of(42)
```

#### 2.3.6 时间操作函数 (需要 `chrono` 特性)

##### `timestamp(string) -> Timestamp`

**功能**: 解析 RFC3339 时间戳

**示例:**
```cel
timestamp("2025-01-01T00:00:00Z")
```

##### `duration(string) -> Duration`

**功能**: 解析时长字符串

**示例:**
```cel
duration("1h30m")       // 1.5 小时
duration("24h")         // 1 天
```

##### 时间戳方法

```cel
ts.getFullYear()        // 2025
ts.getMonth()           // 0 (January)
ts.getDate()            // 1
ts.getDayOfWeek()       // 3 (Wednesday)
ts.getHours()           // 15
ts.getMinutes()         // 30
ts.getSeconds()         // 45
ts.getMilliseconds()    // 123
```

##### 时长方法

```cel
dur.getHours()          // 从 duration 提取小时部分
dur.getMinutes()        // 提取分钟部分
dur.getSeconds()        // 提取秒部分
dur.getMilliseconds()   // 提取毫秒部分
```

---

### 2.4 宏系统(列表推导)

cel-rust 支持高级的宏系统,这些宏在解析阶段展开为 **Comprehension** 表达式。

#### 2.4.1 `has(map.field)` - 字段存在性检查

**功能**: 检查 Map 或对象是否包含指定字段

**语法**: `has(<map>.<field>)`

**示例:**
```cel
has({"name": "Alice"}.name)        // true
has({"name": "Alice"}.email)       // false
```

**展开形式**: 转换为 `"field" in <map>`

#### 2.4.2 `<list>.all(var, predicate)` - 全称量词

**功能**: 检查列表中所有元素是否满足条件

**语法**: `<list>.all(<var>, <predicate>)`

**示例:**
```cel
[1, 2, 3].all(x, x > 0)            // true
[1, 2, 3].all(x, x > 2)            // false
["a", "ab", "abc"].all(s, s.size() > 0)  // true
```

**展开形式**:
```cel
// 伪代码
accumulator = true
for x in list:
    if not predicate(x):
        accumulator = false
        break
return accumulator
```

#### 2.4.3 `<list>.exists(var, predicate)` - 存在量词

**功能**: 检查列表中是否存在满足条件的元素

**语法**: `<list>.exists(<var>, <predicate>)`

**示例:**
```cel
[1, 2, 3].exists(x, x > 2)         // true (3 > 2)
[1, 2, 3].exists(x, x > 10)        // false
```

#### 2.4.4 `<list>.exists_one(var, predicate)` - 唯一性量词

**功能**: 检查列表中是否恰好有一个元素满足条件

**语法**: `<list>.exists_one(<var>, <predicate>)`

**示例:**
```cel
[1, 2, 3].exists_one(x, x == 2)    // true (只有 2 满足)
[1, 2, 2].exists_one(x, x == 2)    // false (两个 2)
```

#### 2.4.5 `<list>.map(var, transform)` - 映射转换

**功能**: 对列表中每个元素应用转换函数

**语法**: `<list>.map(<var>, <transform>)`

**示例:**
```cel
[1, 2, 3].map(x, x * 2)            // [2, 4, 6]
["a", "b"].map(s, s + "!")         // ["a!", "b!"]
```

**三参数形式** (带过滤):
```cel
<list>.map(<var>, <filter>, <transform>)
```

**示例:**
```cel
[1, 2, 3, 4].map(x, x > 2, x * 10)  // [30, 40] (只对 >2 的元素)
```

#### 2.4.6 `<list>.filter(var, predicate)` - 过滤

**功能**: 筛选满足条件的元素

**语法**: `<list>.filter(<var>, <predicate>)`

**示例:**
```cel
[1, 2, 3, 4, 5].filter(x, x % 2 == 0)  // [2, 4]
["a", "ab", "abc"].filter(s, s.size() > 1)  // ["ab", "abc"]
```

#### 2.4.7 对 Map 使用宏

**Map 的 .map() 方法**:

对 Map 使用 `.map()` 会遍历键值对。

**示例:**
```cel
{"a": 1, "b": 2}.map(k, k + "!")   // 对键应用转换
```

---

### 2.5 高级特性

#### 2.5.1 自定义函数注册

用户可以通过 Rust API 注册自定义函数供 CEL 表达式调用。

**Rust 端 API:**
```rust
context.add_function("myFunc", |a: i64, b: i64| a + b);
```

**CEL 中调用:**
```cel
myFunc(10, 20)  // 30
```

**函数签名支持:**
- 基本类型: `i64`, `u64`, `f64`, `bool`, `Arc<String>`, `Arc<Vec<u8>>`
- CEL 值类型: `Value`
- 特殊提取器: `This<T>` (方法接收者), `Arguments` (所有参数), `Identifier` (标识符名称)

#### 2.5.2 自定义类型 (Opaque)

通过 `Opaque` trait 可以扩展 CEL 支持的类型。

**Rust 端实现:**
```rust
struct MyType { data: String }

impl Opaque for MyType {
    fn runtime_type_name(&self) -> &str {
        "MyType"
    }
}

// 注册到上下文
context.add_variable("myObj", Value::Opaque(Arc::new(MyType { data: "...".to_string() })));
```

**CEL 中使用:**
```cel
myObj == myObj  // 相等性比较
```

#### 2.5.3 变量解析器 (VariableResolver)

动态提供变量值,而无需预先添加到上下文。

**Rust 端实现:**
```rust
struct MyResolver;

impl VariableResolver for MyResolver {
    fn resolve_variable(&self, name: &str) -> Option<Value> {
        match name {
            "dynamicVar" => Some(42.into()),
            _ => None,
        }
    }
}

context.set_variable_resolver(Box::new(MyResolver));
```

**CEL 中使用:**
```cel
dynamicVar + 10  // 52 (变量从 resolver 获取)
```

#### 2.5.4 上下文嵌套 (作用域链)

支持父子上下文,实现变量作用域和shadowing。

**用途**:
- 宏展开时为循环变量创建子作用域
- 隔离临时变量

**Rust 端 API:**
```rust
let child_ctx = context.new_inner_scope();
child_ctx.add_variable("loopVar", value);
```

#### 2.5.5 可选语法 (Optional Syntax)

需要在解析器中启用:

**Rust 端配置:**
```rust
Parser::new()
    .enable_optional_syntax(true)
    .parse(source)
```

**支持的语法:**
- `obj.?field` - 可选字段访问
- `list[?index]` - 可选索引
- `{?"key": value}` - 可选 Map 键值对
- `[?element]` - 可选列表元素
- `Message{?field: value}` - 可选结构体字段

**示例:**
```cel
// 安全链式访问
user.?address.?city.orValue("Unknown")

// 可选列表元素
[1, ?null, 3]           // [1, 3] (跳过 null)

// 可选 Map 键
{a: 1, ?maybeKey: maybeValue}  // 当 maybeValue 为 none 时跳过
```

#### 2.5.6 注释支持

cel-rust 0.12.0+ 支持 `//` 风格单行注释。

**示例:**
```cel
// 这是注释
1 + 2  // 行尾注释
```

---

## 3. 解析器规范

### 3.1 词法规则

#### 3.1.1 标识符

**规则**: 以字母或下划线开头,后跟字母、数字或下划线

**合法**: `var`, `myVar`, `_internal`, `var123`

**非法**: `123var`, `var-name`

#### 3.1.2 关键字

**保留关键字**:
- `true`, `false`, `null`
- (宏关键字不是保留字,可作为标识符)

#### 3.1.3 字面量格式

**整数**:
- 十进制: `123`, `-456`
- 十六进制: `0x1A`, `0XFF`
- 无符号: `123u`, `0xFFu`

**浮点数**:
- 标准: `3.14`, `-2.5`
- 科学计数: `1.5e10`, `1E-5`
- 省略整数部分: `.5` (等价于 `0.5`)

**字符串**:
- 双引号: `"hello"`
- 转义: `"line1\nline2"`

**字节**:
- 前缀 `b`: `b"hello"`, `b"\x00\xFF"`

#### 3.1.4 运算符与符号

**单字符**: `+`, `-`, `*`, `/`, `%`, `<`, `>`, `!`, `?`, `:`, `.`, `,`, `(`, `)`, `[`, `]`, `{`, `}`

**多字符**: `==`, `!=`, `<=`, `>=`, `&&`, `||`, `.?`, `[?]`

### 3.2 语法规则

#### 3.2.1 运算符优先级 (从高到低)

1. **成员访问/索引**: `.`, `[]`, `()`
2. **一元运算符**: `-`, `!`
3. **乘除模**: `*`, `/`, `%`
4. **加减**: `+`, `-`
5. **比较**: `<`, `<=`, `>`, `>=`
6. **相等性**: `==`, `!=`
7. **成员测试**: `in`
8. **逻辑与**: `&&`
9. **逻辑或**: `||`
10. **三元条件**: `? :`

#### 3.2.2 结合性

**左结合**: 大多数二元运算符 (`+`, `-`, `*`, `/`, `&&`, `||`, 等)

**右结合**: 三元条件运算符 (`? :`)

**示例:**
```cel
1 + 2 + 3        // (1 + 2) + 3 (左结合)
a ? b : c ? d : e  // a ? b : (c ? d : e) (右结合)
```

### 3.3 宏展开规则

宏在解析阶段自动展开为 `ComprehensionExpr` (推导式表达式)。

**展开时机**: 语法分析阶段

**作用域**: 宏内部的循环变量仅在宏内可见

### 3.4 错误处理与报告

#### 3.4.1 解析错误

**错误类型**:
- 语法错误: 不符合 CEL 语法规则
- 词法错误: 无效的字符或token

**错误信息包含**:
- 错误描述
- 源代码位置 (行号、列号)
- 错误代码片段

**示例错误:**
```
ParseError at line 1, column 5:
  1 + * 2
      ^
Expected expression, found '*'
```

#### 3.4.2 多错误报告

cel-rust 支持一次报告多个解析错误:

**Rust API:**
```rust
match Program::compile(source) {
    Err(ParseErrors(errors)) => {
        for err in errors {
            println!("{}", err);
        }
    }
    Ok(program) => { ... }
}
```

### 3.5 可配置选项

#### 3.5.1 递归深度限制

**目的**: 防止过深的嵌套表达式导致栈溢出

**默认值**: 96

**配置:**
```rust
Parser::new()
    .max_recursion_depth(50)
    .parse(source)
```

#### 3.5.2 可选语法开关

**目的**: 启用/禁用可选访问语法 (`?.`, `[?]`)

**默认值**: 禁用

**配置:**
```rust
Parser::new()
    .enable_optional_syntax(true)
    .parse(source)
```

---

## 4. 执行模型

### 4.1 编译-执行两阶段模型

**阶段 1: 编译**

输入: CEL 源代码字符串

输出: `Program` 对象(包含 AST)

```rust
let program = Program::compile("x + y")?;
```

**阶段 2: 执行**

输入: `Program` + `Context`

输出: 求值结果 `Value`

```rust
let mut context = Context::default();
context.add_variable("x", 10.into());
context.add_variable("y", 20.into());
let result = program.execute(&context)?;  // 30
```

**优势:**
- **编译一次,多次执行**: 同一个 `Program` 可用不同 `Context` 重复执行
- **线程安全**: `Program` 可跨线程共享
- **性能**: 避免每次执行都重新解析

### 4.2 AST 表示与遍历求值

**AST 节点类型:**
- `Literal` - 字面量
- `Ident` - 变量引用
- `Call` - 函数调用 (包括运算符)
- `Select` - 字段访问
- `List` - 列表字面量
- `Map` - Map 字面量
- `Comprehension` - 推导式 (宏展开后)
- `Struct` - 结构体字面量

**求值方式**: 递归树遍历 (tree-walking interpreter)

**伪代码:**
```
function eval(node, context):
    match node:
        Literal(value):
            return value
        Ident(name):
            return context.lookup_variable(name)
        Call(func_name, args):
            evaluated_args = [eval(arg, context) for arg in args]
            func = context.lookup_function(func_name)
            return func(evaluated_args)
        ...
```

### 4.3 短路求值策略

**逻辑运算符:**
- `&&`: 左侧为 `false` 时不求值右侧
- `||`: 左侧为 `true` 时不求值右侧

**三元运算符:**
- `cond ? true_expr : false_expr`: 根据 `cond` 只求值一个分支

**优势:**
- 性能优化(避免不必要的计算)
- 避免错误 (如 `false && (1/0 == 0)` 不会除零)

### 4.4 类型检查时机 (运行时)

cel-rust 执行**运行时类型检查**,而非静态类型检查。

**类型错误检测时机**: 执行时发现类型不匹配

**示例:**
```cel
"hello" + 123  // 运行时报错: type mismatch
```

**未来可能支持**: 静态类型推断和检查(当前版本不支持)

### 4.5 错误处理机制

**错误类型 (`ExecutionError`):**
- `UndeclaredReference` - 未声明的变量或函数
- `NoSuchKey` - Map 键不存在
- `IndexOutOfBounds` - 列表索引越界
- `DivideByZero` - 除零错误
- `Overflow` - 整数溢出
- `TypeError` - 类型不匹配
- `NoSuchOverload` - 函数重载不匹配
- `FunctionError` - 函数执行错误

**错误传播**: 使用 Rust 的 `Result` 类型,错误向上传播

**错误恢复**: 执行失败时返回 `Err`,不支持部分求值

### 4.6 线程安全要求

**线程安全组件:**
- `Program` - 可在多线程间共享 (实现 `Send + Sync`)
- `Value` - 使用 `Arc` 共享不可变数据

**线程局部组件:**
- `Context` - 每个线程独立创建,不跨线程共享

**示例:**
```rust
use std::thread;

let program = Arc::new(Program::compile("x * 2")?);

let handles: Vec<_> = (0..10).map(|i| {
    let program = Arc::clone(&program);
    thread::spawn(move || {
        let mut context = Context::default();
        context.add_variable("x", i.into());
        program.execute(&context)
    })
}).collect();

for handle in handles {
    println!("{:?}", handle.join());
}
```

---

## 5. 扩展机制

### 5.1 函数注册 API

**简单函数:**
```rust
context.add_function("add", |a: i64, b: i64| a + b);
```

**返回 Result:**
```rust
context.add_function("safe_div", |a: i64, b: i64| {
    if b == 0 {
        Err(ExecutionError::DivideByZero)
    } else {
        Ok(a / b)
    }
});
```

**方法调用 (使用 `This<T>`):**
```rust
context.add_function("double", |This(x): This<i64>| x * 2);
```

**CEL 中调用:**
```cel
(5).double()  // 10
```

**可变参数 (使用 `Arguments`):**
```rust
context.add_function("sum", |Arguments(args): Arguments| {
    let mut total = 0i64;
    for arg in args {
        if let Value::Int(n) = arg {
            total += n;
        }
    }
    total
});
```

### 5.2 自定义类型接口

**实现 `Opaque` trait:**
```rust
use cel::Opaque;
use std::any::Any;

#[derive(Debug)]
struct Point { x: i64, y: i64 }

impl Opaque for Point {
    fn runtime_type_name(&self) -> &str {
        "Point"
    }

    fn json(&self) -> Option<serde_json::Value> {
        Some(json!({"x": self.x, "y": self.y}))
    }
}

// 注册
context.add_variable("origin", Value::Opaque(Arc::new(Point { x: 0, y: 0 })));
```

**CEL 中使用:**
```cel
origin == origin  // true (相等性比较)
```

### 5.3 变量解析器接口

**实现 `VariableResolver` trait:**
```rust
struct EnvResolver;

impl VariableResolver for EnvResolver {
    fn resolve_variable(&self, name: &str) -> Option<Value> {
        std::env::var(name).ok().map(|v| Value::String(Arc::new(v)))
    }
}

context.set_variable_resolver(Box::new(EnvResolver));
```

**CEL 中使用:**
```cel
HOME + "/documents"  // 从环境变量获取 HOME
```

### 5.4 Serde 集成支持

**自动转换 Rust 结构体为 CEL 值:**
```rust
use serde::Serialize;
use cel::ser::to_value;

#[derive(Serialize)]
struct User {
    name: String,
    age: u32,
}

let user = User { name: "Alice".to_string(), age: 30 };
let value = to_value(&user)?;  // 转换为 CEL Map
context.add_variable("user", value);
```

**CEL 中使用:**
```cel
user.name          // "Alice"
user.age > 18      // true
```

### 5.5 JSON 转换支持 (需要 `json` 特性)

**CEL 值导出为 JSON:**
```rust
use cel::json::to_json;

let value = program.execute(&context)?;
let json = to_json(&value)?;
println!("{}", serde_json::to_string_pretty(&json)?);
```

**JSON 导入为 CEL 值:**
```rust
let json = json!({"name": "Alice", "scores": [95, 87, 92]});
let value = cel::ser::to_value(&json)?;
context.add_variable("data", value);
```

---

## 6. 性能与安全要求

### 6.1 非图灵完备保证

**限制:**
- 不支持无限循环 (无 `while`, `for` 等循环语句)
- 不支持递归函数调用
- 推导式 (Comprehension) 仅对有限集合迭代

**保证**: 任何 CEL 表达式执行必然终止

### 6.2 执行时间/深度限制

**递归深度限制**:
- 解析阶段限制表达式嵌套深度(默认 96)
- 防止栈溢出

**未来改进**: 可增加执行步数限制(当前版本未实现)

### 6.3 内存使用约束

**当前实现:**
- 使用 `Arc` 共享大对象(字符串、列表、Map)
- 避免不必要的克隆

**未来改进**:
- 内存池分配器
- 内存使用上限配置

### 6.4 并发安全性

**保证:**
- `Program` 线程安全,可跨线程共享
- `Value` 线程安全(内部使用 `Arc`)

**注意:**
- `Context` 非线程安全,每个线程独立创建

### 6.5 沙箱隔离要求

**隔离机制:**
- 表达式无法访问文件系统、网络或系统调用
- 仅能访问 `Context` 中提供的变量和函数
- 自定义函数由使用者控制权限

**安全建议:**
- 不要将不受信任的函数注册到 `Context`
- 使用 `VariableResolver` 时限制可访问的变量范围

---

## 7. 使用示例

### 7.1 基本表达式求值

```rust
use cel::{Program, Context};

let program = Program::compile("2 + 3 * 4")?;
let context = Context::default();
let result = program.execute(&context)?;
assert_eq!(result, 14.into());
```

### 7.2 变量使用

```rust
let program = Program::compile("name + ' is ' + string(age) + ' years old'")?;
let mut context = Context::default();
context.add_variable("name", "Alice".into());
context.add_variable("age", 30.into());
let result = program.execute(&context)?;
// result: "Alice is 30 years old"
```

### 7.3 自定义函数

```rust
let program = Program::compile("add(x, y) * 2")?;
let mut context = Context::default();
context.add_function("add", |a: i64, b: i64| a + b);
context.add_variable("x", 5.into());
context.add_variable("y", 10.into());
let result = program.execute(&context)?;
// result: 30
```

### 7.4 列表推导

```rust
let program = Program::compile("[1, 2, 3, 4, 5].filter(x, x % 2 == 0).map(x, x * 10)")?;
let context = Context::default();
let result = program.execute(&context)?;
// result: [20, 40]
```

### 7.5 时间处理

```rust
let program = Program::compile(
    "timestamp('2025-01-01T00:00:00Z') + duration('24h')"
)?;
let context = Context::default();
let result = program.execute(&context)?;
// result: Timestamp("2025-01-02T00:00:00Z")
```

### 7.6 Serde 集成

```rust
use serde::Serialize;

#[derive(Serialize)]
struct Product {
    name: String,
    price: f64,
    in_stock: bool,
}

let product = Product {
    name: "Widget".to_string(),
    price: 19.99,
    in_stock: true,
};

let mut context = Context::default();
context.add_variable("product", cel::ser::to_value(&product)?);

let program = Program::compile("product.in_stock && product.price < 20.0")?;
let result = program.execute(&context)?;
// result: true
```

---

## 8. 特性标志 (Cargo Features)

cel-rust 通过 Cargo features 控制可选功能:

| Feature | 默认 | 说明 |
|---------|------|------|
| `default` | 是 | 包含 `regex` 和 `chrono` |
| `regex` | 是 | 启用 `matches()` 函数 |
| `chrono` | 是 | 启用 Timestamp 和 Duration 类型及相关函数 |
| `json` | 否 | 启用 JSON 导入/导出功能 |
| `bytes` | 否 | 集成 `bytes::Bytes` 类型 |

**自定义配置示例:**
```toml
[dependencies]
cel = { version = "0.12.0", default-features = false, features = ["regex"] }
```

---

## 9. 版本历史

**当前版本**: 0.12.0 (2025-12-29)

**主要功能演进:**

- **0.12.0**: Optional 类型支持,Opaque 类型,注释支持
- **0.11.x**: 递归深度限制,性能优化,错误处理改进
- **0.10.0**: ANTLR4 解析器,宏系统,Comprehension
- **0.9.x**: 短路求值,Duration/Timestamp 支持,JSON 转换
- **0.8.x 及更早**: 基础功能实现

---

## 10. 总结

cel-rust 提供了完整的 CEL 实现,涵盖:

**核心功能:**
- 完整的运算符支持(算术、逻辑、比较、成员)
- 丰富的数据类型(基础类型、复合类型、时间类型、扩展类型)
- 40+ 内置函数
- 强大的宏系统(列表推导)
- 可选值 (Optional) 支持

**扩展能力:**
- 自定义函数注册
- 自定义类型 (Opaque)
- 动态变量解析
- Serde 集成
- JSON 转换

**工程特性:**
- 编译-执行两阶段模型
- 线程安全
- 沙箱隔离
- 非图灵完备保证
- 完善的错误处理

**适用场景:**
- 访问控制策略
- 数据验证
- 配置规则
- 轻量级表达式求值

cel-rust 为将 CEL 移植到 C 语言提供了完整的功能参考和规范基础。
