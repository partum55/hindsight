#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "cli/cli.hpp"

using hindsight::cli::Subcommand;
using hindsight::cli::parse_subcommand;

TEST_CASE("subcommands are recognised") {
    CHECK(parse_subcommand("scan") == Subcommand::Scan);
    CHECK(parse_subcommand("serve") == Subcommand::Serve);
    CHECK(parse_subcommand("export") == Subcommand::Export);
    CHECK(parse_subcommand("help") == Subcommand::Help);
    CHECK(parse_subcommand("--help") == Subcommand::Help);
}

TEST_CASE("unknown input is rejected") {
    CHECK(parse_subcommand("") == Subcommand::Unknown);
    CHECK(parse_subcommand("scanx") == Subcommand::Unknown);
    CHECK(parse_subcommand("Scan") == Subcommand::Unknown);
}
