#pragma once

#include "nova/core/game_metadata.h"
#include <string>
#include <vector>

namespace nova::cli {

enum class Command {
    Init,
    Build,
    Run,
    Check,
    Help,
};

struct CliConfig {
    Command command = Command::Help;
    std::string source_path;
    std::string output_path;
    std::string project_name;
    bool verbose = false;
    bool show_version = false;
};

CliConfig parse_args(int argc, char* argv[]);

void print_help(const char* program_name);

void print_version();

GameMetadata load_project_metadata(const std::string& dir);

GameMetadata extract_front_matter(const std::string& file);

int do_init(const CliConfig& config);

int do_build(const CliConfig& config);

int do_run(const CliConfig& config);

int do_check(const CliConfig& config);

} // namespace nova::cli
