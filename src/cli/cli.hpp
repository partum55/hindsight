#pragma once

// cli --- argument parsing, subcommands, output formatting.
//
// See docs/ARCHITECTURE.md for how this module fits into the pipeline.

#include <string_view>

namespace hindsight::cli {

// TODO: not implemented yet. Only subcommand recognition exists; no option parsing, no output
// formatting.

enum class Subcommand {
    Scan,
    Serve,
    Export,
    Help,
    Unknown,
};

constexpr Subcommand parse_subcommand(std::string_view arg) noexcept {
    if (arg == "scan") {
        return Subcommand::Scan;
    }
    if (arg == "serve") {
        return Subcommand::Serve;
    }
    if (arg == "export") {
        return Subcommand::Export;
    }
    if (arg == "help" || arg == "-h" || arg == "--help") {
        return Subcommand::Help;
    }
    return Subcommand::Unknown;
}

} // namespace hindsight::cli
