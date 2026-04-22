#pragma once

#include "../core/filesystem_compat.h"
#include "../report/conversion_report.h"
#include "../resource/resource_manifest.h"

#include <vector>

namespace nova::renpy2nova {

/// @brief 项目模式转换配置。
struct ProjectConversionOptions {
    filesystem_compat::path project_root; ///< Ren'Py 项目根目录
    filesystem_compat::path output_dir;   ///< 输出根目录
    bool include_review_items = true;   ///< 是否在 manifest 中附带部分支持复核项
};

/// @brief 单个脚本文件的项目转换结果摘要。
struct ProjectFileConversionResult {
    filesystem_compat::path source_relative_path; ///< 相对项目根目录的源路径
    filesystem_compat::path output_relative_path; ///< 相对输出根目录的 .nvm 路径
    filesystem_compat::path report_relative_path; ///< 相对输出根目录的 JSON 报告路径
    bool converted = false;                     ///< 是否成功生成 .nvm 输出
    bool needs_manual_intervention = false;    ///< 是否存在需人工处理项
    size_t diagnostic_count = 0;               ///< diagnostics 数量
    ConversionSummary summary;                 ///< 当前文件的条目/级别汇总
};

/// @brief 需要汇总进手动修复 manifest 的条目。
struct ProjectManifestItem {
    filesystem_compat::path source_relative_path; ///< 相对项目根目录的源路径
    filesystem_compat::path report_relative_path; ///< 相对输出根目录的 JSON 报告路径
    ConversionEntry entry;                      ///< 原始转换条目
};

/// @brief 项目级转换统计。
struct ProjectConversionTotals {
    size_t total_files = 0;                    ///< 扫描到的 .rpy 文件总数
    size_t converted_files = 0;                ///< 成功生成 .nvm 的文件数
    size_t failed_files = 0;                   ///< 转换失败或写出失败的文件数
    size_t files_with_manual_intervention = 0; ///< 包含 Unsupported/ManualFixRequired 的文件数
    size_t total_diagnostics = 0;              ///< diagnostics 总数
    ConversionSummary summary;                 ///< 全项目条目/级别汇总
};

/// @brief 项目模式整体转换结果。
struct ProjectConversionOutput {
    filesystem_compat::path project_root;              ///< 项目根目录
    filesystem_compat::path output_dir;                ///< 输出根目录
    std::vector<ProjectFileConversionResult> files;     ///< 每个脚本文件的摘要
    std::vector<ProjectManifestItem> manual_fix_items;  ///< Unsupported/ManualFixRequired 条目
    std::vector<ProjectManifestItem> review_items;      ///< PartiallySupported 复核条目
    ResourceManifest resource_manifest;                 ///< 项目级资源线索清单
    ProjectConversionTotals totals;                     ///< 项目级统计

    /// @brief 是否存在失败文件。
    bool has_failures() const;
};

/// @brief Ren'Py 项目目录到 NovaMark 输出目录的批量转换门面。
class ProjectConverter {
public:
    /// @brief 递归扫描并转换项目目录下所有 .rpy 文件。
    /// @param options 项目转换参数
    /// @return 包含已写出产物与项目级统计的结果摘要
    ProjectConversionOutput convert_project(const ProjectConversionOptions& options) const;
};

} // namespace nova::renpy2nova
