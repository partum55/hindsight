// hindsight --- zero-config analyzer for Node.js projects.
//
// Early development: this is a CLI stub. No analysis is implemented.

#include "cli/cli.hpp"

#include <iostream>
#include <span>
#include <string_view>

namespace {

constexpr std::string_view kUsage = R"(hindsight - zero-config analyzer for Node.js projects

usage: hindsight <command> [path]

commands:
  scan      analyze the project and write findings
  serve     serve the graph and findings as a local web page
  export    write the graph and findings as JSON
  help      show this message

status: early development, nothing works yet.
)";

int run(std::span<const char* const> args) {
    if (args.empty()) {
        std::cout << kUsage;
        return 0;
    }

    using hindsight::cli::Subcommand;
    const std::string_view command = args.front();

    switch (hindsight::cli::parse_subcommand(command)) {
    case Subcommand::Scan:
    case Subcommand::Serve:
    case Subcommand::Export:
        std::cout << command << ": not implemented yet\n";
        return 0;
    case Subcommand::Help:
        std::cout << kUsage;
        return 0;
    case Subcommand::Unknown:
        break;
    }

    std::cerr << "hindsight: unknown command '" << command << "'\n\n" << kUsage;
    return 2;
}

} // namespace

int main(int argc, char** argv) {
    const std::span<const char* const> args{argv, static_cast<std::size_t>(argc)};
    return run(args.subspan(1));
}
