#pragma once

#include "nova/packer/nvmp_writer.h"
#include "nova/packer/ast_serializer.h"
#include "nova/packer/asset_bundler.h"
#include "nova/ast/ast_node.h"
#include "nova/core/game_metadata.h"
#include "nova/core/result.h"
#include <string>
#include <vector>

namespace nova {

/// @brief 打包结果
struct PackResult {
    bool success;
    std::string outputPath;
    std::string error;
    size_t assetCount;
    size_t bytecodeSize;
    size_t totalSize;
};

/// @brief NovaMark 打包器
class Packer {
public:
    Packer();
    
    /// @brief 设置输入脚本文件
    void addScript(const std::string& path);
    
    /// @brief 设置输入脚本目录
    void addScriptDirectory(const std::string& path);
    
    /// @brief 设置资源目录
    void setAssetDirectory(const std::string& path);
    
    /// @brief 设置输出路径
    void setOutputPath(const std::string& path);

    void setMetadata(const GameMetadata& metadata);
    
    /// @brief 执行打包
    PackResult pack();
    
    /// @brief 获取错误信息
    const std::string& error() const { return m_error; }

private:
    std::vector<std::string> m_scripts;
    std::string m_assetDir;
    std::string m_outputPath;
    GameMetadata m_metadata;
    std::string m_error;
    
    bool collectScripts(const std::string& path);
    std::string readFile(const std::string& path) const;
};

/// @brief 便捷打包函数
PackResult packProject(
    const std::string& scriptDir,
    const std::string& assetDir,
    const std::string& outputPath
);

} // namespace nova
