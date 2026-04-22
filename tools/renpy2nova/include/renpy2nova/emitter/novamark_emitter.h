#pragma once

#include "renpy2nova/analyzer/renpy_analyzer.h"
#include "renpy2nova/report/conversion_report.h"

#include <string>

namespace nova::renpy2nova {

/// @brief renpy2nova 为 Ren'Py/Python None 保留的 NovaMark 字符串哨兵值。
inline constexpr char RESERVED_NONE_SENTINEL[] = "__RENPY2NOVA_RESERVED_NONE_SENTINEL__";

/// @brief NovaMark 输出生成器。
class NovamarkEmitter {
public:
    /// @brief 从 Ren'Py 中间表示生成 NovaMark 文本。
    /// @param project Ren'Py 分析结果
    /// @return 生成的 NovaMark 文本与诊断报告
    ConversionResult<std::string> emit(const RenpyProject& project) const;
};

} // namespace nova::renpy2nova
