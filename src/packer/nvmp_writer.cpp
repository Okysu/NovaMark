#include "nova/packer/nvmp_writer.h"
#include <fstream>
#include <cstring>
#include <algorithm>

namespace {

uint64_t hash_path_for_nvmp(const std::string& path) {
    std::string normalized = path;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    std::replace(normalized.begin(), normalized.end(), '\\', '/');

    uint64_t hash = 5381;
    for (char c : normalized) {
        hash = ((hash << 5) + hash) + static_cast<uint64_t>(c);
    }
    return hash;
}

}

namespace nova {

NvmpWriter::NvmpWriter() {}

void NvmpWriter::setBytecode(std::vector<uint8_t> bytecode) {
    m_bytecode = std::move(bytecode);
}

void NvmpWriter::setAssets(const AssetBundler& bundler) {
    m_index = bundler.buildIndex(0);
    m_dataSection = bundler.buildDataSection();
}

void NvmpWriter::setMetadata(const GameMetadata& metadata) {
    m_metadata = metadata;
}

bool NvmpWriter::writeToFile(const std::string& path) {
    auto buffer = writeToBuffer();
    if (buffer.empty() && !m_error.empty()) {
        return false;
    }
    
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        m_error = "Failed to open file: " + path;
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    return true;
}

std::vector<uint8_t> NvmpWriter::writeToBuffer() {
    std::vector<uint8_t> out;

    std::vector<NvmpIndexEntry> finalIndex = m_index;
    std::vector<uint8_t> finalDataSection = m_dataSection;

    if (m_metadata.valid) {
        std::string metadataYaml = m_metadata.to_yaml();
        std::vector<uint8_t> metadataBytes(metadataYaml.begin(), metadataYaml.end());

        NvmpIndexEntry metadataEntry{};
        metadataEntry.nameHash = hash_path_for_nvmp(NVMP_METADATA_PATH);
        metadataEntry.offset = finalDataSection.size();
        metadataEntry.length = static_cast<uint32_t>(metadataBytes.size());
        metadataEntry.originalLength = static_cast<uint32_t>(metadataBytes.size());
        metadataEntry.type = AssetType::Other;

        finalIndex.push_back(metadataEntry);
        finalDataSection.insert(finalDataSection.end(), metadataBytes.begin(), metadataBytes.end());
    }
     
    uint64_t headerSize = sizeof(NvmpHeader);
    uint64_t indexSize = finalIndex.size() * sizeof(NvmpIndexEntry);
    uint64_t astSize = m_bytecode.size();
    uint64_t dataSize = finalDataSection.size();
    
    NvmpHeader header;
    std::memcpy(header.magic, NVMP_MAGIC, 4);
    header.version = NVMP_VERSION;
    header.flags = 0;
    header.indexCount = static_cast<uint32_t>(finalIndex.size());
    header.indexOffset = headerSize;
    header.astOffset = headerSize + indexSize;
    header.dataOffset = header.astOffset + astSize;
    
    uint64_t totalSize = headerSize + indexSize + astSize + dataSize;
    out.reserve(totalSize);

    for (auto& entry : finalIndex) {
        entry.offset += header.astOffset + astSize;
    }
     
    writeHeader(out, header);
    writeIndex(out, finalIndex);
    writeBytecode(out);
    out.insert(out.end(), finalDataSection.begin(), finalDataSection.end());
     
    return out;
}

void NvmpWriter::writeHeader(std::vector<uint8_t>& out, const NvmpHeader& header) {
    out.insert(out.end(), 
               reinterpret_cast<const uint8_t*>(&header),
               reinterpret_cast<const uint8_t*>(&header) + sizeof(NvmpHeader));
}

void NvmpWriter::writeIndex(std::vector<uint8_t>& out, const std::vector<NvmpIndexEntry>& index) {
    for (const auto& entry : index) {
        out.insert(out.end(),
                   reinterpret_cast<const uint8_t*>(&entry),
                   reinterpret_cast<const uint8_t*>(&entry) + sizeof(NvmpIndexEntry));
    }
}

void NvmpWriter::writeBytecode(std::vector<uint8_t>& out) {
    out.insert(out.end(), m_bytecode.begin(), m_bytecode.end());
}

void NvmpWriter::writeData(std::vector<uint8_t>& out) {
    out.insert(out.end(), m_dataSection.begin(), m_dataSection.end());
}

// ============================================
// NvmpReader
// ============================================

NvmpReader::NvmpReader() {
    std::memset(&m_header, 0, sizeof(m_header));
}

bool NvmpReader::loadFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        m_error = "Failed to open file: " + path;
        return false;
    }
    
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    
    return loadFromBuffer(data);
}

bool NvmpReader::loadFromBuffer(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(NvmpHeader)) {
        m_error = "Invalid NVMP file: too small";
        return false;
    }
    
    if (!parseHeader(data.data(), data.size())) {
        return false;
    }
    
    if (!parseIndex(data.data(), data.size())) {
        return false;
    }
    
    if (!parseBytecode(data.data(), data.size())) {
        return false;
    }
    
    if (!parseDataSection(data.data(), data.size())) {
        return false;
    }
    
    return true;
}

bool NvmpReader::parseHeader(const uint8_t* data, size_t size) {
    std::memcpy(&m_header, data, sizeof(NvmpHeader));
    
    if (std::memcmp(m_header.magic, NVMP_MAGIC, 4) != 0) {
        m_error = "Invalid NVMP magic number";
        return false;
    }
    
    if (m_header.version > NVMP_VERSION) {
        m_error = "Unsupported NVMP version: package=" + std::to_string(m_header.version)
            + ", runtime=" + std::to_string(NVMP_VERSION)
            + ". Rebuild the package with the current nova-cli.";
        return false;
    }
    
    return true;
}

bool NvmpReader::parseIndex(const uint8_t* data, size_t size) {
    m_index.clear();
    m_indexLookup.clear();
    
    if (m_header.indexOffset + m_header.indexCount * sizeof(NvmpIndexEntry) > size) {
        m_error = "Invalid index offset or count";
        return false;
    }
    
    const uint8_t* indexPtr = data + m_header.indexOffset;
    for (uint32_t i = 0; i < m_header.indexCount; ++i) {
        NvmpIndexEntry entry;
        std::memcpy(&entry, indexPtr + i * sizeof(NvmpIndexEntry), sizeof(NvmpIndexEntry));
        m_index.push_back(entry);
        m_indexLookup[entry.nameHash] = i;
    }
    
    return true;
}

bool NvmpReader::parseBytecode(const uint8_t* data, size_t size) {
    m_bytecode.clear();
    
    uint64_t astEnd = m_header.dataOffset;
    if (m_header.astOffset > size || astEnd > size) {
        m_error = "Invalid AST offset";
        return false;
    }
    
    size_t astSize = static_cast<size_t>(astEnd - m_header.astOffset);
    m_bytecode.assign(data + m_header.astOffset, data + m_header.astOffset + astSize);
    
    return true;
}

bool NvmpReader::parseDataSection(const uint8_t* data, size_t size) {
    m_dataSection.clear();
    
    if (m_header.dataOffset > size) {
        m_error = "Invalid data offset";
        return false;
    }
    
    size_t dataSize = static_cast<size_t>(size - m_header.dataOffset);
    m_dataSection.assign(data + m_header.dataOffset, data + size);
    
    return true;
}

bool NvmpReader::getAsset(const std::string& name, std::vector<uint8_t>& out) const {
    uint64_t hash = hashPath(name);
    auto it = m_indexLookup.find(hash);
    if (it == m_indexLookup.end()) {
        return false;
    }
    
    const auto& entry = m_index[it->second];
    size_t offset = static_cast<size_t>(entry.offset - m_header.dataOffset);
    
    if (offset + entry.length > m_dataSection.size()) {
        return false;
    }
    
    out.assign(m_dataSection.begin() + offset,
               m_dataSection.begin() + offset + entry.length);
    return true;
}

std::vector<std::string> NvmpReader::listAssets() const {
    std::vector<std::string> names;
    names.reserve(m_index.size());
    return names;
}

GameMetadata NvmpReader::readMetadata() const {
    GameMetadata meta;

    std::vector<uint8_t> metadataBytes;
    if (!getAsset(NVMP_METADATA_PATH, metadataBytes) || metadataBytes.empty()) {
        return meta;
    }

    std::string yaml(metadataBytes.begin(), metadataBytes.end());
    return GameMetadata::from_project_file(yaml);
}

uint64_t NvmpReader::hashPath(const std::string& path) const {
    std::string normalized = path;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    
    uint64_t hash = 5381;
    for (char c : normalized) {
        hash = ((hash << 5) + hash) + static_cast<uint64_t>(c);
    }
    return hash;
}

} // namespace nova
