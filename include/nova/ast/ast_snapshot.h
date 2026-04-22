#pragma once

#include "nova/ast/ast_node.h"

#include <string>
#include <vector>

namespace nova {

struct MemoryScript;

std::string export_ast_snapshot_string(const ProgramNode* program);

std::string export_ast_snapshot_string_from_scripts(const std::vector<MemoryScript>& scripts);

std::string export_ast_snapshot_string_from_path(const std::string& path);

/// @brief 将 AST 快照 JSON 转换为按文件分组的源码 JSON
/// @param snapshot_json AST 快照 JSON 字符串
/// @return {"version":1,"files":[{"file":"...","content":"..."}]}
std::string ast_snapshot_to_source_files_json(const std::string& snapshot_json);

}
