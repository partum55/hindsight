# hindsight

[![CI](https://img.shields.io/badge/CI-pending-lightgrey)](#)
[![License](https://img.shields.io/badge/license-MIT-blue)](LICENSE)
[![Status](https://img.shields.io/badge/status-early--development-red)](#status)

Zero-config analyzer for Node.js projects: one command, one graph, findings you can prove.

## Status

**Early development. Nothing works yet.** The repository currently contains documentation, a build
skeleton and a CLI stub. No analysis is implemented.

## The problem

A Node.js project is a graph — packages depend on files, files import files, functions call
functions — but nothing shows you that graph. Existing tools (knip, madge, dependency-cruiser)
read source text only. Source text does not tell you whether a module was ever loaded, whether an
exported function was ever called, or which dynamic import resolved to what. Anything that decides
at runtime — dynamic `import()`, decorators, callbacks registered through a framework, plugin
registries — is a guess for a static analyzer, and a guess that is wrong in both directions: dead
code reported as live, live code reported as dead.

## What hindsight does

You run one command. hindsight parses the whole codebase in parallel from a C++ core using
tree-sitter, and at the same time runs the project under the Node.js inspector protocol to collect
evidence about what actually executed: which functions ran, which modules were loaded, which
exceptions were thrown and where. The two sources are merged into a single graph over packages,
files and individual functions, where every node and edge carries its provenance. Every finding is
therefore labelled either **proven by execution** or **static hypothesis only** — you always know
whether hindsight saw it happen or merely inferred it.

Graph algorithms over the merged graph produce the findings: import cycles, code unreachable from
any entry point, structural clones of functions, and dependency weight attributed down to
individual functions. The output is a local web page with the graph and a ranked list of concrete
findings — which package costs megabytes for a single function that is never called, where code is
duplicated, which imports are cyclic, and in which function the application crashes.

## How it works

Three mechanisms, all verified against Node v24.15.0:

1. **V8 coverage, no instrumentation.** `NODE_V8_COVERAGE=./cov node app.js` makes Node write
   per-function execution counts as JSON on exit. Nothing is injected into the source, no build
   step, no runtime wrapper.
2. **Inspector over CDP.** Starting Node with `--inspect` exposes an HTTP endpoint; `GET /json/list`
   returns a `webSocketDebuggerUrl` for the Chrome DevTools Protocol. hindsight speaks CDP over
   that socket and consumes the Debugger, Profiler and Runtime domains.
3. **Attaching to a process that is already running.** Sending `SIGUSR1` to a Node process that was
   started *without* any inspect flag opens the inspector at runtime. This is a supported Node
   feature, not a hack. It is POSIX-only and not available on Windows.

## Planned usage

Single command, no configuration file:

```
npx hindsight            # detect, run the project's test command, analyze, open the report
```

Planned CLI surface:

```
hindsight scan [path]      analyze the project and write findings
hindsight serve [path]     serve the graph and findings as a local web page
hindsight export [path]    write the graph and findings as JSON

Options (planned):
  --run <command>          run this command instead of the project's test command
  --attach <pid>           attach to an already running Node process (SIGUSR1, POSIX only)
  --static-only            skip the runtime phase
  --port <n>               port for `serve`
  --json                   machine-readable output
```

By default hindsight runs only the project's own **test command**. Running any other command
requires the explicit `--run` flag; see
[docs/DESIGN-DECISIONS.md](docs/DESIGN-DECISIONS.md), ADR-3.

## Documentation

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — modules, data flow, provenance.
- [docs/DESIGN-DECISIONS.md](docs/DESIGN-DECISIONS.md) — ADR-style records.
- [docs/ROADMAP.md](docs/ROADMAP.md) — milestones.
- [CONTRIBUTING.md](CONTRIBUTING.md) — build and development.

## Toolchain

The project targets **C++23** and requires **GCC 14 or newer**. Verified toolchain: **GCC 14.2.0**
(`g++-14`) on Ubuntu 24.04.5 with **CMake 3.28.3**. Configuration fails with an explanatory message
on GCC 13 or older, and the CMake presets select `g++-14` explicitly.

On Ubuntu 24.04 the default `g++` is 13, which is not sufficient:

```
sudo apt install g++-14
```

Verified by compiling — and running — each feature with `g++-14 -std=c++23`:

- Available: `std::expected`, `std::print`, `std::generator`, deducing `this`, `std::ranges::to`,
  `if consteval`, multidimensional `operator[]`, `<stacktrace>`, `<spanstream>`.
- Not available until GCC 15, and therefore unused: `<flat_map>`, `<flat_set>`, `<mdspan>`.

Requiring GCC 14 rather than staying on the distribution default is a deliberate trade: see
[docs/DESIGN-DECISIONS.md](docs/DESIGN-DECISIONS.md), ADR-5.

## Build

Requires CMake >= 3.28 and GCC 14 or newer (developed with g++-14 14.2.0).

```
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

## License

MIT. See [LICENSE](LICENSE).
