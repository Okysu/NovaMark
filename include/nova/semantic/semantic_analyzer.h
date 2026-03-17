#pragma once

#include "nova/ast/ast_node.h"
#include "nova/semantic/diagnostic.h"
#include "nova/semantic/symbol_table.h"
#include "nova/core/result.h"
#include <memory>

namespace nova {

/// @brief 语义分析结果
struct SemanticAnalysisResult {
    bool success;
    DiagnosticCollector diagnostics;
    SymbolTable symbol_table;
};

/// @brief 语义分析器
class SemanticAnalyzer {
public:
    SemanticAnalyzer();
    
    /// @brief 分析 AST
    SemanticAnalysisResult analyze(const ProgramNode* program);
    
    /// @brief 分析 AST（智能指针版本）
    SemanticAnalysisResult analyze(const AstPtr& program);
    
private:
    DiagnosticCollector m_diagnostics;
    SymbolTable m_symbols;
    
    /// @brief 第一遍：收集所有定义
    void collect_definitions(const ProgramNode* program);
    
    /// @brief 第二遍：检查所有引用
    void check_references(const ProgramNode* program);
    
    /// @brief 第三遍：控制流和结构检查
    void check_structures(const ProgramNode* program);
    
    /// @brief 第四遍：检查未使用的符号
    void check_unused_symbols();
    
    /// @brief 访问节点（分发）
    void visit_node(const AstNode* node);
    
    /// @brief 收集单个节点中的定义
    void collect_from_node(const AstNode* node);
    
    /// @brief 检查单个节点中的引用
    void check_node_references(const AstNode* node);
    
    /// @brief 检查单个节点的结构
    void check_node_structure(const AstNode* node);
    
    /// @brief 检查表达式中的引用
    void check_expression(const AstNode* expr);
    
    /// @brief 检查变量引用
    void check_variable_ref(const std::string& name, SourceLocation loc);
    
    /// @brief 检查场景跳转
    void check_jump_target(const std::string& target, SourceLocation loc);
    
    /// @brief 检查角色引用
    void check_character_ref(const std::string& name, SourceLocation loc);
    
    /// @brief 检查物品引用
    void check_item_ref(const std::string& name, SourceLocation loc);
    
    /// @brief 检查函数调用
    void check_function_call(const std::string& name, 
                             const std::vector<AstNode*>& args,
                             SourceLocation loc);
};

} // namespace nova
