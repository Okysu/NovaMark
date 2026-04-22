#include "../../include/renpy2nova/converter/project_converter.h"

#include "../../include/renpy2nova/analyzer/renpy_analyzer.h"
#include "../../include/renpy2nova/converter/converter.h"
#include "../../include/renpy2nova/lexer/renpy_lexer.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace nova::renpy2nova {
namespace {

namespace fs = filesystem_compat;

constexpr const char* AGGREGATE_REPORT_FILE = "reports/aggregate_report.json";
constexpr const char* MANUAL_FIX_MANIFEST_FILE = "manifests/manual_fix_manifest.json";
constexpr const char* RESOURCE_MANIFEST_FILE = "manifests/resource_manifest.json";

std::string to_generic_string(const fs::path& path) {
    return path.generic_string();
}

void ensure_parent_directory(const fs::path& path) {
    const fs::path parent = path.parent_path();
    if (!parent.empty()) {
        fs::create_directories(parent);
    }
}

std::string read_text_file(const fs::path& path) {
    std::ifstream file(path, std::ios::in);
    if (!file) {
        throw std::runtime_error("failed to open file: " + path.string());
    }

    return std::string{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

void write_text_file(const fs::path& path, const std::string& content) {
    ensure_parent_directory(path);

    std::ofstream file(path, std::ios::out | std::ios::binary);
    if (!file) {
        throw std::runtime_error("failed to write file: " + path.string());
    }

    file << content;
}

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
                if (ch < 0x20U) {
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
                              bool trailing_comma,
                              int indent) {
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
                               bool trailing_comma,
                               int indent) {
    out << std::string(static_cast<size_t>(indent), ' ') << '"' << key << "\": " << value;
    if (trailing_comma) {
        out << ',';
    }
    out << '\n';
}

void append_json_bool_field(std::ostringstream& out,
                            const char* key,
                            bool value,
                            bool trailing_comma,
                            int indent) {
    out << std::string(static_cast<size_t>(indent), ' ') << '"' << key << "\": "
        << (value ? "true" : "false");
    if (trailing_comma) {
        out << ',';
    }
    out << '\n';
}

void append_summary_object(std::ostringstream& out, const ConversionSummary& summary, int indent) {
    const std::string prefix(static_cast<size_t>(indent), ' ');
    out << prefix << "{\n";
    append_json_numeric_field(out, "total_entries", summary.total_entries, true, indent + 2);
    append_json_numeric_field(out, "fully_supported", summary.fully_supported, true, indent + 2);
    append_json_numeric_field(out, "partially_supported", summary.partially_supported, true, indent + 2);
    append_json_numeric_field(out, "unsupported", summary.unsupported, true, indent + 2);
    append_json_numeric_field(out, "manual_fix_required", summary.manual_fix_required, true, indent + 2);
    append_json_numeric_field(out, "error_count", summary.error_count, true, indent + 2);
    append_json_numeric_field(out, "warning_count", summary.warning_count, true, indent + 2);
    append_json_numeric_field(out, "info_count", summary.info_count, false, indent + 2);
    out << prefix << '}';
}

ConversionSummary accumulate_summary(ConversionSummary total, const ConversionSummary& part) {
    total.total_entries += part.total_entries;
    total.fully_supported += part.fully_supported;
    total.partially_supported += part.partially_supported;
    total.unsupported += part.unsupported;
    total.manual_fix_required += part.manual_fix_required;
    total.error_count += part.error_count;
    total.warning_count += part.warning_count;
    total.info_count += part.info_count;
    return total;
}

std::vector<fs::path> collect_project_files(const fs::path& project_root) {
    if (!fs::exists(project_root)) {
        throw std::runtime_error("project directory does not exist: " + project_root.string());
    }
    if (!fs::is_directory(project_root)) {
        throw std::runtime_error("project path is not a directory: " + project_root.string());
    }

    std::vector<fs::path> files;
    for (const auto& entry : fs::recursive_directory_iterator(project_root)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() != ".rpy") {
            continue;
        }

        files.push_back(fs::relative(entry.path(), project_root));
    }

    std::sort(files.begin(), files.end(), [](const fs::path& lhs, const fs::path& rhs) {
        return lhs.generic_string() < rhs.generic_string();
    });
    return files;
}

fs::path make_output_relative_path(const fs::path& source_relative_path) {
    fs::path output_relative_path = source_relative_path;
    output_relative_path.replace_extension(".nvm");
    return fs::path("scripts") / output_relative_path;
}

fs::path make_report_relative_path(const fs::path& source_relative_path) {
    fs::path report_relative_path = source_relative_path;
    report_relative_path.replace_extension(".json");
    return fs::path("reports") / "file_reports" / report_relative_path;
}

void collect_manifest_items(const ProjectFileConversionResult& file_result,
                            const ConversionReport& report,
                            std::vector<ProjectManifestItem>& manual_fix_items,
                            std::vector<ProjectManifestItem>& review_items,
                            bool include_review_items) {
    for (const auto& entry : report.entries()) {
        switch (entry.kind) {
            case ConversionEntryKind::Unsupported:
            case ConversionEntryKind::ManualFixRequired:
                manual_fix_items.push_back(ProjectManifestItem{
                    file_result.source_relative_path,
                    file_result.report_relative_path,
                    entry,
                });
                break;
            case ConversionEntryKind::PartiallySupported:
                if (include_review_items) {
                    review_items.push_back(ProjectManifestItem{
                        file_result.source_relative_path,
                        file_result.report_relative_path,
                        entry,
                    });
                }
                break;
            case ConversionEntryKind::FullySupported:
                break;
        }
    }
}

std::string serialize_aggregate_report_json(const ProjectConversionOutput& output) {
    std::ostringstream out;
    out << "{\n";
    append_json_string_field(out, "project_root", to_generic_string(output.project_root), true, 2);
    append_json_string_field(out, "output_dir", to_generic_string(output.output_dir), true, 2);
    out << "  \"summary\": {\n";
    append_json_numeric_field(out, "total_files", output.totals.total_files, true, 4);
    append_json_numeric_field(out, "converted_files", output.totals.converted_files, true, 4);
    append_json_numeric_field(out, "failed_files", output.totals.failed_files, true, 4);
    append_json_numeric_field(out, "files_with_manual_intervention", output.totals.files_with_manual_intervention, true, 4);
    append_json_numeric_field(out, "total_diagnostics", output.totals.total_diagnostics, true, 4);
    out << "    \"entry_summary\": ";
    append_summary_object(out, output.totals.summary, 4);
    out << '\n';
    out << "  },\n";
    out << "  \"files\": [\n";
    for (size_t index = 0; index < output.files.size(); ++index) {
        const auto& file = output.files[index];
        out << "    {\n";
        append_json_string_field(out, "source_path", to_generic_string(file.source_relative_path), true, 6);
        append_json_string_field(out, "output_script_path", to_generic_string(file.output_relative_path), true, 6);
        append_json_string_field(out, "report_path", to_generic_string(file.report_relative_path), true, 6);
        append_json_bool_field(out, "converted", file.converted, true, 6);
        append_json_bool_field(out, "needs_manual_intervention", file.needs_manual_intervention, true, 6);
        append_json_numeric_field(out, "diagnostic_count", file.diagnostic_count, true, 6);
        out << "      \"summary\": ";
        append_summary_object(out, file.summary, 6);
        out << '\n';
        out << "    }";
        if (index + 1 < output.files.size()) {
            out << ',';
        }
        out << '\n';
    }
    out << "  ]\n";
    out << "}\n";
    return out.str();
}

void append_manifest_item_object(std::ostringstream& out,
                                 const ProjectManifestItem& item,
                                 bool trailing_comma) {
    out << "    {\n";
    append_json_string_field(out, "source_path", to_generic_string(item.source_relative_path), true, 6);
    append_json_string_field(out, "report_path", to_generic_string(item.report_relative_path), true, 6);
    append_json_string_field(out, "kind", conversion_entry_kind_to_string(item.entry.kind), true, 6);
    append_json_string_field(out, "severity", conversion_severity_to_string(item.entry.severity), true, 6);
    append_json_string_field(out, "feature_tag", item.entry.feature_tag, true, 6);
    append_json_string_field(out, "reason", item.entry.reason, true, 6);
    append_json_string_field(out, "action", item.entry.action, true, 6);
    append_json_string_field(out, "original_text", item.entry.original_text, true, 6);
    out << "      \"source_range\": {\n";
    append_json_numeric_field(out, "start_line", item.entry.source_range.start_line, true, 8);
    append_json_numeric_field(out, "end_line", item.entry.source_range.end_line, true, 8);
    append_json_numeric_field(out, "start_column", item.entry.source_range.start_column, true, 8);
    append_json_numeric_field(out, "end_column", item.entry.source_range.end_column, false, 8);
    out << "      }\n";
    out << "    }";
    if (trailing_comma) {
        out << ',';
    }
    out << '\n';
}

std::string serialize_manual_fix_manifest_json(const ProjectConversionOutput& output) {
    std::ostringstream out;
    out << "{\n";
    append_json_string_field(out, "project_root", to_generic_string(output.project_root), true, 2);
    append_json_string_field(out, "output_dir", to_generic_string(output.output_dir), true, 2);
    out << "  \"summary\": {\n";
    append_json_numeric_field(out, "manual_fix_item_count", output.manual_fix_items.size(), true, 4);
    append_json_numeric_field(out, "review_item_count", output.review_items.size(), true, 4);
    append_json_numeric_field(out, "files_with_manual_intervention", output.totals.files_with_manual_intervention, false, 4);
    out << "  },\n";
    out << "  \"manual_fix_items\": [\n";
    for (size_t index = 0; index < output.manual_fix_items.size(); ++index) {
        append_manifest_item_object(out, output.manual_fix_items[index], index + 1 < output.manual_fix_items.size());
    }
    out << "  ],\n";
    out << "  \"review_items\": [\n";
    for (size_t index = 0; index < output.review_items.size(); ++index) {
        append_manifest_item_object(out, output.review_items[index], index + 1 < output.review_items.size());
    }
    out << "  ]\n";
    out << "}\n";
    return out.str();
}

} // namespace

bool ProjectConversionOutput::has_failures() const {
    return totals.failed_files > 0;
}

ProjectConversionOutput ProjectConverter::convert_project(const ProjectConversionOptions& options) const {
    const fs::path project_root = options.project_root.lexically_normal();
    const fs::path output_dir = options.output_dir.lexically_normal();
    const std::vector<fs::path> source_files = collect_project_files(project_root);

    ProjectConversionOutput output;
    output.project_root = project_root;
    output.output_dir = output_dir;
    output.resource_manifest.project_root = project_root;
    output.resource_manifest.output_dir = output_dir;
    output.files.reserve(source_files.size());

    Converter converter;
    RenpyLexer lexer;
    RenpyAnalyzer analyzer;
    for (const fs::path& source_relative_path : source_files) {
        const fs::path input_path = project_root / source_relative_path;
        const fs::path output_relative_path = make_output_relative_path(source_relative_path);
        const fs::path report_relative_path = make_report_relative_path(source_relative_path);
        const fs::path output_path = output_dir / output_relative_path;
        const fs::path report_path = output_dir / report_relative_path;

        ConversionReport report;
        bool converted = false;
        std::string novamark_source;

        try {
            const std::string source = read_text_file(input_path);
            auto result = converter.convert(ConversionInput{source, source_relative_path.generic_string()});
            report = result.is_ok() ? result.unwrap().report : result.report();
            if (result.is_ok()) {
                novamark_source = result.unwrap().novamark_source;
                converted = true;
            }

            auto token_result = lexer.tokenize(source);
            if (token_result.is_ok()) {
                auto analysis_result = analyzer.analyze(std::move(token_result.unwrap()));
                if (analysis_result.is_ok()) {
                    auto resource_entries = extract_resource_manifest_entries(analysis_result.unwrap(), source_relative_path);
                    output.resource_manifest.entries.insert(
                        output.resource_manifest.entries.end(),
                        resource_entries.begin(),
                        resource_entries.end());
                }
            }
        } catch (const std::exception& e) {
            report.add(ConversionSeverity::Error, e.what());
        }

        if (converted) {
            try {
                write_text_file(output_path, novamark_source);
            } catch (const std::exception& e) {
                report.add(ConversionSeverity::Error, e.what());
                converted = false;
            }
        }

        write_text_file(report_path, serialize_report_json(report, source_relative_path.generic_string()));

        ProjectFileConversionResult file_result;
        file_result.source_relative_path = source_relative_path;
        file_result.output_relative_path = output_relative_path;
        file_result.report_relative_path = report_relative_path;
        file_result.converted = converted;
        file_result.needs_manual_intervention = report.needs_manual_intervention();
        file_result.diagnostic_count = report.diagnostics().size();
        file_result.summary = report.summary();

        output.totals.total_files += 1;
        output.totals.total_diagnostics += file_result.diagnostic_count;
        output.totals.summary = accumulate_summary(output.totals.summary, file_result.summary);
        if (file_result.converted) {
            output.totals.converted_files += 1;
        } else {
            output.totals.failed_files += 1;
        }
        if (file_result.needs_manual_intervention) {
            output.totals.files_with_manual_intervention += 1;
        }

        collect_manifest_items(file_result,
                               report,
                               output.manual_fix_items,
                               output.review_items,
                               options.include_review_items);
        output.files.push_back(std::move(file_result));
    }

    write_text_file(output_dir / AGGREGATE_REPORT_FILE, serialize_aggregate_report_json(output));
    write_text_file(output_dir / MANUAL_FIX_MANIFEST_FILE, serialize_manual_fix_manifest_json(output));
    write_text_file(output_dir / RESOURCE_MANIFEST_FILE, serialize_resource_manifest_json(output.resource_manifest));
    return output;
}

} // namespace nova::renpy2nova
