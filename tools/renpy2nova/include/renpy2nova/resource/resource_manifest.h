#pragma once

#include "renpy2nova/analyzer/renpy_analyzer.h"
#include "renpy2nova/core/filesystem_compat.h"

#include <string>
#include <vector>

namespace nova::renpy2nova {

/// @brief 资源线索类型。
enum class ResourceKind {
    CharacterDefinition,
    SceneReference,
    ShowReference,
    PlayMusic,
    PlaySound,
    ImageDefinition,
};

/// @brief 单条保守资源线索。
struct ResourceManifestEntry {
    ResourceKind kind = ResourceKind::SceneReference;     ///< 资源线索类别
    filesystem_compat::path source_relative_path;         ///< 相对项目根目录的源脚本路径
    size_t line = 1;                                      ///< 来源行号
    std::string name;                                     ///< 线索名称，如角色别名/场景名/显示名
    std::string path_hint;                                ///< 保守路径提示（若可安全提取）
    std::string display_name;                             ///< 角色显示名（若可安全提取）
    std::string color;                                    ///< 角色颜色（若可安全提取）
    std::string expression;                               ///< show/image 等附加表达式（若可安全提取）
    std::string original_text;                            ///< 原始可读文本片段
};

/// @brief 项目模式资源清单。
struct ResourceManifest {
    filesystem_compat::path project_root;                 ///< 项目根目录
    filesystem_compat::path output_dir;                   ///< 输出根目录
    std::vector<ResourceManifestEntry> entries;           ///< 所有保守资源线索
};

/// @brief 从分析后的 Ren'Py 项目中提取保守资源线索。
/// @param project 分析结果
/// @param source_relative_path 相对项目根目录的源脚本路径
/// @return 当前脚本中提取到的资源线索列表
std::vector<ResourceManifestEntry> extract_resource_manifest_entries(
    const RenpyProject& project,
    const filesystem_compat::path& source_relative_path);

/// @brief 将资源线索类型转换为稳定的机器可读字符串。
/// @param kind 资源线索类型
/// @return 小写下划线英文标识
const char* resource_kind_to_string(ResourceKind kind);

/// @brief 将项目级资源清单序列化为 JSON 文本。
/// @param manifest 资源清单
/// @return 可直接写入文件的 JSON 文本
std::string serialize_resource_manifest_json(const ResourceManifest& manifest);

} // namespace nova::renpy2nova
