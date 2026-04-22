#include "../include/renpy2nova/app/application.h"
#include "../include/renpy2nova/analyzer/renpy_analyzer.h"
#include "../include/renpy2nova/converter/converter.h"
#include "../include/renpy2nova/converter/project_converter.h"
#include "../include/renpy2nova/core/filesystem_compat.h"
#include "../include/renpy2nova/emitter/novamark_emitter.h"
#include "../include/renpy2nova/lexer/renpy_lexer.h"
#include "../include/renpy2nova/report/conversion_report.h"
#include "../include/renpy2nova/resource/resource_manifest.h"

#include "../../../vcpkg_installed/x64-windows/include/gtest/gtest.h"

#include <ctime>
#include <fstream>
#include <sstream>
#include <vector>

using namespace nova::renpy2nova;

namespace {

namespace fs = filesystem_compat;

std::string normalize_newlines(std::string text);

size_t next_temp_dir_id() {
    static size_t counter = 0;
    return ++counter;
}

fs::path test_data_root() {
    return fs::path(RENRY2NOVA_TEST_DATA_DIR);
}

std::string read_text_file(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    EXPECT_TRUE(input.is_open()) << "无法打开测试文件: " << path.string();

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return normalize_newlines(buffer.str());
}

struct FixtureExpectation {
    std::string name;
    size_t entry_count;
    size_t manual_fix_count;
    size_t partially_supported_count;
    size_t unsupported_count;
    bool needs_manual_intervention;
};

void expect_fixture_conversion(const FixtureExpectation& fixture) {
    const auto fixture_path = test_data_root() / "fixtures" / (fixture.name + ".rpy");
    const auto golden_path = test_data_root() / "golden" / (fixture.name + ".nvm");

    const std::string source = read_text_file(fixture_path);
    const std::string expected_output = read_text_file(golden_path);

    Converter converter;
    auto convert_result = converter.convert(ConversionInput{source, fixture_path.filename().string()});

    ASSERT_TRUE(convert_result.is_ok()) << fixture.name;
    const ConversionOutput& output = convert_result.unwrap();
    EXPECT_EQ(output.novamark_source, expected_output) << fixture.name;
    EXPECT_FALSE(output.report.has_errors()) << fixture.name;
    EXPECT_EQ(output.report.entries().size(), fixture.entry_count) << fixture.name;
    EXPECT_EQ(output.report.entries_by_kind(ConversionEntryKind::ManualFixRequired).size(), fixture.manual_fix_count) << fixture.name;
    EXPECT_EQ(output.report.entries_by_kind(ConversionEntryKind::PartiallySupported).size(), fixture.partially_supported_count) << fixture.name;
    EXPECT_EQ(output.report.entries_by_kind(ConversionEntryKind::Unsupported).size(), fixture.unsupported_count) << fixture.name;
    EXPECT_EQ(output.report.needs_manual_intervention(), fixture.needs_manual_intervention) << fixture.name;
}

class ScopedTempDir {
public:
    ScopedTempDir() {
        const auto base = fs::path(std::filesystem::temp_directory_path().string());
        const auto now = static_cast<long long>(std::time(nullptr));
        m_path = base / ("renpy2nova-" + std::to_string(now) + "-" + std::to_string(next_temp_dir_id()));
        fs::create_directories(m_path);
    }

    ~ScopedTempDir() {
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(m_path.string()), ec);
    }

    const fs::path& path() const {
        return m_path;
    }

private:
    fs::path m_path;
};

void write_text_file(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    ASSERT_TRUE(output.is_open()) << "无法写入测试文件: " << path.string();
    output << content;
}

std::string normalize_newlines(std::string text) {
    std::string result;
    result.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r') {
            if (i + 1 < text.size() && text[i + 1] == '\n') {
                continue;
            }
            result.push_back('\n');
            continue;
        }
        result.push_back(text[i]);
    }
    return result;
}

std::string reserved_none_literal() {
    return std::string("\"") + RESERVED_NONE_SENTINEL + "\"";
}

std::vector<char*> make_argv(std::vector<std::string>& args) {
    std::vector<char*> argv;
    argv.reserve(args.size());
    for (auto& arg : args) {
        argv.push_back(arg.empty() ? nullptr : &arg[0]);
    }
    return argv;
}

} // namespace

TEST(Renpy2NovaReportTest, TracksDiagnosticsAndErrors) {
    ConversionReport report;

    EXPECT_FALSE(report.has_errors());

    report.add(ConversionSeverity::Warning, "占位警告", 2, 3);
    EXPECT_FALSE(report.has_errors());
    ASSERT_EQ(report.diagnostics().size(), 1);
    EXPECT_EQ(report.diagnostics()[0].line, 2);
    EXPECT_EQ(report.diagnostics()[0].column, 3);

    report.add(ConversionSeverity::Error, "占位错误");
    EXPECT_TRUE(report.has_errors());
}

TEST(Renpy2NovaReportTest, LineRangeHelpers) {
    auto single = ConversionLineRange::single(5, 10);
    EXPECT_EQ(single.start_line, 5);
    EXPECT_EQ(single.end_line, 5);
    EXPECT_EQ(single.start_column, 10);
    EXPECT_EQ(single.end_column, 10);

    auto multi = ConversionLineRange::range(3, 7);
    EXPECT_EQ(multi.start_line, 3);
    EXPECT_EQ(multi.end_line, 7);
}

TEST(Renpy2NovaReportTest, EntrySingleLineConstructor) {
    auto entry = ConversionEntry::single_line(
        ConversionEntryKind::Unsupported,
        ConversionSeverity::Warning,
        "screen",
        "Ren'Py screen language not supported",
        "Manually implement as @scene directive",
        "screen my_screen():",
        42
    );

    EXPECT_EQ(entry.kind, ConversionEntryKind::Unsupported);
    EXPECT_EQ(entry.severity, ConversionSeverity::Warning);
    EXPECT_EQ(entry.feature_tag, "screen");
    EXPECT_EQ(entry.source_range.start_line, 42);
    EXPECT_EQ(entry.source_range.end_line, 42);
}

TEST(Renpy2NovaReportTest, TracksEntries) {
    ConversionReport report;

    EXPECT_TRUE(report.entries().empty());
    EXPECT_FALSE(report.needs_manual_intervention());

    report.add_entry(ConversionEntry::single_line(
        ConversionEntryKind::PartiallySupported,
        ConversionSeverity::Info,
        "transition",
        "ATL transitions partially supported",
        "Review generated @transition directive",
        "with easeinright",
        10
    ));

    ASSERT_EQ(report.entries().size(), 1);
    EXPECT_FALSE(report.needs_manual_intervention());

    report.add_entry(ConversionEntry::single_line(
        ConversionEntryKind::Unsupported,
        ConversionSeverity::Warning,
        "screen",
        "screen language not supported",
        "Manual implementation required",
        "screen inventory():",
        20
    ));

    EXPECT_TRUE(report.needs_manual_intervention());
}

TEST(Renpy2NovaReportTest, EntriesByKindFiltering) {
    ConversionReport report;

    report.add_entry(ConversionEntry::single_line(
        ConversionEntryKind::Unsupported, ConversionSeverity::Warning,
        "screen", "reason", "action", "text1", 1));
    report.add_entry(ConversionEntry::single_line(
        ConversionEntryKind::PartiallySupported, ConversionSeverity::Info,
        "transition", "reason", "action", "text2", 2));
    report.add_entry(ConversionEntry::single_line(
        ConversionEntryKind::Unsupported, ConversionSeverity::Warning,
        "transform", "reason", "action", "text3", 3));

    auto unsupported = report.entries_by_kind(ConversionEntryKind::Unsupported);
    ASSERT_EQ(unsupported.size(), 2);

    auto partial = report.entries_by_kind(ConversionEntryKind::PartiallySupported);
    ASSERT_EQ(partial.size(), 1);

    auto fully = report.entries_by_kind(ConversionEntryKind::FullySupported);
    EXPECT_TRUE(fully.empty());
}

TEST(Renpy2NovaReportTest, SummaryAggregation) {
    ConversionReport report;

    report.add_entry(ConversionEntry::single_line(
        ConversionEntryKind::FullySupported, ConversionSeverity::Info,
        "label", "", "", "label start:", 1));
    report.add_entry(ConversionEntry::single_line(
        ConversionEntryKind::PartiallySupported, ConversionSeverity::Warning,
        "transition", "", "", "with dissolve", 5));
    report.add_entry(ConversionEntry::single_line(
        ConversionEntryKind::Unsupported, ConversionSeverity::Warning,
        "screen", "", "", "screen menu():", 10));
    report.add_entry(ConversionEntry::single_line(
        ConversionEntryKind::ManualFixRequired, ConversionSeverity::Error,
        "python", "", "", "python:\n  x = 1", 15));

    report.add(ConversionSeverity::Warning, "extra warning");
    report.add(ConversionSeverity::Error, "extra error");

    auto s = report.summary();

    EXPECT_EQ(s.total_entries, 4);
    EXPECT_EQ(s.fully_supported, 1);
    EXPECT_EQ(s.partially_supported, 1);
    EXPECT_EQ(s.unsupported, 1);
    EXPECT_EQ(s.manual_fix_required, 1);
    EXPECT_EQ(s.warning_count, 3);
    EXPECT_EQ(s.error_count, 2);
    EXPECT_EQ(s.info_count, 1);
}

TEST(Renpy2NovaReportTest, EnumNamesAreStableForStructuredExport) {
    EXPECT_STREQ(conversion_severity_to_string(ConversionSeverity::Info), "info");
    EXPECT_STREQ(conversion_severity_to_string(ConversionSeverity::Warning), "warning");
    EXPECT_STREQ(conversion_severity_to_string(ConversionSeverity::Error), "error");

    EXPECT_STREQ(conversion_entry_kind_to_string(ConversionEntryKind::FullySupported), "fully_supported");
    EXPECT_STREQ(conversion_entry_kind_to_string(ConversionEntryKind::PartiallySupported), "partially_supported");
    EXPECT_STREQ(conversion_entry_kind_to_string(ConversionEntryKind::Unsupported), "unsupported");
    EXPECT_STREQ(conversion_entry_kind_to_string(ConversionEntryKind::ManualFixRequired), "manual_fix_required");
}

TEST(Renpy2NovaReportTest, SerializesStructuredJsonReport) {
    ConversionReport report;
    report.add(ConversionSeverity::Warning, "含有\"引号\"与换行\n", 3, 9);
    report.add_entry(ConversionEntry{
        ConversionEntryKind::ManualFixRequired,
        ConversionSeverity::Error,
        "原因包含换行\n第二行",
        "请手动修复",
        "python:\n    $ score = 1",
        ConversionLineRange{7, 8, 2, 12},
        "python"
    });

    const std::string json = serialize_report_json(report, "fixtures/demo.rpy");

    EXPECT_NE(json.find("\"source_name\": \"fixtures/demo.rpy\""), std::string::npos);
    EXPECT_NE(json.find("\"total_entries\": 1"), std::string::npos);
    EXPECT_NE(json.find("\"manual_fix_required\": 1"), std::string::npos);
    EXPECT_NE(json.find("\"error_count\": 1"), std::string::npos);
    EXPECT_NE(json.find("\"warning_count\": 1"), std::string::npos);
    EXPECT_NE(json.find("\"diagnostics\": ["), std::string::npos);
    EXPECT_NE(json.find("\"entries\": ["), std::string::npos);
    EXPECT_NE(json.find("\"kind\": \"manual_fix_required\""), std::string::npos);
    EXPECT_NE(json.find("\"severity\": \"error\""), std::string::npos);
    EXPECT_NE(json.find("\"feature_tag\": \"python\""), std::string::npos);
    EXPECT_NE(json.find("\"line\": 3"), std::string::npos);
    EXPECT_NE(json.find("\"column\": 9"), std::string::npos);
    EXPECT_NE(json.find("\"start_line\": 7"), std::string::npos);
    EXPECT_NE(json.find("\"end_line\": 8"), std::string::npos);
    EXPECT_NE(json.find("\"start_column\": 2"), std::string::npos);
    EXPECT_NE(json.find("\"end_column\": 12"), std::string::npos);
    EXPECT_NE(json.find("含有\\\"引号\\\"与换行\\n"), std::string::npos);
    EXPECT_NE(json.find("原因包含换行\\n第二行"), std::string::npos);
    EXPECT_NE(json.find("python:\\n    $ score = 1"), std::string::npos);
}

TEST(Renpy2NovaCliTest, ParseCommandLineSupportsExplicitProjectModeFlags) {
    std::vector<std::string> args = {
        "renpy2nova",
        "--project",
        "demo/game",
        "--output-dir",
        "out/migration",
    };
    auto argv = make_argv(args);

    const auto result = parse_command_line(static_cast<int>(argv.size()), argv.data());

    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.options.mode, CliMode::Project);
    EXPECT_EQ(result.options.project_path, "demo/game");
    EXPECT_EQ(result.options.output_dir, "out/migration");
    EXPECT_TRUE(result.options.input_path.empty());
}

TEST(Renpy2NovaResourceManifestTest, ExtractsConservativeResourceHintsFromAnalyzedProject) {
    const std::string source =
        "define e = Character(\"Eileen\", color=\"#C0FFEE\", image=\"eileen_default\")\n"
        "label start:\n"
        "    scene bg room with fade\n"
        "    show eileen happy at left\n"
        "    play music \"theme.ogg\" loop volume 0.75\n"
        "    play sound \"door.wav\" volume 0.5\n"
        "    image eileen happy = \"eileen_happy.png\"\n"
        "    play voice \"line.ogg\"\n";

    RenpyLexer lexer;
    auto token_result = lexer.tokenize(source);
    ASSERT_TRUE(token_result.is_ok());

    RenpyAnalyzer analyzer;
    auto analyze_result = analyzer.analyze(std::move(token_result.unwrap()));
    ASSERT_TRUE(analyze_result.is_ok());

    const auto entries = extract_resource_manifest_entries(analyze_result.unwrap(), fs::path("scripts/demo.rpy"));

    ASSERT_EQ(entries.size(), 6U);

    EXPECT_EQ(entries[0].kind, ResourceKind::CharacterDefinition);
    EXPECT_EQ(entries[0].name, "e");
    EXPECT_EQ(entries[0].path_hint, "eileen_default.png");
    EXPECT_EQ(entries[0].display_name, "Eileen");
    EXPECT_EQ(entries[0].color, "#C0FFEE");

    EXPECT_EQ(entries[1].kind, ResourceKind::SceneReference);
    EXPECT_EQ(entries[1].path_hint, "room");

    EXPECT_EQ(entries[2].kind, ResourceKind::ShowReference);
    EXPECT_EQ(entries[2].name, "eileen");
    EXPECT_EQ(entries[2].expression, "happy");

    EXPECT_EQ(entries[3].kind, ResourceKind::PlayMusic);
    EXPECT_EQ(entries[3].path_hint, "theme.ogg");

    EXPECT_EQ(entries[4].kind, ResourceKind::PlaySound);
    EXPECT_EQ(entries[4].path_hint, "door.wav");

    EXPECT_EQ(entries[5].kind, ResourceKind::ImageDefinition);
    EXPECT_EQ(entries[5].name, "eileen");
    EXPECT_EQ(entries[5].expression, "happy");
    EXPECT_EQ(entries[5].path_hint, "eileen_happy.png");
}

TEST(Renpy2NovaProjectModeTest, ConvertsProjectDirectoryAndWritesMirroredOutputsReportsAndManifest) {
    ScopedTempDir temp_dir;
    const auto project_root = temp_dir.path() / "project";
    const auto output_root = temp_dir.path() / "output";

    write_text_file(project_root / "z_last.rpy", read_text_file(test_data_root() / "fixtures" / "unsupported_degradation.rpy"));
    write_text_file(project_root / "a_first.rpy", read_text_file(test_data_root() / "fixtures" / "simple_dialogue_scene.rpy"));
    write_text_file(project_root / "nested" / "mid.rpy", read_text_file(test_data_root() / "fixtures" / "advanced_practical_coverage.rpy"));

    ProjectConverter converter;
    const auto result = converter.convert_project(ProjectConversionOptions{project_root, output_root, true});

    ASSERT_EQ(result.files.size(), 3);
    EXPECT_EQ(result.files[0].source_relative_path.generic_string(), "a_first.rpy");
    EXPECT_EQ(result.files[1].source_relative_path.generic_string(), "nested/mid.rpy");
    EXPECT_EQ(result.files[2].source_relative_path.generic_string(), "z_last.rpy");

    EXPECT_EQ(result.totals.total_files, 3U);
    EXPECT_EQ(result.totals.converted_files, 3U);
    EXPECT_EQ(result.totals.failed_files, 0U);
    EXPECT_EQ(result.totals.files_with_manual_intervention, 1U);
    EXPECT_EQ(result.totals.summary.total_entries, 7U);
    EXPECT_EQ(result.totals.summary.partially_supported, 4U);
    EXPECT_EQ(result.totals.summary.unsupported, 1U);
    EXPECT_EQ(result.totals.summary.manual_fix_required, 2U);
    EXPECT_FALSE(result.has_failures());

    EXPECT_EQ(result.files[0].output_relative_path.generic_string(), "scripts/a_first.nvm");
    EXPECT_EQ(result.files[1].output_relative_path.generic_string(), "scripts/nested/mid.nvm");
    EXPECT_EQ(result.files[2].report_relative_path.generic_string(), "reports/file_reports/z_last.json");

    EXPECT_EQ(
        read_text_file(output_root / "scripts" / "a_first.nvm"),
        read_text_file(test_data_root() / "golden" / "simple_dialogue_scene.nvm"));
    EXPECT_EQ(
        read_text_file(output_root / "scripts" / "nested" / "mid.nvm"),
        read_text_file(test_data_root() / "golden" / "advanced_practical_coverage.nvm"));

    const std::string per_file_report = read_text_file(output_root / "reports" / "file_reports" / "z_last.json");
    EXPECT_NE(per_file_report.find("\"source_name\": \"z_last.rpy\""), std::string::npos);
    EXPECT_NE(per_file_report.find("\"manual_fix_required\": 2"), std::string::npos);

    const std::string aggregate_report = read_text_file(output_root / "reports" / "aggregate_report.json");
    EXPECT_NE(aggregate_report.find("\"total_files\": 3"), std::string::npos);
    EXPECT_NE(aggregate_report.find("\"converted_files\": 3"), std::string::npos);
    EXPECT_NE(aggregate_report.find("\"failed_files\": 0"), std::string::npos);
    EXPECT_NE(aggregate_report.find("\"files_with_manual_intervention\": 1"), std::string::npos);
    EXPECT_NE(aggregate_report.find("\"source_path\": \"a_first.rpy\""), std::string::npos);
    EXPECT_NE(aggregate_report.find("\"source_path\": \"nested/mid.rpy\""), std::string::npos);
    EXPECT_NE(aggregate_report.find("\"source_path\": \"z_last.rpy\""), std::string::npos);

    const std::string manual_manifest = read_text_file(output_root / "manifests" / "manual_fix_manifest.json");
    EXPECT_NE(manual_manifest.find("\"manual_fix_item_count\": 3"), std::string::npos);
    EXPECT_NE(manual_manifest.find("\"review_item_count\": 4"), std::string::npos);
    EXPECT_NE(manual_manifest.find("\"source_path\": \"z_last.rpy\""), std::string::npos);
    EXPECT_NE(manual_manifest.find("\"kind\": \"manual_fix_required\""), std::string::npos);
    EXPECT_NE(manual_manifest.find("\"kind\": \"unsupported\""), std::string::npos);
    EXPECT_NE(manual_manifest.find("\"kind\": \"partially_supported\""), std::string::npos);

    const std::string resource_manifest = read_text_file(output_root / "manifests" / "resource_manifest.json");
    EXPECT_NE(resource_manifest.find("\"version\": 1"), std::string::npos);
    EXPECT_NE(resource_manifest.find("\"resource_count\": 9"), std::string::npos);
    EXPECT_NE(resource_manifest.find("\"kind\": \"character_definition\""), std::string::npos);
    EXPECT_NE(resource_manifest.find("\"kind\": \"scene_reference\""), std::string::npos);
    EXPECT_NE(resource_manifest.find("\"kind\": \"show_reference\""), std::string::npos);
    EXPECT_NE(resource_manifest.find("\"kind\": \"play_music\""), std::string::npos);
    EXPECT_NE(resource_manifest.find("\"kind\": \"play_sound\""), std::string::npos);
    EXPECT_NE(resource_manifest.find("\"kind\": \"image_definition\""), std::string::npos);
    EXPECT_NE(resource_manifest.find("\"source_path\": \"nested/mid.rpy\""), std::string::npos);
    EXPECT_NE(resource_manifest.find("\"path_hint\": \"eileen_default.png\""), std::string::npos);
    EXPECT_NE(resource_manifest.find("\"path_hint\": \"theme.ogg\""), std::string::npos);
    EXPECT_NE(resource_manifest.find("\"path_hint\": \"door.wav\""), std::string::npos);
    EXPECT_NE(resource_manifest.find("\"path_hint\": \"eileen_happy.png\""), std::string::npos);
}

TEST(Renpy2NovaCliTest, ProjectModeCliEndToEndKeepsSingleFileFlowUntouched) {
    ScopedTempDir temp_dir;
    const auto single_input = test_data_root() / "fixtures" / "simple_dialogue_scene.rpy";
    const auto single_output = temp_dir.path() / "single.nvm";
    const auto single_report = temp_dir.path() / "single_report.json";

    std::vector<std::string> single_args = {
        "renpy2nova",
        single_input.string(),
        "--output",
        single_output.string(),
        "--report",
        single_report.string(),
    };
    auto single_argv = make_argv(single_args);
    std::ostringstream single_stdout;
    std::ostringstream single_stderr;

    const int single_exit_code = run_application(
        static_cast<int>(single_argv.size()),
        single_argv.data(),
        single_stdout,
        single_stderr);

    EXPECT_EQ(single_exit_code, 0);
    EXPECT_TRUE(single_stdout.str().empty());
    EXPECT_EQ(
        read_text_file(single_output),
        read_text_file(test_data_root() / "golden" / "simple_dialogue_scene.nvm"));
    EXPECT_NE(read_text_file(single_report).find("\"source_name\":"), std::string::npos);

    const auto project_root = temp_dir.path() / "project_cli";
    const auto project_output_root = temp_dir.path() / "project_out";
    write_text_file(project_root / "nested" / "scene.rpy", read_text_file(test_data_root() / "fixtures" / "advanced_practical_coverage.rpy"));
    write_text_file(project_root / "unsupported.rpy", read_text_file(test_data_root() / "fixtures" / "unsupported_degradation.rpy"));

    std::vector<std::string> project_args = {
        "renpy2nova",
        "--project",
        project_root.string(),
        "--output-dir",
        project_output_root.string(),
    };
    auto project_argv = make_argv(project_args);
    std::ostringstream project_stdout;
    std::ostringstream project_stderr;

    const int project_exit_code = run_application(
        static_cast<int>(project_argv.size()),
        project_argv.data(),
        project_stdout,
        project_stderr);

    EXPECT_EQ(project_exit_code, 0);
    EXPECT_TRUE(project_stdout.str().empty());
    const std::string project_log = normalize_newlines(project_stderr.str());
    EXPECT_NE(project_log.find("Project conversion complete:"), std::string::npos);
    EXPECT_NE(project_log.find("Script files: 2 | converted 2 | failed 0 | manual intervention 1"), std::string::npos);

    EXPECT_TRUE(std::filesystem::exists(project_output_root / "scripts" / "nested" / "scene.nvm"));
    EXPECT_TRUE(std::filesystem::exists(project_output_root / "scripts" / "unsupported.nvm"));
    EXPECT_TRUE(std::filesystem::exists(project_output_root / "reports" / "aggregate_report.json"));
    EXPECT_TRUE(std::filesystem::exists(project_output_root / "manifests" / "manual_fix_manifest.json"));
    EXPECT_TRUE(std::filesystem::exists(project_output_root / "manifests" / "resource_manifest.json"));
}

TEST(Renpy2NovaLexerTest, EmptySourceProducesOnlyEofToken) {
    RenpyLexer lexer;

    auto result = lexer.tokenize("");

    ASSERT_TRUE(result.is_ok());
    ASSERT_EQ(result.unwrap().size(), 1);
    EXPECT_EQ(result.unwrap()[0].type, RenpyTokenType::Eof);
    EXPECT_EQ(result.unwrap()[0].line, 1);
}

TEST(Renpy2NovaLexerTest, TokenizesSupportedV0Statements) {
    const std::string source =
        "define e = Character(\"Eileen\", color=\"#c0ffee\", image=\"eileen\")\n"
        "default affection = True\n"
        "label start:\n"
        "    scene bg room with fade\n"
        "    show eileen happy at left\n"
        "    with dissolve\n"
        "    hide eileen\n"
        "    e \"你好\"\n"
        "    \"旁白\"\n"
        "    $ affinity = None\n"
        "    if affection == True:\n"
        "        play music \"theme.ogg\" loop volume 0.8\n"
        "    elif affection == False:\n"
        "        play sound \"click.wav\" volume 0.5\n"
        "    else:\n"
        "        play voice \"line.ogg\"\n"
        "    menu:\n"
        "        \"继续\" if affection == True:\n"
        "            jump next_scene\n"
        "    call side_route\n"
        "    return done\n";

    RenpyLexer lexer;
    auto result = lexer.tokenize(source);

    ASSERT_TRUE(result.is_ok());
    const auto& tokens = result.unwrap();
    ASSERT_EQ(tokens.size(), 22);

    EXPECT_EQ(tokens[0].type, RenpyTokenType::DefineCharacter);
    EXPECT_EQ(tokens[0].primary, "e");
    EXPECT_EQ(tokens[0].secondary, "Character(\"Eileen\", color=\"#c0ffee\", image=\"eileen\")");

    EXPECT_EQ(tokens[1].type, RenpyTokenType::DefaultStatement);
    EXPECT_EQ(tokens[1].primary, "affection");
    EXPECT_EQ(tokens[1].secondary, "True");

    EXPECT_EQ(tokens[2].type, RenpyTokenType::Label);
    EXPECT_EQ(tokens[2].primary, "start");

    EXPECT_EQ(tokens[3].type, RenpyTokenType::Scene);
    EXPECT_EQ(tokens[3].primary, "bg room with fade");
    EXPECT_EQ(tokens[3].indent, 4U);

    EXPECT_EQ(tokens[4].type, RenpyTokenType::Show);
    EXPECT_EQ(tokens[4].primary, "eileen happy at left");
    EXPECT_EQ(tokens[5].type, RenpyTokenType::WithStatement);
    EXPECT_EQ(tokens[5].primary, "dissolve");
    EXPECT_EQ(tokens[6].type, RenpyTokenType::Hide);

    EXPECT_EQ(tokens[7].type, RenpyTokenType::Say);
    EXPECT_EQ(tokens[7].primary, "e");
    EXPECT_EQ(tokens[7].secondary, "你好");

    EXPECT_EQ(tokens[8].type, RenpyTokenType::Narrator);
    EXPECT_EQ(tokens[8].primary, "旁白");

    EXPECT_EQ(tokens[9].type, RenpyTokenType::DollarStatement);
    EXPECT_EQ(tokens[9].primary, "affinity = None");

    EXPECT_EQ(tokens[10].type, RenpyTokenType::IfStatement);
    EXPECT_EQ(tokens[10].primary, "affection == True");
    EXPECT_EQ(tokens[11].type, RenpyTokenType::PlayMusic);
    EXPECT_EQ(tokens[11].primary, "\"theme.ogg\" loop volume 0.8");
    EXPECT_EQ(tokens[12].type, RenpyTokenType::ElifStatement);
    EXPECT_EQ(tokens[12].primary, "affection == False");
    EXPECT_EQ(tokens[13].type, RenpyTokenType::PlaySound);
    EXPECT_EQ(tokens[13].primary, "\"click.wav\" volume 0.5");
    EXPECT_EQ(tokens[14].type, RenpyTokenType::ElseStatement);
    EXPECT_EQ(tokens[15].type, RenpyTokenType::UnsupportedConstruct);
    EXPECT_EQ(tokens[15].unsupported_kind, RenpyUnsupportedKind::Audio);

    EXPECT_EQ(tokens[16].type, RenpyTokenType::Menu);
    EXPECT_EQ(tokens[17].type, RenpyTokenType::MenuChoice);
    EXPECT_EQ(tokens[17].primary, "继续");
    EXPECT_EQ(tokens[17].secondary, "affection == True");
    EXPECT_EQ(tokens[18].type, RenpyTokenType::Jump);
    EXPECT_EQ(tokens[18].primary, "next_scene");

    EXPECT_EQ(tokens[19].type, RenpyTokenType::Call);
    EXPECT_EQ(tokens[19].primary, "side_route");

    EXPECT_EQ(tokens[20].type, RenpyTokenType::Return);
    EXPECT_EQ(tokens[20].primary, "done");

    EXPECT_EQ(tokens[21].type, RenpyTokenType::Eof);
}

TEST(Renpy2NovaEmitterTest, MapsPracticalShowTransitionAndMusicOptionsConservatively) {
    RenpyProject project;

    RenpyNode label;
    label.kind = RenpyNodeKind::Label;
    label.name = "start";
    label.line = 1;

    RenpyNode show;
    show.kind = RenpyNodeKind::Show;
    show.name = "eileen happy at left";
    show.line = 2;

    RenpyNode with_dissolve;
    with_dissolve.kind = RenpyNodeKind::With;
    with_dissolve.unsupported_kind = RenpyUnsupportedKind::With;
    with_dissolve.name = "dissolve";
    with_dissolve.value.clear();
    with_dissolve.line = 3;

    RenpyNode play_music;
    play_music.kind = RenpyNodeKind::PlayMusic;
    play_music.name = "\"theme.ogg\" loop volume 0.3";
    play_music.line = 4;

    RenpyNode play_sound;
    play_sound.kind = RenpyNodeKind::PlaySound;
    play_sound.name = "\"click.wav\"";
    play_sound.line = 5;

    label.children = {show, with_dissolve, play_music, play_sound};
    project.statements = {label};

    NovamarkEmitter emitter;
    auto result = emitter.emit(project);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(
        result.unwrap(),
        "#scene_start \"start\"\n"
        "@sprite eileen show happy position:left transition:dissolve\n"
        "@bgm theme.ogg loop:true volume:0.3\n"
        "@sfx click.wav\n"
    );

    ASSERT_EQ(result.report().entries().size(), 1);
    EXPECT_EQ(result.report().entries()[0].feature_tag, "with");
    EXPECT_EQ(result.report().entries()[0].kind, ConversionEntryKind::PartiallySupported);
    EXPECT_EQ(result.report().entries()[0].severity, ConversionSeverity::Info);
}

TEST(Renpy2NovaEmitterTest, KeepsAmbiguousMediaAndUnsupportedAudioExplicit) {
    RenpyProject project;

    RenpyNode label;
    label.kind = RenpyNodeKind::Label;
    label.name = "start";
    label.line = 1;

    RenpyNode show;
    show.kind = RenpyNodeKind::Show;
    show.name = "eileen happy at center";
    show.line = 2;

    RenpyNode with_pixellate;
    with_pixellate.kind = RenpyNodeKind::With;
    with_pixellate.unsupported_kind = RenpyUnsupportedKind::With;
    with_pixellate.name = "pixellate";
    with_pixellate.value.clear();
    with_pixellate.line = 3;

    RenpyNode play_music;
    play_music.kind = RenpyNodeKind::PlayMusic;
    play_music.name = "\"theme.ogg\" if_changed";
    play_music.line = 4;

    RenpyNode play_sound;
    play_sound.kind = RenpyNodeKind::PlaySound;
    play_sound.name = "\"click.wav\" loop";
    play_sound.line = 5;

    label.children = {show, with_pixellate, play_music, play_sound};
    project.statements = {label};

    NovamarkEmitter emitter;
    auto result = emitter.emit(project);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(
        result.unwrap(),
        "#scene_start \"start\"\n"
        "// TODO source: line 2; reason: This Ren'Py statement is not recognized yet and cannot be converted reliably.; action: Inspect it manually and add the corresponding NovaMark logic.; original: show eileen happy at center\n"
        "// TODO source: line 3; reason: The with transition was recorded only as a comment; no executable transition command was generated.; action: Add the corresponding NovaMark transition manually based on the intended effect.; original: with pixellate\n"
        "// TODO source: line 4; reason: This audio statement was only partially recognized, so no NovaMark audio command was generated safely.; action: Add @bgm / @sfx manually, or confirm that the statement can be ignored.; original: play music \"theme.ogg\" if_changed\n"
        "// TODO source: line 5; reason: This audio statement was only partially recognized, so no NovaMark audio command was generated safely.; action: Add @bgm / @sfx manually, or confirm that the statement can be ignored.; original: play sound \"click.wav\" loop\n"
    );

    ASSERT_EQ(result.report().entries().size(), 4);
    EXPECT_EQ(result.report().entries()[0].feature_tag, "show");
    EXPECT_EQ(result.report().entries()[0].kind, ConversionEntryKind::Unsupported);
    EXPECT_EQ(result.report().entries()[1].feature_tag, "with");
    EXPECT_EQ(result.report().entries()[2].feature_tag, "audio");
    EXPECT_EQ(result.report().entries()[3].feature_tag, "audio");
}

TEST(Renpy2NovaAnalyzerTest, BuildsStructuredTreeForLabelsConditionalsAndMenuChoices) {
    const std::string source =
        "define e = Character(\"Eileen\")\n"
        "default affection = 0\n"
        "label start:\n"
        "    e \"你好\"\n"
        "    if affection == True:\n"
        "        play music \"theme.ogg\"\n"
        "    elif affection == False:\n"
        "        play sound \"click.wav\"\n"
        "    else:\n"
        "        $ affection = None\n"
        "    menu:\n"
        "        \"继续\" if affection == True:\n"
        "            jump next_scene\n"
        "        \"支线\":\n"
            "            call side_route\n"
        "    return\n";

    RenpyLexer lexer;
    auto token_result = lexer.tokenize(source);
    ASSERT_TRUE(token_result.is_ok());

    RenpyAnalyzer analyzer;
    auto project_result = analyzer.analyze(token_result.unwrap());
    ASSERT_TRUE(project_result.is_ok());

    const RenpyProject& project = project_result.unwrap();
    ASSERT_EQ(project.statements.size(), 3);

    EXPECT_EQ(project.statements[0].kind, RenpyNodeKind::CharacterDefinition);
    EXPECT_EQ(project.statements[0].name, "e");

    EXPECT_EQ(project.statements[1].kind, RenpyNodeKind::DefaultStatement);
    EXPECT_EQ(project.statements[1].name, "affection");
    EXPECT_EQ(project.statements[1].value, "0");

    const RenpyNode& label = project.statements[2];
    ASSERT_EQ(label.kind, RenpyNodeKind::Label);
    EXPECT_EQ(label.name, "start");
    ASSERT_EQ(label.children.size(), 4);

    EXPECT_EQ(label.children[0].kind, RenpyNodeKind::Say);
    EXPECT_EQ(label.children[0].name, "e");
    EXPECT_EQ(label.children[0].value, "你好");

    const RenpyNode& conditional = label.children[1];
    ASSERT_EQ(conditional.kind, RenpyNodeKind::If);
    EXPECT_EQ(conditional.name, "affection == True");
    ASSERT_EQ(conditional.children.size(), 1);
    EXPECT_EQ(conditional.children[0].kind, RenpyNodeKind::PlayMusic);
    EXPECT_EQ(conditional.children[0].name, "\"theme.ogg\"");
    ASSERT_EQ(conditional.else_children.size(), 1);
    EXPECT_EQ(conditional.else_children[0].kind, RenpyNodeKind::If);
    EXPECT_EQ(conditional.else_children[0].name, "affection == False");
    ASSERT_EQ(conditional.else_children[0].children.size(), 1);
    EXPECT_EQ(conditional.else_children[0].children[0].kind, RenpyNodeKind::PlaySound);
    ASSERT_EQ(conditional.else_children[0].else_children.size(), 1);
    EXPECT_EQ(conditional.else_children[0].else_children[0].kind, RenpyNodeKind::DollarStatement);
    EXPECT_EQ(conditional.else_children[0].else_children[0].name, "affection = None");

    const RenpyNode& menu = label.children[2];
    ASSERT_EQ(menu.kind, RenpyNodeKind::Menu);
    ASSERT_EQ(menu.children.size(), 2);

    EXPECT_EQ(menu.children[0].kind, RenpyNodeKind::MenuChoice);
    EXPECT_EQ(menu.children[0].value, "继续");
    EXPECT_EQ(menu.children[0].name, "affection == True");
    ASSERT_EQ(menu.children[0].children.size(), 1);
    EXPECT_EQ(menu.children[0].children[0].kind, RenpyNodeKind::Jump);
    EXPECT_EQ(menu.children[0].children[0].name, "next_scene");

    EXPECT_EQ(menu.children[1].kind, RenpyNodeKind::MenuChoice);
    EXPECT_EQ(menu.children[1].value, "支线");
    ASSERT_EQ(menu.children[1].children.size(), 1);
    EXPECT_EQ(menu.children[1].children[0].kind, RenpyNodeKind::Call);
    EXPECT_EQ(menu.children[1].children[0].name, "side_route");

    EXPECT_EQ(label.children[3].kind, RenpyNodeKind::Return);
}

TEST(Renpy2NovaAnalyzerTest, ClassifiesUnsupportedConstructsAndPythonMarkers) {
    const std::string source =
        "label start:\n"
        "    python:\n"
        "        $ score = 1\n"
        "    init python:\n"
        "        $ ready = True\n"
        "    with dissolve\n"
        "    screen inventory():\n"
        "        \"ignored\"\n"
        "    transform fade_in:\n"
        "        \"ignored\"\n"
        "    image eileen happy = \"eileen_happy.png\"\n"
        "    play voice \"line.ogg\"\n";

    RenpyLexer lexer;
    auto token_result = lexer.tokenize(source);
    ASSERT_TRUE(token_result.is_ok());

    RenpyAnalyzer analyzer;
    auto project_result = analyzer.analyze(token_result.unwrap());
    ASSERT_TRUE(project_result.is_ok());

    const RenpyNode& label = project_result.unwrap().statements[0];
    ASSERT_EQ(label.kind, RenpyNodeKind::Label);
    ASSERT_EQ(label.children.size(), 7);

    EXPECT_EQ(label.children[0].kind, RenpyNodeKind::PythonBlock);
    ASSERT_EQ(label.children[0].children.size(), 1);
    EXPECT_EQ(label.children[0].children[0].kind, RenpyNodeKind::DollarStatement);
    EXPECT_EQ(label.children[0].children[0].name, "score = 1");

    EXPECT_EQ(label.children[1].kind, RenpyNodeKind::InitPythonBlock);
    ASSERT_EQ(label.children[1].children.size(), 1);
    EXPECT_EQ(label.children[1].children[0].kind, RenpyNodeKind::DollarStatement);
    EXPECT_EQ(label.children[1].children[0].name, "ready = True");

    EXPECT_EQ(label.children[2].kind, RenpyNodeKind::With);
    EXPECT_EQ(label.children[2].unsupported_kind, RenpyUnsupportedKind::With);

    EXPECT_EQ(label.children[3].kind, RenpyNodeKind::Unsupported);
    EXPECT_EQ(label.children[3].unsupported_kind, RenpyUnsupportedKind::Screen);
    ASSERT_EQ(label.children[3].children.size(), 1);
    EXPECT_EQ(label.children[3].children[0].kind, RenpyNodeKind::Narration);

    EXPECT_EQ(label.children[4].kind, RenpyNodeKind::Unsupported);
    EXPECT_EQ(label.children[4].unsupported_kind, RenpyUnsupportedKind::Transform);

    EXPECT_EQ(label.children[5].kind, RenpyNodeKind::Unsupported);
    EXPECT_EQ(label.children[5].unsupported_kind, RenpyUnsupportedKind::Image);

    EXPECT_EQ(label.children[6].kind, RenpyNodeKind::Unsupported);
    EXPECT_EQ(label.children[6].unsupported_kind, RenpyUnsupportedKind::Audio);
}

TEST(Renpy2NovaPipelineTest, AnalyzerEmitterAndConverterRemainCompileReady) {
    const std::string none_literal = reserved_none_literal();
    const std::string source =
        "define e = Character(\"Eileen\", color=\"#C0FFEE\", image=\"eileen_default\")\n"
        "default affection = True\n"
        "label start:\n"
        "    scene bg room with fade\n"
        "    show eileen happy at left\n"
        "    with dissolve\n"
        "    e \"Hello\"\n"
        "    \"Narration\"\n"
        "    $ affinity = None\n"
        "    if affection == True:\n"
        "        play music \"theme.ogg\" loop volume 0.8\n"
        "    elif affection == False:\n"
        "        play sound \"click.wav\" volume 0.5\n"
        "    else:\n"
        "        play voice \"line.ogg\"\n"
        "    menu:\n"
        "        \"Continue\" if affection == True:\n"
        "            jump next_scene\n"
        "        \"Side\":\n"
        "            call side_route\n"
        "    python:\n"
        "        $ score = True\n"
        "    scene bg park with fade\n"
        "    image eileen happy = \"eileen_happy.png\"\n";

    const std::string expected_output =
        std::string(
            "@char Eileen\n"
            "    color: #C0FFEE\n"
            "    sprite_default: eileen_default.png\n"
            "@end\n"
            "\n"
            "@var affection = true\n"
            "\n"
            "#scene_start \"start\"\n"
            "@bg room transition:fade\n"
            "@sprite eileen show happy position:left transition:dissolve\n"
            "Eileen: Hello\n"
            "> Narration\n"
            "@set affinity = ")
        + none_literal +
        "\n"
        "if affection == true\n"
        "    @bgm theme.ogg loop:true volume:0.8\n"
        "else\n"
        "    if affection == false\n"
        "        @sfx click.wav volume:0.5\n"
        "    else\n"
        "        // TODO source: line 15; reason: This audio statement was only partially recognized, so no NovaMark audio command was generated safely.; action: Add @bgm / @sfx manually, or confirm that the statement can be ignored.; original: play voice \"line.ogg\"\n"
        "    endif\n"
        "endif\n"
        "? Choose an option\n"
        "- [Continue] -> scene_next_scene if affection == true\n"
        "- [Side]\n"
        "    @call scene_side_route\n"
        "// TODO source: line 21; reason: Python blocks cannot be mapped safely to NovaMark.; action: Rewrite this block as explicit script commands or port the logic manually.; original: python:\n"
        "    @set score = true\n"
        "@bg park transition:fade\n"
        "// TODO source: line 24; reason: Image definitions must be migrated separately at the resource layer.; action: Create the resource mapping manually and add the required character or sprite configuration.; original: image eileen happy = \"eileen_happy.png\"\n";

    RenpyLexer lexer;
    auto token_result = lexer.tokenize(source);
    ASSERT_TRUE(token_result.is_ok());

    RenpyAnalyzer analyzer;
    auto project_result = analyzer.analyze(token_result.unwrap());
    ASSERT_TRUE(project_result.is_ok());
    ASSERT_EQ(project_result.unwrap().statements.size(), 3);
    EXPECT_EQ(project_result.unwrap().statements[0].kind, RenpyNodeKind::CharacterDefinition);
    EXPECT_EQ(project_result.unwrap().statements[1].kind, RenpyNodeKind::DefaultStatement);
    EXPECT_EQ(project_result.unwrap().statements[2].kind, RenpyNodeKind::Label);

    NovamarkEmitter emitter;
    auto emit_result = emitter.emit(project_result.unwrap());
    ASSERT_TRUE(emit_result.is_ok());
    EXPECT_EQ(emit_result.unwrap(), expected_output);
    ASSERT_EQ(emit_result.report().entries().size(), 4);
    EXPECT_EQ(emit_result.report().entries()[0].kind, ConversionEntryKind::PartiallySupported);
    EXPECT_EQ(emit_result.report().entries()[0].feature_tag, "with");
    EXPECT_EQ(emit_result.report().entries()[1].kind, ConversionEntryKind::PartiallySupported);
    EXPECT_EQ(emit_result.report().entries()[1].feature_tag, "audio");
    EXPECT_EQ(emit_result.report().entries()[2].kind, ConversionEntryKind::ManualFixRequired);
    EXPECT_EQ(emit_result.report().entries()[2].feature_tag, "python");
    EXPECT_EQ(emit_result.report().entries()[3].kind, ConversionEntryKind::Unsupported);
    EXPECT_EQ(emit_result.report().entries()[3].feature_tag, "image");

    Converter converter;
    ConversionInput input{source, "script.rpy"};
    auto convert_result = converter.convert(input);
    ASSERT_TRUE(convert_result.is_ok());
    EXPECT_EQ(convert_result.unwrap().novamark_source, expected_output);
    EXPECT_FALSE(convert_result.unwrap().report.has_errors());
    ASSERT_EQ(convert_result.unwrap().report.entries().size(), 4);
}

TEST(Renpy2NovaEmitterTest, EmitsCharacterMenuAndInlineTodoComments) {
    RenpyProject project;

    RenpyNode character;
    character.kind = RenpyNodeKind::CharacterDefinition;
    character.name = "n";
    character.value = "Character(\"Narrator\")";
    character.line = 1;

    RenpyNode label;
    label.kind = RenpyNodeKind::Label;
    label.name = "intro";
    label.line = 2;

    RenpyNode menu;
    menu.kind = RenpyNodeKind::Menu;
    menu.line = 3;

    RenpyNode choice;
    choice.kind = RenpyNodeKind::MenuChoice;
    choice.value = "Open door";
    choice.line = 4;

    RenpyNode call;
    call.kind = RenpyNodeKind::Call;
    call.name = "side_room";
    call.line = 5;
    choice.children.push_back(call);

    RenpyNode unsupported;
    unsupported.kind = RenpyNodeKind::Unsupported;
    unsupported.unsupported_kind = RenpyUnsupportedKind::Screen;
    unsupported.name = "screen";
    unsupported.value = "inventory()";
    unsupported.line = 6;

    menu.children.push_back(choice);
    label.children.push_back(menu);
    label.children.push_back(unsupported);

    project.statements.push_back(character);
    project.statements.push_back(label);

    NovamarkEmitter emitter;
    auto result = emitter.emit(project);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(
        result.unwrap(),
        "@char Narrator\n"
        "@end\n"
        "\n"
        "#scene_intro \"intro\"\n"
        "? Choose an option\n"
        "- [Open door]\n"
        "    @call scene_side_room\n"
        "// TODO source: line 6; reason: screen language is not supported for automatic conversion in the current version.; action: Rewrite it manually as scene, menu, or UI state script content.; original: screen inventory():\n"
    );

    ASSERT_EQ(result.report().entries().size(), 1);
    EXPECT_EQ(result.report().entries()[0].feature_tag, "screen");
    EXPECT_TRUE(result.report().needs_manual_intervention());
}

TEST(Renpy2NovaEmitterTest, PreservesPreludeBeforeTerminalCallInMenuChoiceBody) {
    RenpyProject project;

    RenpyNode label;
    label.kind = RenpyNodeKind::Label;
    label.name = "start";
    label.line = 1;

    RenpyNode menu;
    menu.kind = RenpyNodeKind::Menu;
    menu.value = "请选择行动";
    menu.line = 2;

    RenpyNode choice;
    choice.kind = RenpyNodeKind::MenuChoice;
    choice.value = "准备后调查";
    choice.line = 3;

    RenpyNode prelude;
    prelude.kind = RenpyNodeKind::DollarStatement;
    prelude.name = "clue_ready = True";
    prelude.line = 4;

    RenpyNode call;
    call.kind = RenpyNodeKind::Call;
    call.name = "secret_route";
    call.line = 5;

    choice.children = {prelude, call};
    menu.children.push_back(choice);
    label.children.push_back(menu);
    project.statements = {label};

    NovamarkEmitter emitter;
    auto result = emitter.emit(project);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(
        result.unwrap(),
        "#scene_start \"start\"\n"
        "? 请选择行动\n"
        "- [准备后调查]\n"
        "    @set clue_ready = true\n"
        "    @call scene_secret_route\n"
    );
    EXPECT_TRUE(result.report().entries().empty());
}

TEST(Renpy2NovaEmitterTest, PreservesSafeTerminalCallInMenuChoiceBody) {
    RenpyProject project;

    RenpyNode label;
    label.kind = RenpyNodeKind::Label;
    label.name = "start";
    label.line = 1;

    RenpyNode menu;
    menu.kind = RenpyNodeKind::Menu;
    menu.value = "请选择去向";
    menu.line = 2;

    RenpyNode choice;
    choice.kind = RenpyNodeKind::MenuChoice;
    choice.value = "进入支线";
    choice.line = 3;

    RenpyNode call;
    call.kind = RenpyNodeKind::Call;
    call.name = "side_route";
    call.line = 4;

    choice.children = {call};
    menu.children.push_back(choice);
    label.children.push_back(menu);
    project.statements = {label};

    NovamarkEmitter emitter;
    auto result = emitter.emit(project);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(
        result.unwrap(),
        "#scene_start \"start\"\n"
        "? 请选择去向\n"
        "- [进入支线]\n"
        "    @call scene_side_route\n"
    );
    EXPECT_TRUE(result.report().entries().empty());
}

TEST(Renpy2NovaEmitterTest, DegradesUnsafeCallPlusTailMenuChoiceBodyToManualFix) {
    RenpyProject project;

    RenpyNode label;
    label.kind = RenpyNodeKind::Label;
    label.name = "start";
    label.line = 1;

    RenpyNode menu;
    menu.kind = RenpyNodeKind::Menu;
    menu.value = "请选择路线";
    menu.line = 2;

    RenpyNode choice;
    choice.kind = RenpyNodeKind::MenuChoice;
    choice.value = "先调用再说话";
    choice.line = 3;

    RenpyNode call;
    call.kind = RenpyNodeKind::Call;
    call.name = "side_route";
    call.line = 4;

    RenpyNode narration;
    narration.kind = RenpyNodeKind::Narration;
    narration.value = "这一句让菜单体不再安全。";
    narration.line = 5;

    choice.children = {call, narration};
    menu.children.push_back(choice);
    label.children.push_back(menu);
    project.statements = {label};

    NovamarkEmitter emitter;
    auto result = emitter.emit(project);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(
        result.unwrap(),
        "#scene_start \"start\"\n"
        "? 请选择路线\n"
        "- [先调用再说话]\n"
        "    // TODO source: line 3; reason: Menu choice body contains a call shape that cannot be converted safely to NovaMark block choices.; action: Rewrite this choice body manually so control flow after the call is preserved explicitly, or simplify it to an allowed @set prelude plus terminal @call.; original: \"先调用再说话\":\n"
    );

    ASSERT_EQ(result.report().entries().size(), 1);
    EXPECT_EQ(result.report().entries()[0].kind, ConversionEntryKind::ManualFixRequired);
    EXPECT_EQ(result.report().entries()[0].severity, ConversionSeverity::Warning);
    EXPECT_EQ(result.report().entries()[0].feature_tag, "menu_call_body");
    EXPECT_EQ(result.report().entries()[0].source_range.start_line, 3U);
    EXPECT_TRUE(result.report().needs_manual_intervention());
}

TEST(Renpy2NovaEmitterTest, DegradesUnsafeNonWhitelistedPreludeBeforeTerminalCallToManualFix) {
    RenpyProject project;

    RenpyNode label;
    label.kind = RenpyNodeKind::Label;
    label.name = "start";
    label.line = 1;

    RenpyNode menu;
    menu.kind = RenpyNodeKind::Menu;
    menu.value = "请选择行动";
    menu.line = 2;

    RenpyNode choice;
    choice.kind = RenpyNodeKind::MenuChoice;
    choice.value = "先说再调用";
    choice.line = 3;

    RenpyNode narration;
    narration.kind = RenpyNodeKind::Narration;
    narration.value = "这句前置旁白不在安全白名单中。";
    narration.line = 4;

    RenpyNode call;
    call.kind = RenpyNodeKind::Call;
    call.name = "side_route";
    call.line = 5;

    choice.children = {narration, call};
    menu.children.push_back(choice);
    label.children.push_back(menu);
    project.statements = {label};

    NovamarkEmitter emitter;
    auto result = emitter.emit(project);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(
        result.unwrap(),
        "#scene_start \"start\"\n"
        "? 请选择行动\n"
        "- [先说再调用]\n"
        "    // TODO source: line 3; reason: Menu choice body contains a call shape that cannot be converted safely to NovaMark block choices.; action: Rewrite this choice body manually so control flow after the call is preserved explicitly, or simplify it to an allowed @set prelude plus terminal @call.; original: \"先说再调用\":\n"
    );

    ASSERT_EQ(result.report().entries().size(), 1);
    EXPECT_EQ(result.report().entries()[0].kind, ConversionEntryKind::ManualFixRequired);
    EXPECT_EQ(result.report().entries()[0].severity, ConversionSeverity::Warning);
    EXPECT_EQ(result.report().entries()[0].feature_tag, "menu_call_body");
    EXPECT_EQ(result.report().entries()[0].source_range.start_line, 3U);
    EXPECT_TRUE(result.report().needs_manual_intervention());
}

TEST(Renpy2NovaEmitterTest, EmitsCharacterMetadataConditionalsAndAudioMappings) {
    RenpyProject project;

    RenpyNode character;
    character.kind = RenpyNodeKind::CharacterDefinition;
    character.name = "e";
    character.value = "Character(\"Eileen\", color=\"#C0FFEE\", image=\"eileen_default\")";
    character.line = 1;

    RenpyNode label;
    label.kind = RenpyNodeKind::Label;
    label.name = "start";
    label.line = 2;

    RenpyNode scene;
    scene.kind = RenpyNodeKind::Scene;
    scene.name = "bg room with fade";
    scene.line = 3;

    RenpyNode show;
    show.kind = RenpyNodeKind::Show;
    show.name = "eileen happy at right";
    show.line = 4;

    RenpyNode with_dissolve;
    with_dissolve.kind = RenpyNodeKind::With;
    with_dissolve.unsupported_kind = RenpyUnsupportedKind::With;
    with_dissolve.name = "dissolve";
    with_dissolve.line = 5;

    RenpyNode conditional;
    conditional.kind = RenpyNodeKind::If;
    conditional.name = "flag == True";
    conditional.line = 6;

    RenpyNode play_music;
    play_music.kind = RenpyNodeKind::PlayMusic;
    play_music.name = "\"theme.ogg\" loop volume 0.8";
    play_music.line = 7;
    conditional.children.push_back(play_music);

    RenpyNode nested_if;
    nested_if.kind = RenpyNodeKind::If;
    nested_if.name = "flag == False";
    nested_if.line = 8;

    RenpyNode play_sound;
    play_sound.kind = RenpyNodeKind::PlaySound;
    play_sound.name = "\"click.wav\" volume 0.5";
    play_sound.line = 9;
    nested_if.children.push_back(play_sound);

    RenpyNode unsupported_audio;
    unsupported_audio.kind = RenpyNodeKind::Unsupported;
    unsupported_audio.unsupported_kind = RenpyUnsupportedKind::Audio;
    unsupported_audio.name = "audio";
    unsupported_audio.value = "voice \"line.ogg\"";
    unsupported_audio.line = 10;
    nested_if.else_children.push_back(unsupported_audio);

    conditional.else_children.push_back(nested_if);

    RenpyNode menu;
    menu.kind = RenpyNodeKind::Menu;
    menu.value = "选择去向";
    menu.line = 11;

    RenpyNode choice;
    choice.kind = RenpyNodeKind::MenuChoice;
    choice.name = "flag == True";
    choice.value = "继续";
    choice.line = 12;

    RenpyNode jump;
    jump.kind = RenpyNodeKind::Jump;
    jump.name = "next_scene";
    jump.line = 13;
    choice.children.push_back(jump);
    menu.children.push_back(choice);

    label.children = {scene, show, with_dissolve, conditional, menu};
    project.statements = {character, label};

    NovamarkEmitter emitter;
    auto result = emitter.emit(project);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(
        result.unwrap(),
        "@char Eileen\n"
        "    color: #C0FFEE\n"
        "    sprite_default: eileen_default.png\n"
        "@end\n"
        "\n"
        "#scene_start \"start\"\n"
        "@bg room transition:fade\n"
        "@sprite eileen show happy position:right transition:dissolve\n"
        "if flag == true\n"
        "    @bgm theme.ogg loop:true volume:0.8\n"
        "else\n"
        "    if flag == false\n"
        "        @sfx click.wav volume:0.5\n"
        "    else\n"
        "        // TODO source: line 10; reason: This audio statement was only partially recognized, so no NovaMark audio command was generated safely.; action: Add @bgm / @sfx manually, or confirm that the statement can be ignored.; original: play voice \"line.ogg\"\n"
        "    endif\n"
        "endif\n"
        "? 选择去向\n"
        "- [继续] -> scene_next_scene if flag == true\n"
    );

    ASSERT_EQ(result.report().entries().size(), 2);
    EXPECT_EQ(result.report().entries()[0].feature_tag, "with");
    EXPECT_EQ(result.report().entries()[0].kind, ConversionEntryKind::PartiallySupported);
    EXPECT_EQ(result.report().entries()[1].feature_tag, "audio");
    EXPECT_EQ(result.report().entries()[1].kind, ConversionEntryKind::PartiallySupported);
}

TEST(Renpy2NovaEmitterTest, AlignsSpriteDialogueAssignmentsAndReturnSyntax) {
    RenpyProject project;

    RenpyNode character;
    character.kind = RenpyNodeKind::CharacterDefinition;
    character.name = "e";
    character.value = "Character(\"Eileen\")";
    character.line = 1;

    RenpyNode label;
    label.kind = RenpyNodeKind::Label;
    label.name = "start";
    label.line = 2;

    RenpyNode scene;
    scene.kind = RenpyNodeKind::Scene;
    scene.name = "bg room";
    scene.line = 3;

    RenpyNode show;
    show.kind = RenpyNodeKind::Show;
    show.name = "eileen happy at left";
    show.line = 4;

    RenpyNode hide;
    hide.kind = RenpyNodeKind::Hide;
    hide.name = "eileen";
    hide.line = 5;

    RenpyNode say;
    say.kind = RenpyNodeKind::Say;
    say.name = "e";
    say.value = "你好";
    say.line = 6;

    RenpyNode narration;
    narration.kind = RenpyNodeKind::Narration;
    narration.value = "旁白";
    narration.line = 7;

    RenpyNode assign;
    assign.kind = RenpyNodeKind::DollarStatement;
    assign.name = "affection += 1";
    assign.line = 8;

    RenpyNode menu;
    menu.kind = RenpyNodeKind::Menu;
    menu.value = "你要怎么回应？";
    menu.line = 9;

    RenpyNode choice;
    choice.kind = RenpyNodeKind::MenuChoice;
    choice.value = "继续";
    choice.line = 10;

    RenpyNode jump;
    jump.kind = RenpyNodeKind::Jump;
    jump.name = "next_scene";
    jump.line = 11;
    choice.children.push_back(jump);
    menu.children.push_back(choice);

    RenpyNode ret;
    ret.kind = RenpyNodeKind::Return;
    ret.name = "done";
    ret.line = 12;

    label.children = {scene, show, hide, say, narration, assign, menu, ret};
    project.statements = {character, label};

    NovamarkEmitter emitter;
    auto result = emitter.emit(project);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(
        result.unwrap(),
        "@char Eileen\n"
        "@end\n"
        "\n"
        "#scene_start \"start\"\n"
        "@bg room\n"
        "@sprite eileen show happy position:left\n"
        "@sprite eileen hide\n"
        "Eileen: 你好\n"
        "> 旁白\n"
        "@set affection = affection + 1\n"
        "? 你要怎么回应？\n"
        "- [继续] -> scene_next_scene\n"
        "@return\n"
        "    // TODO source: line 12; reason: This Ren'Py statement is not recognized yet and cannot be converted reliably.; action: Inspect it manually and add the corresponding NovaMark logic.; original: return done\n"
    );

    ASSERT_EQ(result.report().entries().size(), 1);
    EXPECT_EQ(result.report().entries()[0].feature_tag, "return_value");
    EXPECT_EQ(result.report().entries()[0].kind, ConversionEntryKind::Unsupported);
}

TEST(Renpy2NovaEmitterTest, MarksUnsafeMenuCallBodyAsManualFixWithMenuCallBodyTag) {
    const std::string source =
        "label start:\n"
        "    menu \"请选择\":\n"
        "        \"危险分支\":\n"
        "            call side_story\n"
        "            $ tail_seen = True\n"
        "    return\n"
        "\n"
        "label side_story:\n"
        "    return\n";

    Converter converter;
    auto convert_result = converter.convert(ConversionInput{source, "inline_menu_call_tail.rpy"});

    ASSERT_TRUE(convert_result.is_ok());
    const ConversionOutput& output = convert_result.unwrap();
    const auto manual_fix_entries = output.report.entries_by_kind(ConversionEntryKind::ManualFixRequired);

    ASSERT_EQ(manual_fix_entries.size(), 1U);
    EXPECT_EQ(manual_fix_entries[0].feature_tag, "menu_call_body");
    EXPECT_EQ(manual_fix_entries[0].source_range.start_line, 3U);
    EXPECT_NE(output.novamark_source.find("// TODO source: line 3; reason: Menu choice body contains a call shape that cannot be converted safely to NovaMark block choices."), std::string::npos);
    EXPECT_EQ(output.novamark_source.find("@call scene_side_story"), std::string::npos);
}

TEST(Renpy2NovaFixtureTest, ConvertsSimpleDialogueSceneFixtureExactly) {
    expect_fixture_conversion({
        "simple_dialogue_scene",
        0,
        0,
        0,
        0,
        false,
    });
}

TEST(Renpy2NovaFixtureTest, ConvertsMenuBranchCallJumpFixtureWithTerminalCallChoiceBodyExactly) {
    expect_fixture_conversion({
        "menu_branch_call_jump",
        0,
        0,
        0,
        0,
        false,
    });
}

TEST(Renpy2NovaFixtureTest, ConvertsMenuBranchTerminalCallSafeFixtureExactly) {
    expect_fixture_conversion({
        "menu_branch_terminal_call_safe",
        0,
        0,
        0,
        0,
        false,
    });
}

TEST(Renpy2NovaFixtureTest, ConvertsMenuBranchPreludeCallFixtureExactly) {
    expect_fixture_conversion({
        "menu_branch_prelude_call",
        0,
        0,
        0,
        0,
        false,
    });
}

TEST(Renpy2NovaFixtureTest, ConvertsMenuBranchCallTailUnsafeFixtureWithManualFixReport) {
    expect_fixture_conversion({
        "menu_branch_call_tail_unsafe",
        1,
        1,
        0,
        0,
        true,
    });
}

TEST(Renpy2NovaFixtureTest, ConvertsMenuBranchNonWhitelistedPreludeUnsafeFixtureWithManualFixReport) {
    expect_fixture_conversion({
        "menu_branch_nonwhitelisted_prelude_unsafe",
        1,
        1,
        0,
        0,
        true,
    });
}

TEST(Renpy2NovaFixtureTest, ConvertsUnsupportedDegradationFixtureWithExpectedReportCounts) {
    expect_fixture_conversion({
        "unsupported_degradation",
        5,
        2,
        2,
        1,
        true,
    });
}

TEST(Renpy2NovaFixtureTest, ConvertsAdvancedCoverageFixtureExactly) {
    expect_fixture_conversion({
        "advanced_practical_coverage",
        2,
        0,
        2,
        0,
        false,
    });
}
