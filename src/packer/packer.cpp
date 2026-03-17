#include "nova/packer/packer.h"
#include "nova/lexer/lexer.h"
#include "nova/parser/parser.h"
#include "nova/semantic/semantic_analyzer.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace nova {

Packer::Packer() {}

void Packer::addScript(const std::string& path) {
    m_scripts.push_back(path);
}

void Packer::addScriptDirectory(const std::string& path) {
    collectScripts(path);
}

void Packer::setAssetDirectory(const std::string& path) {
    m_assetDir = path;
}

void Packer::setOutputPath(const std::string& path) {
    m_outputPath = path;
}

PackResult Packer::pack() {
    PackResult result;
    result.success = false;
    result.assetCount = 0;
    result.bytecodeSize = 0;
    result.totalSize = 0;
    
    if (m_scripts.empty()) {
        m_error = "No scripts to pack";
        result.error = m_error;
        return result;
    }
    
    if (m_outputPath.empty()) {
        m_error = "Output path not set";
        result.error = m_error;
        return result;
    }
    
    SourceLocation packedLoc("<packed>", 1, 1);
    
    auto combinedProgram = std::make_unique<ProgramNode>(packedLoc);
    std::vector<std::string> allAssetRefs;
    
    for (const auto& script : m_scripts) {
        std::string source = readFile(script);
        if (source.empty()) {
            m_error = "Failed to read script: " + script;
            result.error = m_error;
            return result;
        }
        
        Lexer lexer(source, script);
        auto tokensResult = lexer.tokenize();
        if (tokensResult.is_err()) {
            m_error = "Lexer error in " + script + ": " + tokensResult.error().message;
            result.error = m_error;
            return result;
        }
        
        Parser parser(tokensResult.unwrap());
        auto astResult = parser.parse();
        if (astResult.is_err()) {
            m_error = "Parser error in " + script + ": " + astResult.error().message;
            result.error = m_error;
            return result;
        }
        
        auto astPtr = std::move(astResult).unwrap();
        auto program = dynamic_cast<ProgramNode*>(astPtr.get());
        if (program) {
            auto& stmts = program->statements();
            while (!stmts.empty()) {
                combinedProgram->add_statement(std::move(stmts.front()));
                stmts.erase(stmts.begin());
            }
        }
    }
    
    AstSerializer serializer;
    auto bytecode = serializer.serialize(combinedProgram.get());
    
    size_t bytecodeSize = bytecode.size();
    
    AssetBundler bundler;
    if (!m_assetDir.empty() && fs::exists(m_assetDir)) {
        bundler.addDirectory(m_assetDir);
    }
    bundler.addFromReferences(serializer.getAssetReferences(), m_assetDir);
    
    NvmpWriter writer;
    writer.setBytecode(std::move(bytecode));
    writer.setAssets(bundler);
    
    if (!writer.writeToFile(m_outputPath)) {
        m_error = writer.error();
        result.error = m_error;
        return result;
    }
    
    auto buffer = writer.writeToBuffer();
    
    result.success = true;
    result.outputPath = m_outputPath;
    result.assetCount = bundler.count();
    result.bytecodeSize = bytecodeSize;
    result.totalSize = buffer.size();
    
    return result;
}

bool Packer::collectScripts(const std::string& path) {
    if (!fs::exists(path)) {
        return false;
    }
    
    if (fs::is_directory(path)) {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.path().extension() == ".nvm") {
                m_scripts.push_back(entry.path().string());
            }
        }
    } else if (fs::is_regular_file(path)) {
        m_scripts.push_back(path);
    }
    
    return true;
}

std::string Packer::readFile(const std::string& path) const {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return content;
}

PackResult packProject(
    const std::string& scriptDir,
    const std::string& assetDir,
    const std::string& outputPath
) {
    Packer packer;
    packer.addScriptDirectory(scriptDir);
    packer.setAssetDirectory(assetDir);
    packer.setOutputPath(outputPath);
    return packer.pack();
}

} // namespace nova