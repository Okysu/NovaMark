#pragma once

#include <string>
#include <utility>
#include <vector>

namespace nova::renpy2nova {

/// @brief 转换诊断级别。
enum class ConversionSeverity {
    Info,
    Warning,
    Error,
};

/// @brief 转换条目类别（用于 v0 降级跟踪）。
enum class ConversionEntryKind {
    FullySupported,
    PartiallySupported,
    Unsupported,
    ManualFixRequired,
};

/// @brief 源代码行号范围。
struct ConversionLineRange {
    size_t start_line = 1;
    size_t end_line = 1;
    size_t start_column = 1;
    size_t end_column = 1;

    static ConversionLineRange single(size_t line, size_t column = 1);

    static ConversionLineRange range(size_t start_line, size_t end_line);
};

/// @brief 转换诊断信息。
struct ConversionDiagnostic {
    ConversionSeverity severity;
    std::string message;
    size_t line = 1;
    size_t column = 1;
};

/// @brief Ren'Py → NovaMark 转换中的结构化降级条目。
/// 用于记录不支持/部分支持的语法特性，以便后续阶段生成 inline TODO 或人工修正指引。
struct ConversionEntry {
    ConversionEntryKind kind;
    ConversionSeverity severity;
    std::string reason;
    std::string action;
    std::string original_text;
    ConversionLineRange source_range;
    std::string feature_tag;

    static ConversionEntry single_line(ConversionEntryKind kind,
                                       ConversionSeverity severity,
                                       std::string feature_tag,
                                       std::string reason,
                                       std::string action,
                                       std::string original_text,
                                       size_t line);
};

/// @brief 转换报告摘要：按类别和级别汇总的统计信息。
struct ConversionSummary {
    size_t total_entries = 0;
    size_t fully_supported = 0;
    size_t partially_supported = 0;
    size_t unsupported = 0;
    size_t manual_fix_required = 0;
    size_t error_count = 0;
    size_t warning_count = 0;
    size_t info_count = 0;
};

/// @brief 转换过程报告。
class ConversionReport {
public:
    void add(ConversionSeverity severity, std::string message, size_t line = 1, size_t column = 1);

    bool has_errors() const;

    const std::vector<ConversionDiagnostic>& diagnostics() const;

    void add_entry(ConversionEntry entry);

    const std::vector<ConversionEntry>& entries() const;

    std::vector<ConversionEntry> entries_by_kind(ConversionEntryKind kind) const;

    bool needs_manual_intervention() const;

    ConversionSummary summary() const;

private:
    std::vector<ConversionDiagnostic> m_diagnostics;
    std::vector<ConversionEntry> m_entries;
};

/// @brief 将转换诊断级别转换为稳定的机器可读字符串。
/// @param severity 诊断级别
/// @return 小写英文标识（info/warning/error）
const char* conversion_severity_to_string(ConversionSeverity severity);

/// @brief 将转换条目类别转换为稳定的机器可读字符串。
/// @param kind 条目类别
/// @return 小写下划线英文标识
const char* conversion_entry_kind_to_string(ConversionEntryKind kind);

/// @brief 将转换报告序列化为结构化 JSON 文本。
/// @param report 转换报告
/// @param source_name 源文件名或显示名
/// @return 适合写入文件的 JSON 字符串
std::string serialize_report_json(const ConversionReport& report, const std::string& source_name);

/// @brief 带转换报告的结果类型。
/// @tparam T 成功值类型
template<typename T>
class ConversionResult {
public:
    ConversionResult(T value, ConversionReport report = {})
        : m_is_ok(true), m_value(std::move(value)), m_report(std::move(report)) {}

    explicit ConversionResult(ConversionReport report)
        : m_is_ok(false), m_report(std::move(report)) {}

    bool is_ok() const { return m_is_ok; }

    bool is_err() const { return !m_is_ok; }

    T& unwrap() { return m_value; }

    const T& unwrap() const { return m_value; }

    ConversionReport& report() { return m_report; }

    const ConversionReport& report() const { return m_report; }

private:
    bool m_is_ok = false;
    T m_value{};
    ConversionReport m_report;
};

} // namespace nova::renpy2nova
