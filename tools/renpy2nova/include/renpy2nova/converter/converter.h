#pragma once

#include "renpy2nova/report/conversion_report.h"

#include <string>

namespace nova::renpy2nova {

/// @brief 转换器输入参数。
struct ConversionInput {
    std::string source;      ///< Ren'Py 脚本文本
    std::string source_name; ///< 源文件名或显示名
};

/// @brief 转换器输出结果。
struct ConversionOutput {
    std::string novamark_source;  ///< 生成的 NovaMark 文本
    ConversionReport report;      ///< 转换诊断报告
};

/// @brief Ren'Py 到 NovaMark 的独立转换门面。
class Converter {
public:
    /// @brief 执行完整转换管线。
    /// @param input 转换输入
    /// @return 成功时返回转换输出，失败时返回报告错误
    ConversionResult<ConversionOutput> convert(const ConversionInput& input) const;
};

} // namespace nova::renpy2nova
