#pragma once

#include "nova/packer/nvmp_format.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace nova {

/// @brief 资源打包器，负责收集和打包游戏资源
class AssetBundler {
public:
    AssetBundler();
    
    /// @brief 添加资源目录
    void addDirectory(const std::string& path);

    /// @brief 设置资源根目录，用于生成相对资源键
    void setRootDirectory(const std::string& path);
    
    /// @brief 添加单个资源文件
    void addFile(const std::string& path);
    
    /// @brief 从脚本引用中添加资源
    void addFromReferences(const std::vector<std::string>& refs, const std::string& basePath);
    
    /// @brief 获取所有资源条目
    const std::unordered_map<std::string, std::vector<uint8_t>>& assets() const { return m_assets; }
    
    /// @brief 获取资源数量
    size_t count() const { return m_assets.size(); }
    
    /// @brief 清空所有资源
    void clear() { m_assets.clear(); }
    
    /// @brief 构建索引表
    std::vector<NvmpIndexEntry> buildIndex(uint64_t dataOffset) const;
    
    /// @brief 构建数据区
    std::vector<uint8_t> buildDataSection() const;

private:
    std::unordered_map<std::string, std::vector<uint8_t>> m_assets;
    std::unordered_set<std::string> m_addedPaths;
    std::string m_rootDir;

    std::vector<std::pair<std::string, const std::vector<uint8_t>*>> orderedAssets() const;
     
    AssetType detectAssetType(const std::string& path) const;
    std::string normalizePath(const std::string& path) const;
    std::string makeAssetKey(const std::string& path) const;
    bool readFile(const std::string& path, std::vector<uint8_t>& out) const;
    uint64_t hashPath(const std::string& path) const;
};

} // namespace nova
