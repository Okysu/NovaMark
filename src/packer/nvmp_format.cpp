#include "nova/packer/nvmp_format.h"
#include <cstring>

namespace nova {

// ============================================
// BytecodeWriter
// ============================================

void BytecodeWriter::writeByte(uint8_t value) {
    m_buffer.push_back(value);
}

void BytecodeWriter::writeU16(uint16_t value) {
    m_buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    m_buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void BytecodeWriter::writeU32(uint32_t value) {
    m_buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    m_buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    m_buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    m_buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

void BytecodeWriter::writeU64(uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        m_buffer.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
    }
}

void BytecodeWriter::writeFloat(float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(float));
    writeU32(bits);
}

void BytecodeWriter::writeDouble(double value) {
    uint64_t bits;
    std::memcpy(&bits, &value, sizeof(double));
    writeU64(bits);
}

void BytecodeWriter::writeString(const std::string& str) {
    uint32_t len = static_cast<uint32_t>(str.size());
    writeU32(len);
    for (char c : str) {
        m_buffer.push_back(static_cast<uint8_t>(c));
    }
}

void BytecodeWriter::writeBytes(const std::vector<uint8_t>& data) {
    m_buffer.insert(m_buffer.end(), data.begin(), data.end());
}

// ============================================
// BytecodeReader
// ============================================

BytecodeReader::BytecodeReader(const std::vector<uint8_t>& data)
    : m_data(data), m_pos(0) {}

uint8_t BytecodeReader::readByte() {
    if (m_pos >= m_data.size()) return 0;
    return m_data[m_pos++];
}

uint16_t BytecodeReader::readU16() {
    uint16_t value = 0;
    value |= static_cast<uint16_t>(readByte());
    value |= static_cast<uint16_t>(readByte()) << 8;
    return value;
}

uint32_t BytecodeReader::readU32() {
    uint32_t value = 0;
    for (int i = 0; i < 4; ++i) {
        value |= static_cast<uint32_t>(readByte()) << (i * 8);
    }
    return value;
}

uint64_t BytecodeReader::readU64() {
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value |= static_cast<uint64_t>(readByte()) << (i * 8);
    }
    return value;
}

float BytecodeReader::readFloat() {
    uint32_t bits = readU32();
    float value;
    std::memcpy(&value, &bits, sizeof(float));
    return value;
}

double BytecodeReader::readDouble() {
    uint64_t bits = readU64();
    double value;
    std::memcpy(&value, &bits, sizeof(double));
    return value;
}

std::string BytecodeReader::readString() {
    uint32_t len = readU32();
    std::string result;
    result.reserve(len);
    for (uint32_t i = 0; i < len && !atEnd(); ++i) {
        result += static_cast<char>(readByte());
    }
    return result;
}

} // namespace nova