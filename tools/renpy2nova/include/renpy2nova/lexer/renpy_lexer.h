#pragma once

#include "renpy2nova/report/conversion_report.h"

#include <string>
#include <vector>

namespace nova::renpy2nova {

/// @brief v0 转换阶段识别的 Ren'Py 行级 Token 类型。
enum class RenpyTokenType {
    Blank,
    Comment,
    DefineCharacter,
    DefaultStatement,
    IfStatement,
    ElifStatement,
    ElseStatement,
    Label,
    Jump,
    Call,
    Return,
    Say,
    Narrator,
    Menu,
    MenuChoice,
    Scene,
    Show,
    Hide,
    PlayMusic,
    PlaySound,
    DollarStatement,
    PythonBlockStart,
    InitPythonBlockStart,
    WithStatement,
    UnsupportedConstruct,
    Unknown,
    Eof,
};

/// @brief 当前阶段显式分类的不支持 Ren'Py 构造。
enum class RenpyUnsupportedKind {
    None,
    Screen,
    Transform,
    With,
    Image,
    Audio,
    Unknown,
};

/// @brief Ren'Py 行级 Token。
struct RenpyToken {
    RenpyTokenType type = RenpyTokenType::Unknown;  ///< Token 类型
    std::string lexeme;                             ///< 原始文本（不含行尾换行）
    std::string primary;                           ///< 主字段，如名称、目标、文本或关键字
    std::string secondary;                         ///< 次字段，如表达式或参数
    RenpyUnsupportedKind unsupported_kind = RenpyUnsupportedKind::None; ///< 不支持构造分类
    size_t line = 1;                               ///< 1 起始行号
    size_t column = 1;                             ///< 1 起始列号
    size_t indent = 0;                             ///< 缩进宽度（空格计数，Tab 记为 4）
};

/// @brief Ren'Py 脚本词法分析器。
class RenpyLexer {
public:
    /// @brief 将 Ren'Py 源文本切分为 v0 可识别的行级 Token 流。
    /// @param source Ren'Py 脚本文本
    /// @return 成功返回 Token 列表，失败返回转换诊断
    ConversionResult<std::vector<RenpyToken>> tokenize(const std::string& source) const;
};

} // namespace nova::renpy2nova
