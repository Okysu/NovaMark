#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace nova {

// NVMP 文件格式版本
//
// NOTE:
// The AST bytecode layout has evolved over time (e.g. ChoiceOption body and
// interpolated-text payloads). Until AST bytecode gets its own explicit
// embedded version, the container header version acts as the compatibility
// boundary for packaged .nvmp artifacts.
constexpr uint32_t NVMP_VERSION = 2;
constexpr const char* NVMP_METADATA_PATH = "__nova/metadata.yaml";

// 魔法数字 "NOVA"
constexpr char NVMP_MAGIC[4] = {'N', 'O', 'V', 'A'};

// 资源类型枚举
enum class AssetType : uint8_t {
    Image = 0,      // 图片 (png, jpg, webp)
    Audio = 1,      // 音频 (ogg, mp3, wav)
    Font = 2,       // 字体
    Video = 3,      // 视频
    Other = 255     // 其他
};

// 文件头结构 (32 bytes)
struct NvmpHeader {
    char magic[4];              // "NOVA"
    uint32_t version;           // 格式版本
    uint32_t flags;             // 标志位（预留）
    uint32_t indexCount;        // 索引条目数
    uint64_t indexOffset;       // 索引表偏移
    uint64_t astOffset;         // AST 字节码偏移
    uint64_t dataOffset;        // 二进制数据偏移
};

// 索引条目 (32 bytes per entry)
struct NvmpIndexEntry {
    uint64_t nameHash;          // 资源名哈希
    uint64_t offset;            // 在数据区的偏移
    uint32_t length;            // 数据长度
    uint32_t originalLength;    // 原始长度（压缩前）
    AssetType type;             // 资源类型
    uint8_t reserved[7];        // 预留对齐
};

// 字节码操作码
enum class OpCode : uint8_t {
    // 节点类型标记
    NodeProgram = 0,
    NodeDialogue = 1,
    NodeNarrator = 2,
    NodeSceneDef = 3,
    NodeJump = 4,
    NodeChoice = 5,
    NodeVarDef = 6,
    NodeBranch = 7,
    NodeCharDef = 8,
    NodeItemDef = 9,
    NodeBgCommand = 10,
    NodeSpriteCommand = 11,
    NodeBgmCommand = 12,
    NodeSfxCommand = 13,
    NodeSetCommand = 14,
    NodeGiveCommand = 15,
    NodeTakeCommand = 16,
    NodeSave = 17,
    NodeCall = 18,
    NodeReturn = 19,
    NodeEnding = 20,
    NodeFlag = 21,
    NodeLabel = 22,
    NodeCheckCommand = 23,
    NodeThemeDef = 26,
    NodeFrontMatter = 27,
    NodeLiteral = 28,
    NodeIdentifier = 29,
    NodeBinaryExpr = 30,
    NodeUnaryExpr = 31,
    NodeCallExpr = 32,
    NodeDiceExpr = 33,
    NodeChoiceOption = 34,
    
    // 字面量类型
    LiteralNull = 100,
    LiteralString = 101,
    LiteralNumber = 102,
    LiteralBool = 103,
    
    // 控制标记
    EndNode = 200,
    EndSection = 201
};

// 字节码写入器辅助类
class BytecodeWriter {
public:
    void writeByte(uint8_t value);
    void writeU16(uint16_t value);
    void writeU32(uint32_t value);
    void writeU64(uint64_t value);
    void writeFloat(float value);
    void writeDouble(double value);
    void writeString(const std::string& str);
    void writeBytes(const std::vector<uint8_t>& data);
    
    const std::vector<uint8_t>& data() const { return m_buffer; }
    size_t size() const { return m_buffer.size(); }
    void clear() { m_buffer.clear(); }

private:
    std::vector<uint8_t> m_buffer;
};

// 字节码读取器辅助类
class BytecodeReader {
public:
    explicit BytecodeReader(const std::vector<uint8_t>& data);
    
    uint8_t readByte();
    uint16_t readU16();
    uint32_t readU32();
    uint64_t readU64();
    float readFloat();
    double readDouble();
    std::string readString();
    
    bool atEnd() const { return m_pos >= m_data.size(); }
    size_t position() const { return m_pos; }
    void seek(size_t pos) { m_pos = pos; }

private:
    const std::vector<uint8_t>& m_data;
    size_t m_pos = 0;
};

} // namespace nova
