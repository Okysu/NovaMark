#pragma once

#include <iosfwd>
#include <string>

namespace nova::renpy2nova {

/// @brief CLI 执行模式。
enum class CliMode {
    SingleFile,
    Project,
};

/// @brief 命令行解析结果。
struct CliOptions {
    bool show_help = false;          ///< 是否显示帮助
    CliMode mode = CliMode::SingleFile; ///< 当前执行模式
    std::string input_path;          ///< 单文件输入路径
    std::string output_path;         ///< 单文件输出路径
    std::string report_path;         ///< 单文件 JSON 报告路径
    std::string project_path;        ///< 项目根目录
    std::string output_dir;          ///< 项目模式输出根目录
};

/// @brief 命令行解析返回值。
struct CliParseResult {
    bool ok = false;          ///< 是否解析成功
    CliOptions options;       ///< 解析出的选项
    std::string error_message; ///< 失败时的错误信息
};

/// @brief 构建帮助文本。
/// @param program_name 程序名
/// @return 完整帮助文案
std::string build_usage_text(const char* program_name);

/// @brief 解析命令行参数。
/// @param argc 参数数量
/// @param argv 参数数组
/// @return 成功/失败结果；语义校验由 run_application 进一步完成
CliParseResult parse_command_line(int argc, char** argv);

/// @brief 运行 renpy2nova CLI 应用。
/// @param argc 参数数量
/// @param argv 参数数组
/// @param out 标准输出流
/// @param err 标准错误流
/// @return 进程退出码
int run_application(int argc, char** argv, std::ostream& out, std::ostream& err);

} // namespace nova::renpy2nova
