#pragma once

#include <string>
#include <vector>

namespace nova {

/// @brief Unicode 字符串工具类
/// 支持中文等 UTF-8 编码的字符处理
class UString {
public:
    /// @brief 从 UTF-8 字符串构造
    explicit UString(std::string utf8);
    
    /// @brief 获取字符数量（Unicode 码点数）
    size_t length() const { return m_chars.size(); }
    
    /// @brief 获取第 n 个字符
    char32_t operator[](size_t index) const { return m_chars[index]; }
    
    /// @brief 转换回 UTF-8 字符串
    const std::string& to_utf8() const { return m_utf8; }
    
    /// @brief 迭代器支持
    auto begin() const { return m_chars.begin(); }
    auto end() const { return m_chars.end(); }
    
    /// @brief 检查字符是否为空白
    static bool is_whitespace(char32_t c);
    
    /// @brief 检查字符是否为中文字符
    static bool is_chinese(char32_t c);
    
    /// @brief 检查字符是否为标识符首字符（字母、下划线、中文）
    static bool is_ident_start(char32_t c);
    
    /// @brief 检查字符是否为标识符字符（字母、数字、下划线、中文）
    static bool is_ident_char(char32_t c);
    
    /// @brief 检查字符是否为数字
    static bool is_digit(char32_t c);

private:
    std::string m_utf8;
    std::vector<char32_t> m_chars;
};

/// @brief 获取 UTF-8 字符串的字符数量
size_t utf8_length(const std::string& str);

/// @brief 获取 UTF-8 字符串第 n 个字符的字节偏移
size_t utf8_byte_offset(const std::string& str, size_t char_index);

} // namespace nova
