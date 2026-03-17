#include "nova/packer/asset_bundler.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <functional>

namespace fs = std::filesystem;

namespace nova {

AssetBundler::AssetBundler() {}

void AssetBundler::addDirectory(const std::string& path) {
    if (!fs::exists(path) || !fs::is_directory(path)) {
        return;
    }
    
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (entry.is_regular_file()) {
            addFile(entry.path().string());
        }
    }
}

void AssetBundler::addFile(const std::string& path) {
    std::string normalized = normalizePath(path);
    
    if (m_addedPaths.count(normalized) > 0) {
        return;
    }
    
    std::vector<uint8_t> data;
    if (readFile(path, data)) {
        m_assets[normalized] = std::move(data);
        m_addedPaths.insert(normalized);
    }
}

void AssetBundler::addFromReferences(const std::vector<std::string>& refs, const std::string& basePath) {
    for (const auto& ref : refs) {
        std::string fullPath = basePath + "/" + ref;
        addFile(fullPath);
    }
}

std::vector<NvmpIndexEntry> AssetBundler::buildIndex(uint64_t dataOffset) const {
    std::vector<NvmpIndexEntry> index;
    index.reserve(m_assets.size());
    
    uint64_t currentOffset = dataOffset;
    
    for (const auto& [path, data] : m_assets) {
        NvmpIndexEntry entry;
        entry.nameHash = hashPath(path);
        entry.offset = currentOffset;
        entry.length = static_cast<uint32_t>(data.size());
        entry.originalLength = static_cast<uint32_t>(data.size());
        entry.type = detectAssetType(path);
        std::memset(entry.reserved, 0, sizeof(entry.reserved));
        
        index.push_back(entry);
        currentOffset += data.size();
    }
    
    return index;
}

std::vector<uint8_t> AssetBundler::buildDataSection() const {
    std::vector<uint8_t> data;
    
    for (const auto& [path, assetData] : m_assets) {
        data.insert(data.end(), assetData.begin(), assetData.end());
    }
    
    return data;
}

AssetType AssetBundler::detectAssetType(const std::string& path) const {
    std::string ext = path;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext.find(".png") != std::string::npos ||
        ext.find(".jpg") != std::string::npos ||
        ext.find(".jpeg") != std::string::npos ||
        ext.find(".webp") != std::string::npos ||
        ext.find(".gif") != std::string::npos) {
        return AssetType::Image;
    }
    
    if (ext.find(".ogg") != std::string::npos ||
        ext.find(".mp3") != std::string::npos ||
        ext.find(".wav") != std::string::npos ||
        ext.find(".flac") != std::string::npos) {
        return AssetType::Audio;
    }
    
    if (ext.find(".ttf") != std::string::npos ||
        ext.find(".otf") != std::string::npos ||
        ext.find(".woff") != std::string::npos) {
        return AssetType::Font;
    }
    
    if (ext.find(".mp4") != std::string::npos ||
        ext.find(".webm") != std::string::npos ||
        ext.find(".avi") != std::string::npos) {
        return AssetType::Video;
    }
    
    return AssetType::Other;
}

std::string AssetBundler::normalizePath(const std::string& path) const {
    std::string result = path;
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

bool AssetBundler::readFile(const std::string& path, std::vector<uint8_t>& out) const {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    out.resize(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(out.data()), size);
    
    return true;
}

uint64_t AssetBundler::hashPath(const std::string& path) const {
    std::string normalized = normalizePath(path);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    uint64_t hash = 5381;
    for (char c : normalized) {
        hash = ((hash << 5) + hash) + static_cast<uint64_t>(c);
    }
    return hash;
}

} // namespace nova