# Contributing

The project is in early development and nothing works yet. The most useful contributions right now
are to the design documents in `docs/`.

## Requirements

- CMake >= 3.28
- **GCC 14 or newer.** Developed and verified with GCC 14.2.0 (`g++-14`). Ubuntu 24.04 ships GCC 13
  by default, which cannot build this project: `sudo apt install g++-14`. CMake fails configuration
  with that instruction if it finds an older GCC.
  `std::expected`, `std::print`, `std::generator`, deducing `this` and `std::ranges::to` are all
  available and may be used freely. `<flat_map>`, `<flat_set>` and `<mdspan>` need GCC 15 and are
  **not** available — do not use them. See [docs/DESIGN-DECISIONS.md](docs/DESIGN-DECISIONS.md),
  ADR-5.
- Node.js is required at runtime (not to build). Behaviour was verified against Node v24.15.0.

## Build and test

```
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Or without presets:

```
cmake -B build -S . -DCMAKE_CXX_COMPILER=g++-14
cmake --build build
ctest --test-dir build
```

The test framework is doctest, fetched at configure time via CMake `FetchContent`. The first
configure therefore needs network access.

## Layout

```
src/<module>/    one directory per module; see docs/ARCHITECTURE.md
src/main.cpp     CLI entry point
tests/           doctest tests
docs/            design documents
```

## Conventions

- Warnings are errors in practice: the build enables `-Wall -Wextra -Wpedantic`, and patches should
  not add warnings.
- Formatting is enforced by `.clang-format` (LLVM base, 4-space indent, 100 column limit). Run
  `clang-format -i` on the files you touch.
- Each module lives in namespace `hindsight::<module>`.
- Prefer `std::expected` over exceptions for expected failure paths.
- Keep third-party dependencies minimal. Adding one needs a reason in the pull request. tree-sitter
  and a WebSocket library are planned and not yet vendored.

## Design changes

Anything that changes module boundaries, the provenance model, or the safety rule about which
commands may be executed should come with an ADR entry in `docs/DESIGN-DECISIONS.md` using the
existing context/decision/consequences format.
