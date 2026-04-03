#pragma once

#include "nova/ast/ast_node.h"

#include <string>
#include <vector>

namespace nova {

struct MemoryScript;

std::string export_ast_snapshot_string(const ProgramNode* program);

std::string export_ast_snapshot_string_from_scripts(const std::vector<MemoryScript>& scripts);

std::string export_ast_snapshot_string_from_path(const std::string& path);

}
