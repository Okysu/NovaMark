# NovaMark 项目指南

## 项目概述

NovaMark 是一套为文字游戏/视觉小说设计的标记语言及其运行时引擎。

**核心架构**: C++ 核心 VM + 多端哑渲染器（状态驱动）

## 技术决策

| 项目 | 决策 |
|------|------|
| C++ 标准 | C++17 |
| 构建系统 | CMake 3.16+ |
| 包管理 | vcpkg |
| 测试框架 | GoogleTest |
| 错误处理 | 混合策略（编译错误 → `Result<T>`，运行时 → 异常） |
| CLI 工具 | `nova-cli`（build/run/check 子命令） |
| Unicode | 完整支持中文标识符 |
| 类型系统 | 动态类型 |
| AST 格式 | JSON（调试）+ 二进制（运行时） |

## 构建命令

```bash
# 配置项目 (首次或 CMakeLists 变更后)
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake

# 构建全部
cmake --build build --config Release

# 构建单个目标
cmake --build build --target nova-lexer

# 运行全部测试
ctest --test-dir build --output-on-failure

# 运行单个测试
./build/tests/nova-test --gtest_filter=LexerTest.*

# 运行 CLI 工具
./build/src/cli/nova-cli build scripts/main.nvm -o game.nvmp
./build/src/cli/nova-cli run game.nvmp
./build/src/cli/nova-cli check scripts/
```

## 目录结构

```
NovaMark/
├── AGENTS.md              # 本文件
├── CMakeLists.txt         # 根 CMake 配置
├── vcpkg.json             # 依赖清单
├── src/
│   ├── CMakeLists.txt
│   ├── lexer/             # 词法分析器
│   ├── parser/            # 语法分析器
│   ├── ast/               # AST 节点定义
│   ├── semantic/          # 语义分析
│   ├── vm/                # 虚拟机
│   ├── packer/            # 打包工具
│   ├── cli/               # CLI 入口
│   └── core/              # 公共工具（Result, String 等）
├── tests/
│   ├── CMakeLists.txt
│   ├── lexer_test.cpp
│   ├── parser_test.cpp
│   └── ...
├── docs/                  # 设计文档
│   ├── NovaMark 语法规范.md
│   └── NovaMark 引擎架构与渲染指南.md
└── examples/              # 示例 .nvm 文件
```

## 代码风格

### 命名约定

```cpp
// 命名空间: 小写下划线
namespace nova::lexer {}

// 类/结构体: 大驼峰
class Lexer {};
struct Token {};

// 函数/方法: 小写下划线
void tokenize(const std::string& input);
Token next_token();

// 成员变量: 小写下划线，私有加 m_ 前缀
class Foo {
    std::string m_name;      // 私有
public:
    std::string public_data; // 公开
};

// 常量: 全大写下划线
constexpr int MAX_TOKEN_SIZE = 1024;

// 枚举: 大驼峰，枚举值全大写下划线
enum class TokenType {
    Identifier,
    StringLiteral,
    NumberLiteral,
    AtSign,      // @
    Hash,        // #
    Colon,       // :
    Arrow,       // ->
    Eof
};
```

### 文件组织

```cpp
// 头文件: include/namespace/class_name.h
// 源文件: src/namespace/class_name.cpp

// 头文件保护使用 #pragma once
#pragma once

// 头文件顺序: 本项目 → 标准库 → 第三方
#include "core/result.h"
#include "core/string_utils.h"

#include <string>
#include <vector>
#include <memory>

#include <gtest/gtest.h>  // 仅测试文件
```

### 注释规范

```cpp
// 所有公开 API 必须有中文注释
/// @brief 将源代码转换为 Token 流
/// @param source NovaMark 源代码字符串
/// @return 成功返回 Token 列表，失败返回错误信息
Result<std::vector<Token>> tokenize(const std::string& source);

// 复杂逻辑必须有行内注释说明意图
// 骰子表达式: 2d6+3 解析为 {count=2, sides=6, modifier=3}
auto dice = parse_dice_expr(expr);
```

### 错误处理

```cpp
// 编译期错误: 使用 Result<T> 返回错误
template<typename T>
class Result {
    std::variant<T, Error> m_value;
public:
    bool is_ok() const;
    T& unwrap();
    const Error& error() const;
};

// 使用示例
Result<Token> Lexer::next_token() {
    if (at_end()) {
        return Error{ErrorKind::UnexpectedEof, "意外的文件结束", location()};
    }
    return Token{...};
}

// 运行时错误: 使用异常
class NovaRuntimeError : public std::runtime_error {
public:
    NovaRuntimeError(const std::string& msg, SourceLocation loc);
};
```

## 测试规范

### 测试命名

```cpp
// 测试套件: 模块名 + Test
// 测试名: 场景描述（中文可接受）
TEST(LexerTest, TokenizeSimpleDialogue) {
    // Arrange
    std::string source = "林晓: 你好世界";
    
    // Act
    auto result = Lexer(source).tokenize();
    
    // Assert
    ASSERT_TRUE(result.is_ok());
    auto tokens = result.unwrap();
    EXPECT_EQ(tokens.size(), 4);
}
```

### 测试覆盖要求

- 每个 Token 类型必须有独立测试
- 每个语法结构必须有解析测试
- 边界条件（空输入、超长输入、非法字符）必须覆盖
- 错误路径必须有测试（如未闭合字符串）

### 测试数据

```cpp
// 使用 TEST_P 进行参数化测试
class LexerParamTest : public testing::TestWithParam<std::pair<std::string, TokenType>> {};

TEST_P(LexerParamTest, SingleToken) {
    auto [input, expected_type] = GetParam();
    // ...
}

INSTANTIATE_TEST_SUITE_P(
    LexerTokens,
    LexerParamTest,
    testing::Values(
        std::make_pair("@", TokenType::AtSign),
        std::make_pair("#", TokenType::Hash),
        std::make_pair("->", TokenType::Arrow)
    )
);
```

## 外部依赖原则

**最小化依赖**: 仅引入必要的库

| 依赖 | 用途 | 是否参与打包 |
|------|------|--------------|
| GoogleTest | 测试框架 | 否 |
| nlohmann/json | JSON 序列化 | 是（header-only） |

**禁止引入**:
- Boost（过于庞大）
- LLVM（除非编译器复杂度需要）
- Qt（使用标准库替代）

## 开发顺序

1. **Lexer** - 词法分析器，Token 定义
2. **Parser** - 语法分析器，AST 构建
3. **Semantic** - 语义检查（未定义变量、死链接等）
4. **VM** - 虚拟机状态机
5. **Packer** - .nvmp 打包工具
6. **CLI** - nova-cli 命令行工具

## 参考文档

- `docs/NovaMark 语法规范.md` - 完整语法说明
- `docs/NovaMark 引擎架构与渲染指南.md` - 架构设计
