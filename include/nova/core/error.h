#pragma once

#include "result.h"
#include "source_location.h"

namespace nova::core {

/// @brief 运行时错误异常
class NovaRuntimeError : public std::runtime_error {
public:
    NovaRuntimeError(const std::string& message, SourceLocation loc = SourceLocation::unknown())
        : std::runtime_error(loc.to_string() + ": " + message)
        , m_location(loc) {}
    
    const SourceLocation& location() const { return m_location; }

private:
    SourceLocation m_location;
};

} // namespace nova::core
