#pragma once

#include <string>

namespace nova {

/// @brief 源代码位置信息，用于错误报告
struct SourceLocation {
    std::string file;       ///< 文件名
    int line = 1;           ///< 行号（从 1 开始）
    int column = 1;         ///< 列号（从 1 开始）
    
    SourceLocation() = default;
    SourceLocation(std::string f, int l, int c) : file(std::move(f)), line(l), column(c) {}
    
    /// @brief 创建默认位置（未知位置）
    static SourceLocation unknown() { return {"<unknown>", 0, 0}; }
    
    /// @brief 格式化为字符串 "file:line:column"
    std::string to_string() const {
        return file + ":" + std::to_string(line) + ":" + std::to_string(column);
    }
};

} // namespace nova
