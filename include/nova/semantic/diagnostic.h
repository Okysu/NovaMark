#pragma once

#include "nova/core/source_location.h"
#include <string>
#include <vector>

namespace nova {

/// @brief 语义分析诊断级别
enum class DiagnosticLevel {
    Error,
    Warning,
    Note
};

/// @brief 语义错误类型
enum class SemanticError {
    UndefinedScene,
    UndefinedVariable,
    UndefinedCharacter,
    UndefinedItem,
    UndefinedLabel,
    DuplicateScene,
    DuplicateVariable,
    DuplicateCharacter,
    DuplicateItem,
    DuplicateLabel,
    InvalidOperand,
    InvalidFunctionCall,
    MissingSuccessBranch,
    MissingFailBranch,
    EmptyChoice,
    UnreachableCode,
    UnusedVariable,
};

/// @brief 单条诊断信息
struct Diagnostic {
    DiagnosticLevel level;
    SemanticError error;
    std::string message;
    SourceLocation location;
    
    std::string to_string() const;
};

/// @brief 诊断收集器
class DiagnosticCollector {
public:
    void error(SemanticError err, std::string message, SourceLocation loc);
    void warning(SemanticError err, std::string message, SourceLocation loc);
    void note(std::string message, SourceLocation loc);
    
    const std::vector<Diagnostic>& diagnostics() const { return m_diagnostics; }
    bool has_errors() const { return m_error_count > 0; }
    size_t error_count() const { return m_error_count; }
    size_t warning_count() const { return m_warning_count; }
    
    void clear();
    std::string to_string() const;
    
private:
    std::vector<Diagnostic> m_diagnostics;
    size_t m_error_count = 0;
    size_t m_warning_count = 0;
};

} // namespace nova
