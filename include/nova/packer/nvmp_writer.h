#pragma once

#include "nova/packer/nvmp_format.h"
#include "nova/packer/ast_serializer.h"
#include "nova/packer/asset_bundler.h"
#include <string>
#include <vector>

namespace nova {

/// @brief NVMP 文件写入器
class NvmpWriter {
public:
    NvmpWriter();
    
    /// @brief 设置 AST 字节码
    void setBytecode(std::vector<uint8_t> bytecode);
    
    /// @brief 设置资源打包器
    void setAssets(const AssetBundler& bundler);
    
    /// @brief 写入到文件
    bool writeToFile(const std::string& path);
    
    /// @brief 写入到内存缓冲区
    std::vector<uint8_t> writeToBuffer();
    
    /// @brief 获取错误信息
    const std::string& error() const { return m_error; }

private:
    std::vector<uint8_t> m_bytecode;
    std::vector<NvmpIndexEntry> m_index;
    std::vector<uint8_t> m_dataSection;
    std::string m_error;
    
    void writeHeader(std::vector<uint8_t>& out, const NvmpHeader& header);
    void writeIndex(std::vector<uint8_t>& out, const std::vector<NvmpIndexEntry>& index);
    void writeBytecode(std::vector<uint8_t>& out);
    void writeData(std::vector<uint8_t>& out);
};

/// @brief NVMP 文件读取器
class NvmpReader {
public:
    NvmpReader();
    
    /// @brief 从文件加载
    bool loadFromFile(const std::string& path);
    
    /// @brief 从内存加载
    bool loadFromBuffer(const std::vector<uint8_t>& data);
    
    /// @brief 获取 AST 字节码
    const std::vector<uint8_t>& bytecode() const { return m_bytecode; }
    
    /// @brief 获取资源数据
    bool getAsset(const std::string& name, std::vector<uint8_t>& out) const;
    
    /// @brief 获取所有资源名称
    std::vector<std::string> listAssets() const;
    
    /// @brief 获取错误信息
    const std::string& error() const { return m_error; }

private:
    NvmpHeader m_header;
    std::vector<NvmpIndexEntry> m_index;
    std::vector<uint8_t> m_bytecode;
    std::vector<uint8_t> m_dataSection;
    std::unordered_map<uint64_t, size_t> m_indexLookup;
    std::string m_error;
    
    bool parseHeader(const uint8_t* data, size_t size);
    bool parseIndex(const uint8_t* data, size_t size);
    bool parseBytecode(const uint8_t* data, size_t size);
    bool parseDataSection(const uint8_t* data, size_t size);
    uint64_t hashPath(const std::string& path) const;
};

} // namespace nova