#include "nova/cli/cli.h"
#include "nova/lexer/lexer.h"
#include "nova/parser/parser.h"
#include "nova/semantic/semantic_analyzer.h"
#include "nova/packer/packer.h"
#include "nova/packer/nvmp_writer.h"
#include "nova/vm/vm.h"
#include "nova/ast/ast_node.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

namespace nova::cli {

static int build_single_file(const CliConfig& config, const std::string& file);
static int build_project(const CliConfig& config, const std::string& dir, const GameMetadata& meta);

void print_help(const char* program_name) {
    std::cout << "NovaMark Compiler v0.1.0\n\n"
              << "Usage: " << program_name << " <command> [options]\n\n"
              << "Commands:\n"
              << "  init <name>                  Create a new project\n"
              << "  build [path]                 Build project or single file\n"
              << "  run <package>                Run .nvmp package\n"
              << "  check [path]                 Check for syntax errors\n"
              << "  help                         Show this help message\n\n"
              << "Build modes:\n"
              << "  nova-cli build                    Build project (finds project.yaml)\n"
              << "  nova-cli build game.nvm           Build single file\n"
              << "  nova-cli build ./MyProject        Build specific project directory\n\n"
              << "Options:\n"
              << "  -o, --output <path>          Output file path\n"
              << "  -v, --verbose                Show verbose output\n"
              << "  --version                    Show version information\n";
}

void print_version() {
    std::cout << "NovaMark Compiler v0.1.0\n";
}

CliConfig parse_args(int argc, char* argv[]) {
    CliConfig config;
    
    if (argc < 2) {
        config.command = Command::Help;
        return config;
    }
    
    std::string cmd = argv[1];
    
    if (cmd == "init") {
        config.command = Command::Init;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-v" || arg == "--verbose") {
                config.verbose = true;
            } else if (arg[0] != '-') {
                config.project_name = arg;
            }
        }
    } else if (cmd == "build") {
        config.command = Command::Build;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
                config.output_path = argv[++i];
            } else if (arg == "-v" || arg == "--verbose") {
                config.verbose = true;
            } else if (arg[0] != '-') {
                config.source_path = arg;
            }
        }
    } else if (cmd == "run") {
        config.command = Command::Run;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-v" || arg == "--verbose") {
                config.verbose = true;
            } else if (arg[0] != '-') {
                config.source_path = arg;
            }
        }
    } else if (cmd == "check") {
        config.command = Command::Check;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-v" || arg == "--verbose") {
                config.verbose = true;
            } else if (arg[0] != '-') {
                config.source_path = arg;
            }
        }
    } else if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        config.command = Command::Help;
    } else if (cmd == "--version") {
        config.show_version = true;
    } else {
        std::cerr << "Unknown command: " << cmd << "\n";
        config.command = Command::Help;
    }
    
    return config;
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
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    return GameMetadata::from_project_file(content);
}

GameMetadata extract_front_matter(const std::string& file) {
    std::ifstream f(file);
    if (!f) {
        return GameMetadata();
    }
    
    std::string firstLine;
    std::getline(f, firstLine);
    
    if (firstLine != "---") {
        return GameMetadata();
    }
    
    std::string content;
    std::string line;
    while (std::getline(f, line)) {
        if (line == "---") break;
        content += line + "\n";
    }
    
    if (content.empty()) {
        return GameMetadata();
    }
    
    return GameMetadata::from_front_matter(content);
}

int do_init(const CliConfig& config) {
    if (config.project_name.empty()) {
        std::cerr << "Error: Project name required\n";
        std::cerr << "Usage: nova-cli init <project_name>\n";
        return 1;
    }
    
    fs::path projectDir(config.project_name);
    
    if (fs::exists(projectDir)) {
        std::cerr << "Error: Directory already exists: " << config.project_name << "\n";
        return 1;
    }
    
    std::cout << "Creating project: " << config.project_name << "\n";
    
    fs::create_directory(projectDir);
    fs::create_directory(projectDir / "scripts");
    fs::create_directory(projectDir / "assets");
    fs::create_directory(projectDir / "assets" / "bg");
    fs::create_directory(projectDir / "assets" / "sprites");
    fs::create_directory(projectDir / "assets" / "audio");
    
    GameMetadata meta;
    meta.name = config.project_name;
    meta.title = config.project_name;
    meta.version = "1.0.0";
    meta.entry_scene = "scene_start";
    
    std::ofstream projectFile(projectDir / "project.yaml");
    projectFile << meta.to_yaml();
    projectFile.close();
    
    std::ofstream initScript(projectDir / "scripts" / "00_init.nvm");
    initScript << "@var hp = 100\n"
               << "@var gold = 0\n\n"
               << "@char Narrator\n"
               << "  color: #FFFFFF\n"
               << "@end\n";
    initScript.close();
    
    std::ofstream mainScript(projectDir / "scripts" / "01_main.nvm");
    mainScript << "#scene_start \"Start\"\n\n"
               << "> Welcome to " << config.project_name << "!\n\n"
               << "Narrator: This is your first scene.\n\n"
               << "-> scene_start\n";
    mainScript.close();
    
    std::cout << "Project created: " << config.project_name << "/\n"
              << "  project.yaml\n"
              << "  scripts/\n"
              << "    00_init.nvm\n"
              << "    01_main.nvm\n"
              << "  assets/\n"
              << "    bg/\n"
              << "    sprites/\n"
              << "    audio/\n";
    
    return 0;
}

int do_build(const CliConfig& config) {
    std::string buildPath = config.source_path.empty() ? "." : config.source_path;
    
    bool isSingleFile = fs::path(buildPath).extension() == ".nvm";
    
    if (isSingleFile) {
        return build_single_file(config, buildPath);
    }
    
    GameMetadata meta = load_project_metadata(buildPath);
    
    if (!meta.valid) {
        std::cerr << "Error: No project.yaml found in " << buildPath << "\n";
        std::cerr << "Use 'nova-cli init <name>' to create a project,\n";
        std::cerr << "or specify a .nvm file for single-file build.\n";
        return 1;
    }
    
    return build_project(config, buildPath, meta);
}

int build_single_file(const CliConfig& config, const std::string& file) {
    if (!fs::exists(file)) {
        std::cerr << "Error: File not found: " << file << "\n";
        return 1;
    }
    
    GameMetadata meta = extract_front_matter(file);
    if (!meta.valid) {
        meta.name = fs::path(file).stem().string();
        meta.title = meta.name;
    }
    
    std::string outputPath = config.output_path;
    if (outputPath.empty()) {
        outputPath = fs::path(file).stem().string() + ".nvmp";
    }
    
    if (config.verbose) {
        std::cout << "Building single file: " << file << "\n";
        if (meta.valid) {
            std::cout << "Title: " << meta.title << "\n";
            std::cout << "Author: " << meta.author << "\n";
        }
        std::cout << "Output: " << outputPath << "\n";
    }
    
    Packer packer;
    packer.addScript(file);
    packer.setOutputPath(outputPath);
    
    auto result = packer.pack();
    
    if (!result.success) {
        std::cerr << "Build failed: " << result.error << "\n";
        return 1;
    }
    
    if (config.verbose) {
        std::cout << "Bytecode: " << result.bytecodeSize << " bytes\n";
        std::cout << "Total: " << result.totalSize << " bytes\n";
    }
    
    std::cout << "Build successful: " << outputPath << "\n";
    return 0;
}

int build_project(const CliConfig& config, const std::string& dir, const GameMetadata& meta) {
    fs::path scriptsPath = fs::path(dir) / meta.scripts_path;
    fs::path assetsPath = fs::path(dir) / meta.assets_path;
    
    std::string outputPath = config.output_path;
    if (outputPath.empty()) {
        outputPath = (fs::path(dir) / (meta.name + ".nvmp")).string();
    }
    
    if (config.verbose) {
        std::cout << "Project: " << meta.name << " v" << meta.version << "\n";
        if (!meta.author.empty()) {
            std::cout << "Author: " << meta.author << "\n";
        }
        std::cout << "Entry: " << meta.entry_scene << "\n";
        std::cout << "Scripts: " << scriptsPath << "\n";
        std::cout << "Assets: " << assetsPath << "\n";
        std::cout << "Output: " << outputPath << "\n";
    }
    
    if (!fs::exists(scriptsPath)) {
        std::cerr << "Error: Scripts directory not found: " << scriptsPath << "\n";
        return 1;
    }
    
    Packer packer;
    packer.addScriptDirectory(scriptsPath.string());
    
    if (fs::exists(assetsPath)) {
        packer.setAssetDirectory(assetsPath.string());
    }
    packer.setOutputPath(outputPath);
    
    auto result = packer.pack();
    
    if (!result.success) {
        std::cerr << "Build failed: " << result.error << "\n";
        return 1;
    }
    
    if (config.verbose) {
        std::cout << "Bytecode: " << result.bytecodeSize << " bytes\n";
        std::cout << "Assets: " << result.assetCount << "\n";
        std::cout << "Total: " << result.totalSize << " bytes\n";
    }
    
    std::cout << "Build successful: " << outputPath << "\n";
    return 0;
}

int do_run(const CliConfig& config) {
    if (config.source_path.empty()) {
        std::cerr << "Error: No package file specified\n";
        return 1;
    }
    
    if (!fs::exists(config.source_path)) {
        std::cerr << "Error: Package file not found: " << config.source_path << "\n";
        return 1;
    }
    
    NvmpReader reader;
    if (!reader.loadFromFile(config.source_path)) {
        std::cerr << "Error: Failed to load package: " << reader.error() << "\n";
        return 1;
    }
    
    if (config.verbose) {
        std::cout << "Package: " << config.source_path << "\n";
        std::cout << "Bytecode: " << reader.bytecode().size() << " bytes\n";
        std::cout << "Assets: " << reader.listAssets().size() << "\n";
    }
    
    std::cout << "Note: VM requires a renderer. Running in headless mode.\n";
    return 0;
}

int do_check(const CliConfig& config) {
    std::string checkPath = config.source_path.empty() ? "." : config.source_path;
    
    std::vector<std::string> files;
    
    if (fs::path(checkPath).extension() == ".nvm") {
        files.push_back(checkPath);
    } else {
        GameMetadata meta = load_project_metadata(checkPath);
        std::string scriptsPath;
        
        if (meta.valid) {
            scriptsPath = (fs::path(checkPath) / meta.scripts_path).string();
            if (config.verbose) {
                std::cout << "Project: " << meta.name << "\n";
            }
        } else {
            scriptsPath = checkPath;
        }
        
        if (fs::is_directory(scriptsPath)) {
            for (const auto& entry : fs::directory_iterator(scriptsPath)) {
                if (entry.path().extension() == ".nvm") {
                    files.push_back(entry.path().string());
                }
            }
        }
    }
    
    if (files.empty()) {
        std::cerr << "Error: No .nvm files found\n";
        return 1;
    }
    
    int errorCount = 0;
    
    for (const auto& file : files) {
        std::ifstream f(file);
        if (!f) {
            std::cerr << "Error: Cannot open: " << file << "\n";
            errorCount++;
            continue;
        }
        
        std::string source((std::istreambuf_iterator<char>(f)),
                           std::istreambuf_iterator<char>());
        
        Lexer lexer(source, file);
        auto tokens_result = lexer.tokenize();
        
        if (tokens_result.is_err()) {
            std::cerr << tokens_result.error().to_string() << "\n";
            errorCount++;
            continue;
        }
        
        Parser parser(std::move(tokens_result).unwrap());
        auto ast_result = parser.parse();
        
        if (ast_result.is_err()) {
            std::cerr << ast_result.error().to_string() << "\n";
            errorCount++;
            continue;
        }
        
        auto& ast = ast_result.unwrap();
        auto program = dynamic_cast<ProgramNode*>(ast.get());
        if (!program) continue;
        
        SemanticAnalyzer analyzer;
        auto semantic_result = analyzer.analyze(program);
        
        if (!semantic_result.success) {
            for (const auto& diag : semantic_result.diagnostics.diagnostics()) {
                if (diag.level == DiagnosticLevel::Error) {
                    std::cerr << file << ": " << diag.location.to_string() 
                              << ": error: " << diag.message << "\n";
                }
            }
            errorCount++;
            continue;
        }
        
        if (config.verbose) {
            auto scenes = semantic_result.symbol_table.get_symbols_by_kind(SymbolKind::Scene);
            auto chars = semantic_result.symbol_table.get_symbols_by_kind(SymbolKind::Character);
            std::cout << file << ": OK (scenes:" << scenes.size() 
                      << " chars:" << chars.size() << ")\n";
        }
    }
    
    if (errorCount == 0) {
        std::cout << "Check passed: " << files.size() << " file(s)\n";
        return 0;
    }
    
    std::cout << "Check failed: " << errorCount << " error(s)\n";
    return 1;
}

} // namespace nova::cli