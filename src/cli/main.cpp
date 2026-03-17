#include "nova/cli/cli.h"

int main(int argc, char* argv[]) {
    auto config = nova::cli::parse_args(argc, argv);
    
    if (config.show_version) {
        nova::cli::print_version();
        return 0;
    }
    
    switch (config.command) {
        case nova::cli::Command::Help:
            nova::cli::print_help(argc > 0 ? argv[0] : "nova-cli");
            return 0;
            
        case nova::cli::Command::Init:
            return nova::cli::do_init(config);
            
        case nova::cli::Command::Build:
            return nova::cli::do_build(config);
            
        case nova::cli::Command::Run:
            return nova::cli::do_run(config);
            
        case nova::cli::Command::Check:
            return nova::cli::do_check(config);
    }
    
    return 1;
}
