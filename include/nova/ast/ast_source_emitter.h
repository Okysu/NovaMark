#pragma once

#include <string>

namespace nova {

/// @brief 将 AST 快照 JSON 转换为按文件分组的 NovaMark 源码 JSON
/// @param snapshot_json AST 快照 JSON，格式为 {"version":1,"root":{"type":"Program","children":[...]}}
/// @return 源码文件 JSON，格式为 {"version":1,"files":[{"file":"...","content":"..."}]}
std::string ast_snapshot_to_source_files_json(const std::string& snapshot_json);

} // namespace nova
