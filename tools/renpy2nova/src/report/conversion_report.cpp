#include "renpy2nova/report/conversion_report.h"

#include <algorithm>
#include <sstream>
#include <utility>

namespace nova::renpy2nova {
namespace {

void append_json_escaped(std::ostringstream& out, const std::string& value) {
    for (unsigned char ch : value) {
        switch (ch) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (ch < 0x20) {
                    static const char* HEX = "0123456789ABCDEF";
                    out << "\\u00"
                        << HEX[(ch >> 4U) & 0x0FU]
                        << HEX[ch & 0x0FU];
                } else {
                    out << static_cast<char>(ch);
                }
                break;
        }
    }
}

void append_json_string_field(std::ostringstream& out,
                              const char* key,
                              const std::string& value,
                              bool trailing_comma = true,
                              int indent = 0) {
    out << std::string(static_cast<size_t>(indent), ' ') << '"' << key << "\": \"";
    append_json_escaped(out, value);
    out << '"';
    if (trailing_comma) {
        out << ',';
    }
    out << '\n';
}

void append_json_numeric_field(std::ostringstream& out,
                               const char* key,
                               size_t value,
                               bool trailing_comma = true,
                               int indent = 0) {
    out << std::string(static_cast<size_t>(indent), ' ') << '"' << key << "\": " << value;
    if (trailing_comma) {
        out << ',';
    }
    out << '\n';
}

} // namespace

ConversionLineRange ConversionLineRange::single(size_t line, size_t column) {
    return ConversionLineRange{line, line, column, column};
}

ConversionLineRange ConversionLineRange::range(size_t start_line, size_t end_line) {
    return ConversionLineRange{start_line, end_line, 1, 1};
}

ConversionEntry ConversionEntry::single_line(ConversionEntryKind kind,
                                              ConversionSeverity severity,
                                              std::string feature_tag,
                                              std::string reason,
                                              std::string action,
                                              std::string original_text,
                                              size_t line) {
    return ConversionEntry{
        kind, severity, std::move(reason), std::move(action),
        std::move(original_text), ConversionLineRange::single(line), std::move(feature_tag)};
}

void ConversionReport::add(ConversionSeverity severity, std::string message, size_t line, size_t column) {
    m_diagnostics.push_back(ConversionDiagnostic{severity, std::move(message), line, column});
}

bool ConversionReport::has_errors() const {
    return std::any_of(m_diagnostics.begin(), m_diagnostics.end(), [](const ConversionDiagnostic& diagnostic) {
        return diagnostic.severity == ConversionSeverity::Error;
    });
}

const std::vector<ConversionDiagnostic>& ConversionReport::diagnostics() const {
    return m_diagnostics;
}

void ConversionReport::add_entry(ConversionEntry entry) {
    m_entries.push_back(std::move(entry));
}

const std::vector<ConversionEntry>& ConversionReport::entries() const {
    return m_entries;
}

std::vector<ConversionEntry> ConversionReport::entries_by_kind(ConversionEntryKind kind) const {
    std::vector<ConversionEntry> result;
    for (const auto& entry : m_entries) {
        if (entry.kind == kind) {
            result.push_back(entry);
        }
    }
    return result;
}

bool ConversionReport::needs_manual_intervention() const {
    return std::any_of(m_entries.begin(), m_entries.end(), [](const ConversionEntry& entry) {
        return entry.kind == ConversionEntryKind::Unsupported ||
               entry.kind == ConversionEntryKind::ManualFixRequired;
    });
}

ConversionSummary ConversionReport::summary() const {
    ConversionSummary s;
    s.total_entries = m_entries.size();

    for (const auto& entry : m_entries) {
        switch (entry.kind) {
            case ConversionEntryKind::FullySupported:    ++s.fully_supported; break;
            case ConversionEntryKind::PartiallySupported: ++s.partially_supported; break;
            case ConversionEntryKind::Unsupported:       ++s.unsupported; break;
            case ConversionEntryKind::ManualFixRequired:  ++s.manual_fix_required; break;
        }
        switch (entry.severity) {
            case ConversionSeverity::Error:   ++s.error_count;   break;
            case ConversionSeverity::Warning: ++s.warning_count; break;
            case ConversionSeverity::Info:    ++s.info_count;    break;
        }
    }

    for (const auto& diagnostic : m_diagnostics) {
        switch (diagnostic.severity) {
            case ConversionSeverity::Error:   ++s.error_count;   break;
            case ConversionSeverity::Warning: ++s.warning_count; break;
            case ConversionSeverity::Info:    ++s.info_count;    break;
        }
    }

    return s;
}

const char* conversion_severity_to_string(ConversionSeverity severity) {
    switch (severity) {
        case ConversionSeverity::Info: return "info";
        case ConversionSeverity::Warning: return "warning";
        case ConversionSeverity::Error: return "error";
    }

    return "unknown";
}

const char* conversion_entry_kind_to_string(ConversionEntryKind kind) {
    switch (kind) {
        case ConversionEntryKind::FullySupported: return "fully_supported";
        case ConversionEntryKind::PartiallySupported: return "partially_supported";
        case ConversionEntryKind::Unsupported: return "unsupported";
        case ConversionEntryKind::ManualFixRequired: return "manual_fix_required";
    }

    return "unknown";
}

std::string serialize_report_json(const ConversionReport& report, const std::string& source_name) {
    const ConversionSummary summary = report.summary();
    std::ostringstream out;
    out << "{\n";
    append_json_string_field(out, "source_name", source_name, true, 2);
    out << "  \"summary\": {\n";
    append_json_numeric_field(out, "total_entries", summary.total_entries, true, 4);
    append_json_numeric_field(out, "fully_supported", summary.fully_supported, true, 4);
    append_json_numeric_field(out, "partially_supported", summary.partially_supported, true, 4);
    append_json_numeric_field(out, "unsupported", summary.unsupported, true, 4);
    append_json_numeric_field(out, "manual_fix_required", summary.manual_fix_required, true, 4);
    append_json_numeric_field(out, "error_count", summary.error_count, true, 4);
    append_json_numeric_field(out, "warning_count", summary.warning_count, true, 4);
    append_json_numeric_field(out, "info_count", summary.info_count, false, 4);
    out << "  },\n";

    out << "  \"diagnostics\": [\n";
    for (size_t index = 0; index < report.diagnostics().size(); ++index) {
        const auto& diagnostic = report.diagnostics()[index];
        out << "    {\n";
        append_json_string_field(out, "severity", conversion_severity_to_string(diagnostic.severity), true, 6);
        append_json_string_field(out, "message", diagnostic.message, true, 6);
        append_json_numeric_field(out, "line", diagnostic.line, true, 6);
        append_json_numeric_field(out, "column", diagnostic.column, false, 6);
        out << "    }";
        if (index + 1 < report.diagnostics().size()) {
            out << ',';
        }
        out << '\n';
    }
    out << "  ],\n";

    out << "  \"entries\": [\n";
    for (size_t index = 0; index < report.entries().size(); ++index) {
        const auto& entry = report.entries()[index];
        out << "    {\n";
        append_json_string_field(out, "kind", conversion_entry_kind_to_string(entry.kind), true, 6);
        append_json_string_field(out, "severity", conversion_severity_to_string(entry.severity), true, 6);
        append_json_string_field(out, "feature_tag", entry.feature_tag, true, 6);
        append_json_string_field(out, "reason", entry.reason, true, 6);
        append_json_string_field(out, "action", entry.action, true, 6);
        append_json_string_field(out, "original_text", entry.original_text, true, 6);
        out << "      \"source_range\": {\n";
        append_json_numeric_field(out, "start_line", entry.source_range.start_line, true, 8);
        append_json_numeric_field(out, "end_line", entry.source_range.end_line, true, 8);
        append_json_numeric_field(out, "start_column", entry.source_range.start_column, true, 8);
        append_json_numeric_field(out, "end_column", entry.source_range.end_column, false, 8);
        out << "      }\n";
        out << "    }";
        if (index + 1 < report.entries().size()) {
            out << ',';
        }
        out << '\n';
    }
    out << "  ]\n";
    out << "}\n";
    return out.str();
}

} // namespace nova::renpy2nova
