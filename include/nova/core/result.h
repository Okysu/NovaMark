#pragma once

#include "source_location.h"
#include <string>
#include <variant>
#include <optional>

namespace nova {

/// @brief 错误类型枚举
enum class ErrorKind {
    // 词法错误
    UnexpectedChar,         ///< 意外的字符
    UnexpectedEof,          ///< 意外的文件结束
    UnterminatedString,     ///< 未闭合的字符串
    InvalidEscape,          ///< 无效的转义序列
    InvalidNumber,          ///< 无效的数字格式
    
    // 语法错误
    UnexpectedToken,        ///< 意外的 Token
    ExpectedToken,          ///< 期望的 Token 未出现
    InvalidSyntax,          ///< 无效的语法结构
    
    // 语义错误
    UndefinedVariable,      ///< 未定义的变量
    UndefinedScene,         ///< 未定义的场景
    UndefinedCharacter,     ///< 未定义的角色
    DuplicateDefinition,    ///< 重复定义
    
    // 运行时错误
    DivisionByZero,         ///< 除零错误
    TypeMismatch,           ///< 类型不匹配
    IndexError,             ///< 索引越界
    
    // 系统错误
    FileNotFound,           ///< 文件未找到
    IOError,                ///< 输入输出错误
};

/// @brief 将错误类型转换为可读字符串
inline const char* error_kind_str(ErrorKind kind) {
    switch (kind) {
        case ErrorKind::UnexpectedChar:     return "unexpected character";
        case ErrorKind::UnexpectedEof:      return "unexpected end of file";
        case ErrorKind::UnterminatedString: return "unterminated string";
        case ErrorKind::InvalidEscape:      return "invalid escape sequence";
        case ErrorKind::InvalidNumber:      return "invalid number format";
        case ErrorKind::UnexpectedToken:    return "unexpected token";
        case ErrorKind::ExpectedToken:      return "expected token not found";
        case ErrorKind::InvalidSyntax:      return "invalid syntax";
        case ErrorKind::UndefinedVariable:  return "undefined variable";
        case ErrorKind::UndefinedScene:     return "undefined scene";
        case ErrorKind::UndefinedCharacter: return "undefined character";
        case ErrorKind::DuplicateDefinition: return "duplicate definition";
        case ErrorKind::DivisionByZero:     return "division by zero";
        case ErrorKind::TypeMismatch:       return "type mismatch";
        case ErrorKind::IndexError:         return "index out of range";
        case ErrorKind::FileNotFound:       return "file not found";
        case ErrorKind::IOError:            return "I/O error";
    }
    return "unknown error";
}

/// @brief 错误信息结构
struct Error {
    ErrorKind kind;                 ///< 错误类型
    std::string message;            ///< 详细错误信息
    SourceLocation location;        ///< 错误位置
    
    /// @brief 格式化为完整的错误信息
    std::string to_string() const {
        return location.to_string() + ": error: " + 
               error_kind_str(kind) + " - " + message;
    }
};

/// @brief 用于操作失败的返回类型
/// @tparam T 成功时包含的值类型
template<typename T>
class Result {
public:
    /// @brief 从成功值构造
    Result(T value) : m_data(std::move(value)) {}
    
    /// @brief 从错误构造
    Result(Error error) : m_data(std::move(error)) {}
    
    /// @brief 检查是否成功
    bool is_ok() const { return std::holds_alternative<T>(m_data); }
    
    /// @brief 检查是否失败
    bool is_err() const { return std::holds_alternative<Error>(m_data); }
    
    /// @brief 获取成功值（需先检查 is_ok）
    T& unwrap() & { return std::get<T>(m_data); }
    
    /// @brief 获取成功值（需先检查 is_ok）
    const T& unwrap() const & { return std::get<T>(m_data); }
    
    /// @brief 获取成功值（需先检查 is_ok）
    T&& unwrap() && { return std::get<T>(std::move(m_data)); }
    
    /// @brief 获取错误信息（需先检查 is_err）
    const Error& error() const { return std::get<Error>(m_data); }
    
    /// @brief 获取值或默认值
    T unwrap_or(T default_value) const {
        return is_ok() ? unwrap() : std::move(default_value);
    }
    
    /// @brief 映射成功值
    template<typename F>
    auto map(F&& f) -> Result<std::invoke_result_t<F, T>> {
        if (is_ok()) {
            return Result<std::invoke_result_t<F, T>>(f(unwrap()));
        }
        return Result<std::invoke_result_t<F, T>>(error());
    }

private:
    std::variant<T, Error> m_data;
};

/// @brief void 类型的 Result 特化（无返回值的操作）
template<>
class Result<void> {
public:
    /// @brief 构造成功结果
    Result() : m_error(std::nullopt) {}
    
    /// @brief 从错误构造
    Result(Error error) : m_error(std::move(error)) {}
    
    /// @brief 检查是否成功
    bool is_ok() const { return !m_error.has_value(); }
    
    /// @brief 检查是否失败
    bool is_err() const { return m_error.has_value(); }
    
    /// @brief 获取错误信息（需先检查 is_err）
    const Error& error() const { return *m_error; }

private:
    std::optional<Error> m_error;
};

/// @brief 创建成功结果的便捷函数
template<typename T>
Result<T> Ok(T value) { return Result<T>(std::move(value)); }

/// @brief 创建 void 成功结果
inline Result<void> Ok() { return Result<void>(); }

/// @brief 创建错误结果的便捷函数
template<typename T>
Result<T> Err(ErrorKind kind, std::string message, SourceLocation loc = SourceLocation::unknown()) {
    return Result<T>(Error{kind, std::move(message), loc});
}

} // namespace nova
