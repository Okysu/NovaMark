#include "nova/packer/packer.h"
#include "nova/ast/ast_snapshot.h"
#include "nova/lexer/lexer.h"
#include "nova/parser/parser.h"
#include "nova/semantic/semantic_analyzer.h"
#include "nova/core/game_metadata.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <stdexcept>

namespace fs = std::filesystem;

namespace nova {

namespace {

int natural_compare(const std::string& a, const std::string& b) {
    size_t i = 0;
    size_t j = 0;

    while (i < a.size() && j < b.size()) {
        if (std::isdigit(static_cast<unsigned char>(a[i])) && std::isdigit(static_cast<unsigned char>(b[j]))) {
            size_t startI = i;
            size_t startJ = j;
            while (i < a.size() && std::isdigit(static_cast<unsigned char>(a[i]))) ++i;
            while (j < b.size() && std::isdigit(static_cast<unsigned char>(b[j]))) ++j;

            int numA = std::stoi(a.substr(startI, i - startI));
            int numB = std::stoi(b.substr(startJ, j - startJ));
            if (numA != numB) {
                return numA - numB;
            }
            continue;
        }

        if (a[i] != b[j]) {
            return static_cast<unsigned char>(a[i]) - static_cast<unsigned char>(b[j]);
        }
        ++i;
        ++j;
    }

    if (i == a.size() && j == b.size()) return 0;
    return i == a.size() ? -1 : 1;
}

GameMetadata load_project_metadata(const std::string& dir) {
    fs::path projectFile = fs::path(dir) / "project.yaml";
    if (!fs::exists(projectFile)) {
        projectFile = fs::path(dir) / "project.yml";
    }

    if (!fs::exists(projectFile)) {
        return GameMetadata();
    }

    std::ifstream file(projectFile);
    if (!file) {
        return GameMetadata();
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return GameMetadata::from_project_file(content);
}

bool collect_nvm_files_sorted(const std::string& path, std::vector<std::string>& files) {
    if (!fs::exists(path)) {
        return false;
    }

    if (fs::is_regular_file(path) && fs::path(path).extension() == ".nvm") {
        files.push_back(path);
        return true;
    }

    if (!fs::is_directory(path)) {
        return false;
    }

    std::vector<std::string> discovered;
    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.path().extension() == ".nvm") {
            discovered.push_back(entry.path().string());
        }
    }

    std::sort(discovered.begin(), discovered.end(), [](const std::string& a, const std::string& b) {
        return natural_compare(fs::path(a).filename().string(), fs::path(b).filename().string()) < 0;
    });
    files.insert(files.end(), discovered.begin(), discovered.end());
    return !discovered.empty();
}

std::string read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

Result<std::unique_ptr<ProgramNode>> build_combined_program(const std::vector<MemoryScript>& scripts) {
    if (scripts.empty()) {
        return Err<std::unique_ptr<ProgramNode>>(ErrorKind::InvalidSyntax, "No scripts to parse");
    }

    SourceLocation packedLoc("<packed>", 1, 1);
    auto combinedProgram = std::make_unique<ProgramNode>(packedLoc);

    for (const auto& script : scripts) {
        const std::string scriptPath = script.path.empty() ? "<memory>" : script.path;
        Lexer lexer(script.content, scriptPath);
        auto tokensResult = lexer.tokenize();
        if (tokensResult.is_err()) {
            const auto& err = tokensResult.error();
            return Err<std::unique_ptr<ProgramNode>>(err.kind,
                "Lexer error in " + scriptPath + ": " + err.message,
                err.location);
        }

        Parser parser(tokensResult.unwrap());
        auto astResult = parser.parse();
        if (astResult.is_err()) {
            const auto& err = astResult.error();
            return Err<std::unique_ptr<ProgramNode>>(err.kind,
                "Parser error in " + scriptPath + ": " + err.message,
                err.location);
        }

        auto astPtr = std::move(astResult).unwrap();
        auto program = dynamic_cast<ProgramNode*>(astPtr.get());
        if (!program) {
            return Err<std::unique_ptr<ProgramNode>>(ErrorKind::InvalidSyntax,
                "Parsed root is not ProgramNode", SourceLocation(scriptPath, 1, 1));
        }

        auto& stmts = program->statements();
        while (!stmts.empty()) {
            combinedProgram->add_statement(std::move(stmts.front()));
            stmts.erase(stmts.begin());
        }
    }

    return Ok(std::move(combinedProgram));
}

}

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

void Packer::setMetadata(const GameMetadata& metadata) {
    m_metadata = metadata;
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
    
    std::vector<MemoryScript> scripts;
    for (const auto& script : m_scripts) {
        std::string source = read_file(script);
        if (source.empty()) {
            m_error = "Failed to read script: " + script;
            result.error = m_error;
            return result;
        }
        scripts.push_back({script, std::move(source)});
    }

    auto combinedProgramResult = build_combined_program(scripts);
    if (combinedProgramResult.is_err()) {
        m_error = combinedProgramResult.error().message;
        result.error = m_error;
        return result;
    }
    auto combinedProgram = std::move(combinedProgramResult).unwrap();
    
    AstSerializer serializer;
    auto bytecode = serializer.serialize(combinedProgram.get());
    
    size_t bytecodeSize = bytecode.size();
    
    AssetBundler bundler;
    if (!m_assetDir.empty() && fs::exists(m_assetDir)) {
        bundler.setRootDirectory(m_assetDir);
        bundler.addDirectory(m_assetDir);
    }
    bundler.addFromReferences(serializer.getAssetReferences(), m_assetDir);
    
    NvmpWriter writer;
    writer.setBytecode(std::move(bytecode));
    writer.setAssets(bundler);
    writer.setMetadata(m_metadata);
    
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
    return collect_nvm_files_sorted(path, m_scripts);
}

std::string Packer::readFile(const std::string& path) const {
    return read_file(path);
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

Result<std::unique_ptr<ProgramNode>> buildCombinedProgramFromScripts(
    const std::vector<MemoryScript>& scripts
) {
    return build_combined_program(scripts);
}

Result<std::unique_ptr<ProgramNode>> buildCombinedProgramFromPath(
    const std::string& path
) {
    std::vector<MemoryScript> scripts;
    fs::path input(path);
    if (fs::is_regular_file(input) && input.extension() == ".nvm") {
        std::string source = read_file(path);
        if (source.empty()) {
            return Err<std::unique_ptr<ProgramNode>>(ErrorKind::IOError, "Failed to read script: " + path);
        }
        scripts.push_back({path, std::move(source)});
        return build_combined_program(scripts);
    }

    GameMetadata meta = load_project_metadata(path);
    std::vector<std::string> files;
    if (meta.valid) {
        fs::path scriptsPath = input / meta.scripts_path;
        if (!collect_nvm_files_sorted(scriptsPath.string(), files)) {
            return Err<std::unique_ptr<ProgramNode>>(ErrorKind::IOError, "No .nvm files found in " + scriptsPath.string());
        }
    } else {
        if (!collect_nvm_files_sorted(path, files)) {
            return Err<std::unique_ptr<ProgramNode>>(ErrorKind::IOError, "No .nvm files found in " + path);
        }
    }

    for (const auto& file : files) {
        std::string source = read_file(file);
        if (source.empty()) {
            return Err<std::unique_ptr<ProgramNode>>(ErrorKind::IOError, "Failed to read script: " + file);
        }
        scripts.push_back({file, std::move(source)});
    }

    return build_combined_program(scripts);
}

std::string export_ast_snapshot_string_from_scripts(const std::vector<MemoryScript>& scripts) {
    auto result = buildCombinedProgramFromScripts(scripts);
    if (result.is_err()) {
        throw std::runtime_error(result.error().message);
    }

    auto program = std::move(result).unwrap();
    return export_ast_snapshot_string(program.get());
}

std::string export_ast_snapshot_string_from_path(const std::string& path) {
    auto result = buildCombinedProgramFromPath(path);
    if (result.is_err()) {
        throw std::runtime_error(result.error().message);
    }

    auto program = std::move(result).unwrap();
    return export_ast_snapshot_string(program.get());
}

} // namespace nova
