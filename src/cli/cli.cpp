#include "nova/cli/cli.h"
#include "nova/lexer/lexer.h"
#include "nova/parser/parser.h"
#include "nova/semantic/semantic_analyzer.h"
#include "nova/packer/packer.h"
#include "nova/packer/nvmp_writer.h"
#include "nova/vm/vm.h"
#include "nova/vm/serializer.h"
#include "nova/ast/ast_node.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <ctime>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace nova::cli {

static int build_single_file(const CliConfig& config, const std::string& file);
static int build_project(const CliConfig& config, const std::string& dir, const GameMetadata& meta);
static bool collect_nvm_files_sorted(const std::string& path, std::vector<std::string>& files);

static int natural_compare(const std::string& a, const std::string& b) {
    size_t i = 0;
    size_t j = 0;
    while (i < a.size() && j < b.size()) {
        const unsigned char ca = static_cast<unsigned char>(a[i]);
        const unsigned char cb = static_cast<unsigned char>(b[j]);
        if (std::isdigit(ca) && std::isdigit(cb)) {
            size_t i2 = i;
            size_t j2 = j;
            while (i2 < a.size() && std::isdigit(static_cast<unsigned char>(a[i2]))) ++i2;
            while (j2 < b.size() && std::isdigit(static_cast<unsigned char>(b[j2]))) ++j2;

            auto numA = a.substr(i, i2 - i);
            auto numB = b.substr(j, j2 - j);
            size_t trimA = numA.find_first_not_of('0');
            size_t trimB = numB.find_first_not_of('0');
            numA.erase(0, trimA == std::string::npos ? numA.size() - 1 : trimA);
            numB.erase(0, trimB == std::string::npos ? numB.size() - 1 : trimB);

            if (numA.size() != numB.size()) {
                return numA.size() < numB.size() ? -1 : 1;
            }
            if (numA != numB) {
                return numA < numB ? -1 : 1;
            }

            if ((i2 - i) != (j2 - j)) {
                return (i2 - i) < (j2 - j) ? -1 : 1;
            }

            i = i2;
            j = j2;
            continue;
        }

        const unsigned char lowerA = static_cast<unsigned char>(std::tolower(ca));
        const unsigned char lowerB = static_cast<unsigned char>(std::tolower(cb));
        if (lowerA != lowerB) {
            return lowerA < lowerB ? -1 : 1;
        }
        ++i;
        ++j;
    }

    if (i == a.size() && j == b.size()) return 0;
    return i == a.size() ? -1 : 1;
}

static void setup_console() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

static std::string save_game(NovaVM& vm, const std::string& sceneName) {
    fs::create_directories(".novamark/saves");
    auto gameState = vm.captureState();
    SaveData save;
    save.saveId = "save_" + std::to_string(std::time(nullptr));
    save.label = sceneName;
    save.timestamp = std::chrono::system_clock::now();
    save.state = gameState;
    
    std::string saveFile = ".novamark/saves/" + save.saveId + ".json";
    if (GameStateSerializer::saveToFile(saveFile, save)) {
        return saveFile;
    }
    return "";
}

static bool load_game(NovaVM& vm, const std::string& path,
    std::optional<std::string>& last_bg,
    std::optional<std::string>& last_bgm,
    std::vector<SpriteState>& last_sprites,
    std::optional<ChoiceState>& last_choice) {
    
    SaveData save;
    if (GameStateSerializer::loadFromFile(path, save)) {
        if (vm.loadSave(save)) {
            last_bg.reset();
            last_bgm.reset();
            last_sprites.clear();
            last_choice.reset();
            return true;
        }
    }
    return false;
}

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
              << "Runtime controls (in Text Mode):\n"
              << "  S = Save game, L = Load game, Q = Quit\n\n"
              << "Save files are stored in .novamark/saves/\n\n"
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

static bool collect_nvm_files_sorted(const std::string& path, std::vector<std::string>& files) {
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
    packer.setMetadata(meta);
    
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
    packer.setMetadata(meta);
    
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
    setup_console();
    
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
        std::cout << "Assets: " << reader.listAssets().size() << "\n\n";
    }
    
    AstDeserializer deserializer(reader.version());
    auto program = deserializer.deserialize(reader.bytecode());
    if (!program) {
        std::cerr << "Error: Failed to deserialize bytecode: " << deserializer.errorMessage() << "\n";
        return 1;
    }
    
    NovaVM vm;
    vm.load(program.get());
    
    std::cout << "=== NovaMark Text Mode ===\n";
    std::cout << "(S = Save, L = Load, Q = Quit)\n\n";

    std::optional<std::string> last_bg;
    std::optional<std::string> last_bgm;
    std::vector<SpriteState> last_sprites;
    std::optional<ChoiceState> last_choice;
    
    std::string input;
    while (true) {
        const auto& state = vm.state();

        if (state.bg != last_bg) {
            last_bg = state.bg;
            if (state.bg) {
                std::cout << "[BG: " << *state.bg << "]\n";
            }
        }

        if (state.bgm != last_bgm) {
            last_bgm = state.bgm;
            if (state.bgm) {
                std::cout << "[BGM: " << *state.bgm << "]\n";
            }
        }

        if (state.sprites != last_sprites) {
            last_sprites = state.sprites;
            for (const auto& sprite : state.sprites) {
                std::cout << "[Sprite: " << sprite.id;
                if (sprite.position) {
                    std::cout << " position=" << *sprite.position;
                }
                if (sprite.x || sprite.y) {
                    std::cout << " at (" << (sprite.x ? *sprite.x : "") << "," << (sprite.y ? *sprite.y : "") << ")";
                }
                std::cout << "]\n";
            }
        }
        
        if (state.dialogue) {
            if (!state.dialogue->speaker.empty()) {
                std::cout << "\n** " << state.dialogue->speaker << " **\n";
            }
            std::cout << state.dialogue->text << "\n\n";
        }
        
        if (state.choice && state.choice->isShow) {
            if (!last_choice.has_value() || *state.choice != *last_choice) {
                last_choice = state.choice;

            if (!state.choice->question.empty()) {
                std::cout << state.choice->question << "\n";
            }
            for (size_t i = 0; i < state.choice->options.size(); ++i) {
                const auto& opt = state.choice->options[i];
                std::cout << "  [" << i << "] " << opt.text;
                if (opt.disabled) {
                    std::cout << " (disabled)";
                }
                std::cout << "\n";
            }
            std::cout << "\n";
            }
        }
        
        if (state.ending) {
            std::cout << "\n=== Ending: " << *state.ending << " ===\n";
            
            auto playthroughFile = ".novamark/playthrough.json";
            fs::create_directories(".novamark");
            GameState ptState;
            ptState.triggeredEndings = vm.playthrough().endings();
            ptState.flags = vm.playthrough().flags();
            std::string ptJson = GameStateSerializer::serialize(ptState);
            std::ofstream ptOut(playthroughFile);
            ptOut << ptJson;
            ptOut.close();
            std::cout << "(Playthrough data saved)\n";
            break;
        }
        
        if (state.status == VMStatus::Ended) {
            std::cout << "\n=== Game Over ===\n";
            break;
        }
        
        if (state.status == VMStatus::WaitingChoice) {
            std::cout << "Choose (0-" << state.choice->options.size() - 1 << "): ";
            std::getline(std::cin, input);
            
            if (input == "S" || input == "s") {
                std::string saveFile = save_game(vm, state.currentScene);
                if (!saveFile.empty()) {
                    std::cout << "Game saved to: " << saveFile << "\n\n";
                } else {
                    std::cout << "Failed to save game.\n\n";
                }
                continue;
            }
            
            if (input == "L" || input == "l") {
                std::cout << "Enter save file path: ";
                std::getline(std::cin, input);
                if (load_game(vm, input, last_bg, last_bgm, last_sprites, last_choice)) {
                    std::cout << "Game loaded from: " << input << "\n\n";
                } else {
                    std::cout << "Failed to load save file.\n\n";
                }
                continue;
            }
            
            if (input == "Q" || input == "q") {
                std::cout << "Quit game.\n";
                break;
            }
            
            try {
                int choice = std::stoi(input);
                if (choice >= 0 && choice < static_cast<int>(state.choice->options.size())) {
                    vm.choose(state.choice->options[choice].id);
                    vm.advance();
                }
            } catch (...) {
                std::cout << "Invalid input. Enter a number, S to save, L to load, or Q to quit.\n\n";
            }
        } else {
            std::cout << "[Press Enter to continue]";
            std::getline(std::cin, input);
            
            if (input == "S" || input == "s") {
                std::string saveFile = save_game(vm, vm.currentScene());
                if (!saveFile.empty()) {
                    std::cout << "Game saved to: " << saveFile << "\n\n";
                } else {
                    std::cout << "Failed to save game.\n\n";
                }
                continue;
            }
            
            if (input == "L" || input == "l") {
                std::cout << "Enter save file path: ";
                std::getline(std::cin, input);
                if (load_game(vm, input, last_bg, last_bgm, last_sprites, last_choice)) {
                    std::cout << "Game loaded from: " << input << "\n\n";
                } else {
                    std::cout << "Failed to load save file.\n\n";
                }
                continue;
            }
            
            if (input == "Q" || input == "q") {
                std::cout << "Quit game.\n";
                break;
            }
            
            vm.advance();
        }
        
        std::cout << "\n";
    }
    
    return 0;
}

int do_check(const CliConfig& config) {
    std::string checkPath = config.source_path.empty() ? "." : config.source_path;
    std::vector<std::string> files;
    GameMetadata meta;
    bool isProjectMode = false;

    if (fs::path(checkPath).extension() == ".nvm") {
        files.push_back(checkPath);
    } else {
        meta = load_project_metadata(checkPath);
        std::string scriptsPath = checkPath;

        if (meta.valid) {
            isProjectMode = true;
            scriptsPath = (fs::path(checkPath) / meta.scripts_path).string();
            if (config.verbose) {
                std::cout << "Project: " << meta.name << "\n";
            }
        }

        collect_nvm_files_sorted(scriptsPath, files);
    }

    if (files.empty()) {
        std::cerr << "Error: No .nvm files found\n";
        return 1;
    }

    int errorCount = 0;

    if (isProjectMode) {
        SourceLocation packedLoc("<packed>", 1, 1);
        auto combinedProgram = std::make_unique<ProgramNode>(packedLoc);

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
            if (!program) {
                continue;
            }

            auto& stmts = program->statements();
            while (!stmts.empty()) {
                combinedProgram->add_statement(std::move(stmts.front()));
                stmts.erase(stmts.begin());
            }
        }

        if (errorCount == 0) {
            SemanticAnalyzer analyzer;
            auto semantic_result = analyzer.analyze(combinedProgram.get());

            if (!semantic_result.success) {
                for (const auto& diag : semantic_result.diagnostics.diagnostics()) {
                    if (diag.level == DiagnosticLevel::Error) {
                        std::cerr << diag.location.to_string() << ": error: " << diag.message << "\n";
                    }
                }
                errorCount++;
            } else if (config.verbose) {
                auto scenes = semantic_result.symbol_table.get_symbols_by_kind(SymbolKind::Scene);
                auto chars = semantic_result.symbol_table.get_symbols_by_kind(SymbolKind::Character);
                std::cout << "<project>: OK (scenes:" << scenes.size()
                          << " chars:" << chars.size() << ")\n";
            }
        }
    } else for (const auto& file : files) {
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
