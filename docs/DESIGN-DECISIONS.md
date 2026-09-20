# Design decisions

ADR-style records. Each entry states the context, the decision and the consequences, including the
ones that hurt. These describe intent; none of them are implemented yet.

---

## ADR-1: Hybrid static + runtime analysis

**Context.** Existing Node.js analyzers (knip, madge, dependency-cruiser) read source text and
nothing else. Source text is not sufficient to describe a Node.js program: dynamic `import()` with a
computed specifier, decorators, callbacks registered through a framework and plugin registries all
decide at runtime what is connected to what. A purely static analyzer must either guess or stay
silent, and its guesses are wrong in both directions — live code reported as dead, dead code
reported as live. A purely dynamic analyzer has the opposite problem: it sees only what the observed
run touched and knows nothing about code that did not execute.

**Decision.** Build both and merge them. Parse the whole codebase statically with tree-sitter, and
in the same invocation run the project under Node's inspector and V8 coverage to collect evidence
about real execution. Merge the two into a single graph in which every node and edge carries a
provenance tag (`STATIC`, `RUNTIME`, `BOTH`). Never collapse the tag: every finding reported to the
user states whether it is proven by execution or is a static hypothesis.

**Consequences.**
- Findings become falsifiable. "Never called" backed by a run that loaded the module is a much
  stronger claim than "no call site found".
- The runtime phase requires actually running the project, which is slower than parsing and can
  fail, hang or have side effects. This forces the safety rule in ADR-3 and cleanup logic in
  `runner`.
- Runtime evidence is only as good as the run that produced it. Absence of `RUNTIME` is never
  reported as proof of absence; it is reported as unobserved.
- The merge step needs stable identity for functions across both sources, which is what makes
  `sourcemap` and range-to-function mapping load-bearing rather than optional.
- Two analysis backends is more code than one. Accepted: it is the entire point of the tool.

---

## ADR-2: Delegate hard module resolution to Node

**Context.** Node's module resolution is not a small algorithm. It covers CommonJS and ESM,
`exports` and `imports` maps with conditions, self-referencing, subpath patterns, `node_modules`
walking, symlink realpath behaviour as produced by pnpm, and `tsconfig` path aliases layered on top.
Reimplementing it in C++ means reimplementing a moving target, and every divergence is a silently
wrong edge in the graph.

**Decision.** Resolve the common, cheap cases in C++ (relative specifiers, plain file extensions),
and delegate everything else to Node itself: a helper process calls `require.resolve` (and the ESM
equivalent) with the correct parent URL and returns the resolved path. Cache every result keyed by
(specifier, importing file) so the helper is consulted once per distinct pair.

**Consequences.**
- Resolution is correct by construction for the hard cases, and stays correct when Node changes.
- Adds a process boundary and an IPC round trip on the cold path. The cache makes this amortized
  cost, not per-import cost.
- Requires a usable Node binary to be present, which hindsight already requires for the runtime
  phase.
- Resolution now matches the Node version being used, which is a feature: the graph reflects the
  project's actual runtime, not an idealized one.
- Pathological projects with very many distinct unresolved specifiers pay a real startup cost.
  Batching requests to the helper is the mitigation if this shows up in benchmarks.

---

## ADR-3: Run only the project's test command by default

**Context.** The runtime phase has to execute the user's code. Executing arbitrary code from a
project directory is exactly the behaviour that makes a developer tool dangerous: a dev server that
never exits, a script that deploys, a task that writes to a real database. The user typed one
command and does not expect the analyzer to have side effects.

**Decision.** By default `runner` executes only the command the project itself declares as its
**test** command. Running anything else requires an explicit flag (`--run <command>`). Attaching to
an existing process requires an explicit flag as well. No command is ever inferred from a heuristic
guess about what "the app" is.

**Consequences.**
- The default path is bounded: a test suite is expected to terminate and expected to be safe to run.
- Coverage is limited to whatever the test suite exercises. Projects with thin tests get thin
  runtime evidence, and the provenance labelling makes that visible rather than hiding it.
- Users who want fuller coverage have to opt in explicitly, which is the correct place to put that
  decision.
- hindsight needs to detect the test command reliably across package managers and workspace layouts,
  which is real work in `runner`.
- Projects with no test command fall back to a static-only analysis with an explicit notice.

---

## ADR-4: C++ core with a thin web UI

**Context.** The analysis is CPU-bound and memory-bound: parsing an entire codebase, holding a graph
of packages, files and functions, and running SCC, reachability and subtree hashing over it. The
presentation is a graph view and a ranked list. These two have almost nothing in common.

**Decision.** Put everything that computes in a native C++ core: parsing, process control,
graph storage (CSR layout), algorithms, findings. Ship a static web UI that renders data served by
the local `server` module over HTTP. The UI holds no analysis logic and no duplicated model of the
graph; it requests findings and graph data and draws them.

**Consequences.**
- The expensive work runs at native speed with parallel parsing and a compact graph layout, which is
  what makes "analyze the whole codebase" a single command rather than a coffee break.
- The UI is replaceable and testable on its own; the core is usable headlessly via `scan` and
  `export` without a browser.
- The wire format between core and UI becomes a real interface that has to be designed and kept
  stable.
- Contributors need a C++ toolchain even for UI work, unless the UI can be developed against a
  recorded `export` payload. Supporting that recorded payload is planned for exactly this reason.
- No Electron, no bundled browser runtime, no Node dependency for rendering.

---

## ADR-5: C++23 as the project standard

**Context.** The project is developed for a Modern C++ course centred on C++23, so the standard is a
fixed requirement rather than a free choice. The codebase is expected to use C++23 features
deliberately where they improve the code: `std::expected` for error handling without exceptions,
deducing `this`, `if consteval`, the multidimensional subscript operator, `std::print`,
`std::generator`, and the C++23 ranges additions including `std::ranges::to`.

**Decision.** Target C++23 (`CMAKE_CXX_STANDARD 23`, `CMAKE_CXX_STANDARD_REQUIRED ON`,
`CMAKE_CXX_EXTENSIONS OFF`) with a minimum compiler of **GCC 14**, restricted to the subset the
verified toolchain actually implements.

**Verified toolchain.** GCC 14.2.0 (`g++-14`) on Ubuntu 24.04.5, CMake 3.28.3. GCC 14 implements
the C++23 language features this project relies on and ships the libstdc++ pieces that GCC 13 was
missing. The following was established by compiling each feature with the local `g++-14
-std=c++23` — and by linking and running the library cases, not merely parsing them:

| Feature | GCC 14.2.0 | Note |
| --- | --- | --- |
| `std::expected` | available | primary error-handling vocabulary type |
| `std::print` | available | formatted output |
| `std::generator` | available | lazy sequences, e.g. AST traversal |
| deducing `this` | available | |
| `std::ranges::to` | available | materializing range pipelines |
| `if consteval` | available | |
| multidimensional `operator[]` | available | |
| `<stacktrace>` | available | |
| `<spanstream>` | available | |
| **`<flat_map>`** | **not available** | header absent; needs GCC 15 |
| **`<flat_set>`** | **not available** | header absent; needs GCC 15 |
| **`<mdspan>`** | **not available** | header absent; needs GCC 15 |

**Decision detail.** The project requires **GCC 14 or newer**, and CMake fails configuration with an
explanatory message on anything older. The presets select `g++-14` explicitly so that every
contributor and CI get the same compiler without having to think about it.

This is a deliberate choice to *not* limit the project to Ubuntu 24.04's default GCC 13. The cost of
requiring GCC 14 is a single apt package (`sudo apt install g++-14`) on the distribution the project
is developed on. The gain is `std::print`, `std::generator`, deducing `this` and `std::ranges::to` —
four features that materially shape how the code is written, and all four of which GCC 13 rejects
outright. Trading one `apt install` for those is clearly worth it; contorting the codebase around a
compiler that is one version behind is not.

`<flat_map>`, `<flat_set>` and `<mdspan>` remain unavailable until GCC 15 and are therefore **not
used**. Where a sorted flat container is wanted, use a sorted `std::vector`; revisit when the minimum
compiler moves to GCC 15.

**Consequences.**
- The code compiles on the toolchain the project actually requires, and the compile matrix is honest.
- Contributors on Ubuntu 24.04 must install `g++-14`; configuration fails with that exact
  instruction rather than with a wall of template errors from a missing header.
- The three GCC 15 headers above are off-limits, and that limit is recorded rather than discovered
  later by a failing build.
- Every feature in the "available" column has been compiled — and, for the library features, run —
  locally under `g++-14 -std=c++23`. This table is a record of measurement and should be
  re-measured, not edited from memory, when the minimum compiler changes.
- CI installs and builds with the same `g++-14`, so a feature working locally and failing in CI is unlikely.
