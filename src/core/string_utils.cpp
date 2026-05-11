#include "nova/core/string_utils.h"
#include <algorithm>

namespace nova {

namespace {
    // 检查一个字节是否为 UTF-8 连续字节 (10xxxxxx)
    bool is_utf8_continuation(unsigned char c) {
        return (c & 0xC0) == 0x80;
    }
    
    // 解码单个 UTF-8 字符，返回码点和消耗的字节数
    std::pair<char32_t, size_t> decode_utf8_char(const std::string& str, size_t pos) {
        if (pos >= str.size()) return {0, 0};
        
        unsigned char c0 = static_cast<unsigned char>(str[pos]);
        
        // ASCII (0xxxxxxx)
        if ((c0 & 0x80) == 0) {
            return {static_cast<char32_t>(c0), 1};
        }
        
        // 2 字节序列 (110xxxxx 10xxxxxx)
        if ((c0 & 0xE0) == 0xC0) {
            if (pos + 1 >= str.size()) return {0, 1};
            unsigned char c1 = static_cast<unsigned char>(str[pos + 1]);
            if (!is_utf8_continuation(c1)) return {0, 1};
            char32_t cp = ((c0 & 0x1F) << 6) | (c1 & 0x3F);
            return {cp, 2};
        }
        
        // 3 字节序列 (1110xxxx 10xxxxxx 10xxxxxx)
        if ((c0 & 0xF0) == 0xE0) {
            if (pos + 2 >= str.size()) return {0, 1};
            unsigned char c1 = static_cast<unsigned char>(str[pos + 1]);
            unsigned char c2 = static_cast<unsigned char>(str[pos + 2]);
            if (!is_utf8_continuation(c1) || !is_utf8_continuation(c2)) return {0, 1};
            char32_t cp = ((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
            return {cp, 3};
        }
        
        // 4 字节序列 (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        if ((c0 & 0xF8) == 0xF0) {
            if (pos + 3 >= str.size()) return {0, 1};
            unsigned char c1 = static_cast<unsigned char>(str[pos + 1]);
            unsigned char c2 = static_cast<unsigned char>(str[pos + 2]);
            unsigned char c3 = static_cast<unsigned char>(str[pos + 3]);
            if (!is_utf8_continuation(c1) || !is_utf8_continuation(c2) || !is_utf8_continuation(c3)) {
                return {0, 1};
            }
            char32_t cp = ((c0 & 0x07) << 18) | ((c1 & 0x3F) << 12) | 
                          ((c2 & 0x3F) << 6) | (c3 & 0x3F);
            return {cp, 4};
        }
        
        // 无效的 UTF-8 序列
        return {0, 1};
    }
}

UString::UString(std::string utf8) : m_utf8(std::move(utf8)) {
    // 解码 UTF-8 为码点数组
    size_t pos = 0;
    while (pos < m_utf8.size()) {
        auto [cp, len] = decode_utf8_char(m_utf8, pos);
        if (len == 0) break;
        m_chars.push_back(cp);
        pos += len;
    }
}

bool UString::is_whitespace(char32_t c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool UString::is_chinese(char32_t c) {
    // CJK 统一汉字范围
    return (c >= 0x4E00 && c <= 0x9FFF) ||
           (c >= 0x3400 && c <= 0x4DBF) ||
           (c >= 0x20000 && c <= 0x2A6DF) ||
           (c >= 0x2A700 && c <= 0x2B73F);
}

bool UString::is_ident_start(char32_t c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_' ||
           is_chinese(c);
}

bool UString::is_ident_char(char32_t c) {
    return is_ident_start(c) || is_digit(c);
}

bool UString::is_digit(char32_t c) {
    return c >= '0' && c <= '9';
}

size_t utf8_length(const std::string& str) {
    return UString(str).length();
}

size_t utf8_byte_offset(const std::string& str, size_t char_index) {
    size_t char_count = 0;
    size_t byte_pos = 0;
    
    while (byte_pos < str.size() && char_count < char_index) {
        auto [cp, len] = decode_utf8_char(str, byte_pos);
        if (len == 0) break;
        byte_pos += len;
        ++char_count;
    }
    
    return byte_pos;
}

} // namespace nova
