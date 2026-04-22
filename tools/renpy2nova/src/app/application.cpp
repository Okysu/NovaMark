#include "../../include/renpy2nova/app/application.h"

#include "../../include/renpy2nova/converter/converter.h"
#include "../../include/renpy2nova/converter/project_converter.h"
#include "../../include/renpy2nova/core/filesystem_compat.h"
#include "../../include/renpy2nova/report/conversion_report.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace nova::renpy2nova {
namespace {

namespace fs = filesystem_compat;

constexpr const char* PROJECT_AGGREGATE_REPORT = "reports/aggregate_report.json";
constexpr const char* PROJECT_MANUAL_FIX_MANIFEST = "manifests/manual_fix_manifest.json";

std::string default_output_path(const std::string& input_path) {
    fs::path p(input_path);
    if (p.extension() == ".rpy") {
        return p.replace_extension(".nvm").string();
    }
    return p.string() + ".nvm";
}

std::string read_file(const std::string& path) {
    std::ifstream file(path, std::ios::in);
    if (!file) {
        throw std::runtime_error("failed to open file: " + path);
    }
    return std::string{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

void ensure_parent_directory(const std::string& path) {
    const fs::path p(path);
    const fs::path parent = p.parent_path();
    if (!parent.empty()) {
        fs::create_directories(parent);
    }
}

void write_file(const std::string& path, const std::string& content) {
    ensure_parent_directory(path);

    std::ofstream file(path, std::ios::out | std::ios::binary);
    if (!file) {
        throw std::runtime_error("failed to write file: " + path);
    }
    file << content;
}

void write_output(const std::string& path, const std::string& content, std::ostream& out) {
    if (path == "-") {
        out << content;
        return;
    }

    write_file(path, content);
}

std::string format_source_range(const ConversionLineRange& range) {
    std::string label = "L" + std::to_string(range.start_line);
    if (range.start_column > 1 || range.end_column > 1) {
        label += ":" + std::to_string(range.start_column);
    }

    if (range.start_line != range.end_line || range.start_column != range.end_column) {
        label += "-L" + std::to_string(range.end_line);
        if (range.end_column > 1 || range.start_column > 1) {
            label += ":" + std::to_string(range.end_column);
        }
    }

    return label;
}

std::string make_inline_excerpt(const std::string& text) {
    std::string excerpt;
    excerpt.reserve(text.size());
    for (char ch : text) {
        switch (ch) {
            case '\r':
                break;
            case '\n':
                excerpt += "\\n";
                break;
            case '\t':
                excerpt += "\\t";
                break;
            default:
                excerpt.push_back(ch);
                break;
        }
    }

    constexpr size_t MAX_EXCERPT = 120;
    if (excerpt.size() > MAX_EXCERPT) {
        excerpt.resize(MAX_EXCERPT - 3);
        excerpt += "...";
    }
    return excerpt;
}

void print_report_summary(const ConversionReport& report,
                          const std::string& source_name,
                          std::ostream& err) {
    const ConversionSummary summary = report.summary();
    if (summary.total_entries == 0 && report.diagnostics().empty()) {
        return;
    }

    err << "\n--- Conversion report: " << source_name << " ---\n";

    for (const auto& diagnostic : report.diagnostics()) {
        const char* level = conversion_severity_to_string(diagnostic.severity);
        err << source_name << ':' << diagnostic.line << ':' << diagnostic.column
            << ": " << level << ": " << diagnostic.message << "\n";
    }

    for (const auto& entry : report.entries()) {
        if (entry.kind == ConversionEntryKind::FullySupported) {
            continue;
        }

        const char* kind_label = "";
        switch (entry.kind) {
            case ConversionEntryKind::PartiallySupported: kind_label = "partially supported"; break;
            case ConversionEntryKind::Unsupported: kind_label = "unsupported"; break;
            case ConversionEntryKind::ManualFixRequired: kind_label = "manual fix required"; break;
            case ConversionEntryKind::FullySupported: break;
        }

        err << "  [" << kind_label << "] " << format_source_range(entry.source_range)
            << ": " << entry.reason << "\n";
        if (!entry.action.empty()) {
            err << "    Action: " << entry.action << "\n";
        }
        if (!entry.original_text.empty()) {
            err << "    Original: " << make_inline_excerpt(entry.original_text) << "\n";
        }
    }

    err << "\nSummary: fully supported " << summary.fully_supported
        << " | partially supported " << summary.partially_supported
        << " | unsupported " << summary.unsupported
        << " | manual fix required " << summary.manual_fix_required
        << " | diagnostics " << report.diagnostics().size() << "\n";

    if (report.needs_manual_intervention()) {
        err << "Hint: manual intervention required; review the structured report for batch follow-up.\n";
    }
}

bool is_option(const std::string& arg) {
    return !arg.empty() && arg.front() == '-';
}

std::string validate_options(const CliOptions& options) {
    if (options.mode == CliMode::Project) {
        if (!options.input_path.empty()) {
            return "--project mode does not accept positional input files";
        }
        if (!options.output_path.empty()) {
            return "--project mode cannot be used with -o/--output; use --output-dir instead";
        }
        if (!options.report_path.empty()) {
            return "--project mode does not support --report; report directories are written automatically";
        }
        if (options.project_path.empty()) {
            return "missing --project <dir>";
        }
        if (options.output_dir.empty()) {
            return "--project mode requires --output-dir <dir>";
        }
        return {};
    }

    if (!options.output_dir.empty()) {
        return "--output-dir is only valid in --project mode";
    }
    return {};
}

int run_single_file_mode(CliOptions options, std::ostream& out, std::ostream& err) {
    if (options.input_path.empty()) {
        out << build_usage_text("renpy2nova");
        return 1;
    }

    if (options.output_path.empty()) {
        options.output_path = default_output_path(options.input_path);
    }

    if (options.report_path == "-") {
        err << "Error: --report requires a file path; writing to stdout is not supported\n";
        return 1;
    }

    if (!options.report_path.empty() && options.output_path != "-" && options.report_path == options.output_path) {
        err << "Error: output file and report file must be different\n";
        return 1;
    }

    if (!fs::exists(options.input_path)) {
        err << "Error: input file does not exist: " << options.input_path << "\n";
        return 1;
    }

    std::string source;
    try {
        source = read_file(options.input_path);
    } catch (const std::exception& e) {
        err << "Error: " << e.what() << "\n";
        return 1;
    }

    ConversionInput input{std::move(source), options.input_path};
    Converter converter;
    auto result = converter.convert(input);

    const ConversionReport& effective_report = result.is_ok()
        ? result.unwrap().report
        : result.report();

    if (!options.report_path.empty()) {
        try {
            write_file(options.report_path, serialize_report_json(effective_report, options.input_path));
        } catch (const std::exception& e) {
            err << "Error: " << e.what() << "\n";
            return 1;
        }
    }

    if (result.is_err()) {
        print_report_summary(effective_report, options.input_path, err);
        if (!options.report_path.empty()) {
            err << "Structured report: " << options.report_path << "\n";
        }
        err << "\nConversion failed. Fix the issues above and try again.\n";
        return 1;
    }

    const auto& output_result = result.unwrap();
    try {
        write_output(options.output_path, output_result.novamark_source, out);
    } catch (const std::exception& e) {
        err << "Error: " << e.what() << "\n";
        return 1;
    }

    print_report_summary(effective_report, options.input_path, err);

    if (options.output_path == "-") {
        err << "\nConversion complete: " << options.input_path << " -> stdout\n";
    } else {
        err << "\nConversion complete: " << options.input_path << " -> " << options.output_path << "\n";
    }

    if (!options.report_path.empty()) {
        err << "Structured report: " << options.report_path << "\n";
    }

    return 0;
}

int run_project_mode(const CliOptions& options, std::ostream& err) {
    if (!fs::exists(options.project_path)) {
        err << "Error: project directory does not exist: " << options.project_path << "\n";
        return 1;
    }
    if (!fs::is_directory(options.project_path)) {
        err << "Error: project path is not a directory: " << options.project_path << "\n";
        return 1;
    }

    ProjectConverter converter;
    ProjectConversionOutput output;
    try {
        output = converter.convert_project(ProjectConversionOptions{
            fs::path(options.project_path),
            fs::path(options.output_dir),
            true,
        });
    } catch (const std::exception& e) {
        err << "Error: " << e.what() << "\n";
        return 1;
    }

    err << "\nProject conversion complete: " << options.project_path << " -> " << options.output_dir << "\n";
    err << "Script files: " << output.totals.total_files
        << " | converted " << output.totals.converted_files
        << " | failed " << output.totals.failed_files
        << " | manual intervention " << output.totals.files_with_manual_intervention
        << " | diagnostics " << output.totals.total_diagnostics << "\n";
    err << "Aggregate report: " << (fs::path(options.output_dir) / PROJECT_AGGREGATE_REPORT).string() << "\n";
    err << "Manual fix manifest: " << (fs::path(options.output_dir) / PROJECT_MANUAL_FIX_MANIFEST).string() << "\n";

    if (output.has_failures()) {
        err << "\nConversion failures detected. Review the per-file reports and aggregate report first.\n";
        return 1;
    }

    return 0;
}

} // namespace

std::string build_usage_text(const char* program_name) {
    std::ostringstream out;
    out << "renpy2nova - Ren'Py to NovaMark converter\n"
        << "\n"
        << "Usage:\n"
        << "  " << program_name << " <input.rpy> [options]\n"
        << "  " << program_name << " --project <dir> --output-dir <dir>\n"
        << "\n"
        << "Single-file options:\n"
        << "  -o, --output <path>   Output file path (default: input.nvm, use - for stdout)\n"
        << "      --report <path>   Write structured conversion report as JSON\n"
        << "\n"
        << "Project-mode options:\n"
        << "      --project <dir>   Recursively scan .rpy files under project root\n"
        << "      --output-dir <dir>\n"
        << "                        Write mirrored scripts/, reports/, manifests/ outputs\n"
        << "\n"
        << "General options:\n"
        << "  -h, --help            Show this help message\n"
        << "\n"
        << "If output path is omitted in single-file mode, the result is written to a .nvm\n"
        << "file with the same base name as the input.\n";
    return out.str();
}

CliParseResult parse_command_line(int argc, char** argv) {
    CliParseResult result;
    result.ok = true;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            result.options.show_help = true;
            break;
        }

        auto require_value = [&](const std::string& flag, std::string& target) -> bool {
            if (i + 1 >= argc) {
                result.ok = false;
                result.error_message = std::string(flag) + " requires a value";
                return false;
            }

            target = argv[++i];
            return true;
        };

        if (arg == "-o" || arg == "--output") {
            if (!require_value(arg, result.options.output_path)) {
                return result;
            }
            continue;
        }
        if (arg == "--report") {
            if (!require_value(arg, result.options.report_path)) {
                return result;
            }
            continue;
        }
        if (arg == "--project") {
            if (!require_value(arg, result.options.project_path)) {
                return result;
            }
            result.options.mode = CliMode::Project;
            continue;
        }
        if (arg == "--output-dir") {
            if (!require_value(arg, result.options.output_dir)) {
                return result;
            }
            continue;
        }
        if (is_option(arg)) {
            result.ok = false;
            result.error_message = "unknown option " + std::string(arg);
            return result;
        }

        if (result.options.input_path.empty()) {
            result.options.input_path = argv[i];
        } else {
            result.ok = false;
            result.error_message = "multiple input files are not supported; only a single input file is accepted";
            return result;
        }
    }

    return result;
}

int run_application(int argc, char** argv, std::ostream& out, std::ostream& err) {
    const CliParseResult parse_result = parse_command_line(argc, argv);
    if (!parse_result.ok) {
        err << "Error: " << parse_result.error_message << "\n";
        err << "Use --help for usage\n";
        return 1;
    }

    const CliOptions& options = parse_result.options;
    if (options.show_help || (options.mode == CliMode::SingleFile && options.input_path.empty())) {
        out << build_usage_text(argc > 0 ? argv[0] : "renpy2nova");
        return options.show_help ? 0 : 1;
    }

    if (const std::string validation_error = validate_options(options); !validation_error.empty()) {
        err << "Error: " << validation_error << "\n";
        err << "Use --help for usage\n";
        return 1;
    }

    if (options.mode == CliMode::Project) {
        return run_project_mode(options, err);
    }

    return run_single_file_mode(options, out, err);
}

} // namespace nova::renpy2nova
