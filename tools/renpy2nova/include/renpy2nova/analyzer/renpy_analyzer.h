#pragma once

#include "renpy2nova/lexer/renpy_lexer.h"
#include "renpy2nova/report/conversion_report.h"

#include <string>
#include <vector>

namespace nova::renpy2nova {

/// @brief v0 转换阶段的 Ren'Py 中间节点类型。
enum class RenpyNodeKind {
    CharacterDefinition,
    DefaultStatement,
    If,
    Elif,
    Else,
    Label,
    Jump,
    Call,
    Return,
    Say,
    Narration,
    Menu,
    MenuChoice,
    Scene,
    Show,
    Hide,
    PlayMusic,
    PlaySound,
    DollarStatement,
    PythonBlock,
    InitPythonBlock,
    With,
    Unsupported,
};

/// @brief Ren'Py 分析后的结构化节点。
struct RenpyNode {
    RenpyNodeKind kind = RenpyNodeKind::Unsupported;  ///< 节点类型
    RenpyUnsupportedKind unsupported_kind = RenpyUnsupportedKind::None; ///< 不支持构造分类
    std::string name;                                 ///< 名称字段，如标签名、变量名、说话人名
    std::string value;                                ///< 值字段，如表达式、文本、参数串
    size_t line = 1;                                  ///< 1 起始行号
    size_t column = 1;                                ///< 1 起始列号
    size_t indent = 0;                                ///< 缩进宽度
    std::vector<RenpyNode> children;                  ///< 缩进块中的子节点
    std::vector<RenpyNode> else_children;             ///< else / elif 链的保守分支内容
};

/// @brief Ren'Py 分析后的项目结构。
struct RenpyProject {
    std::vector<RenpyToken> tokens;      ///< 词法阶段产出的 Token 流
    std::vector<RenpyNode> statements;   ///< 结构化语句树
};

/// @brief 分析 Ren'Py Token 流并生成转换中间表示。
class RenpyAnalyzer {
public:
    /// @brief 执行 v0 分析流程。
    /// @param tokens Ren'Py Token 流
    /// @return 分析后的项目结构与诊断报告
    ConversionResult<RenpyProject> analyze(std::vector<RenpyToken> tokens) const;
};

} // namespace nova::renpy2nova
