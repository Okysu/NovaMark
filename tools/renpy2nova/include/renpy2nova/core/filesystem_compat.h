#pragma once

/// @brief 文件系统兼容层
///
/// 为 C++17 std::filesystem 提供命名空间别名，
/// 便于未来在不同平台上切换文件系统实现。
#include <filesystem>

namespace filesystem_compat = std::filesystem;
