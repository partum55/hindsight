# Architecture

Status: design document. None of this is implemented yet.

hindsight is a C++20 core with a thin static web UI. The core does all parsing, process control,
graph construction and analysis; the UI only renders what the core serves.

## Modules

Each module is a directory under `src/`. Modules are separated by responsibility, not by layer.

| Module | Responsibility |
| --- | --- |
| `cli` | Argument parsing, subcommands (`scan`, `serve`, `export`), output formatting. |
| `runner` | Zero-config project detection and process control: `package.json`, package manager, workspaces, test command, `tsconfig` path aliases; spawning the target process, port allocation, cleanup on exit and on signal. |
| `cdp` | C++ client for the Chrome DevTools Protocol: WebSocket transport, JSON-RPC request/response correlation, event stream dispatch. Consumes the Debugger, Profiler and Runtime domains. |
| `coverage` | Ingestion of V8 coverage JSON; mapping byte ranges to the functions found by `parser`. |
| `parser` | tree-sitter based parallel parsing of JS/TS; symbol extraction, imports/exports, call sites. |
| `resolver` | Module resolution. Hard cases are delegated to Node itself via `require.resolve` in a helper process; results are cached. See DESIGN-DECISIONS ADR-2. |
| `graph` | Compact in-memory graph store (CSR layout) and graph algorithms: strongly connected components for import cycles, reachability from entry points, weight attribution. |
| `clones` | Structural clone detection: AST normalization (identifier names erased), subtree hashing, bucketing of similar hashes, refinement of candidate pairs. |
| `sourcemap` | VLQ decoding; mapping generated positions back to original files. |
| `server` | Local HTTP server serving findings and graph data to the static web UI. |

### Safety rule in `runner`

By default `runner` executes only the project's **test command** as declared by the project itself.
Running any other command requires an explicit CLI flag. See DESIGN-DECISIONS ADR-3.

## Data flow

```
                +---------------------------+
                |        runner             |
                | detect project, spawn      |
                +------------+--------------+
                             |
        +--------------------+--------------------+
        |                                         |
        v                                         v
  +-----------+                         +-------------------+
  |  parser   |  parallel tree-sitter   |  target process   |
  |  (static) |                         |  (runtime)        |
  +-----+-----+                         +---------+---------+
        |                                         |
        | symbols, imports/exports,               | NODE_V8_COVERAGE
        | call sites                              | CDP over WebSocket
        v                                         v
  +-----------+                         +-------------------+
  | resolver  |                         | coverage |  cdp   |
  | specifier |                         | counts   | loaded |
  | -> file   |                         |          | modules|
  +-----+-----+                         |          | throws |
        |                               +----+-----+---+----+
        |                                    |         |
        |                                    v         |
        |                              +-----------+   |
        |                              | sourcemap |<--+
        |                              +-----+-----+
        |                                    |
        +----------------+-------------------+
                         v
                  +-------------+
                  |    merge    |  unified graph, per-edge provenance
                  +------+------+
                         |
                         v
                  +-------------+
                  |    graph    |  SCC, reachability, weight
                  |   clones    |  structural duplicates
                  +------+------+
                         |
                         v
                  +-------------+
                  |  findings   |
                  +------+------+
                         |
             +-----------+-----------+
             v                       v
        +---------+            +-----------+
        |   cli   |            |  server   | -> static web UI
        +---------+            +-----------+
```

In words:

1. **Static parse** produces symbols and edges: package -> file, file -> file (imports),
   function -> function (call sites).
2. **Runtime run** produces evidence: per-function execution counts from V8 coverage, the set of
   modules actually loaded, and the exceptions thrown with their stack frames.
3. **Merge** unifies both into one graph over packages, files and functions. Runtime positions are
   translated back to source positions by `sourcemap` before merging.
4. **Algorithms** run over the merged graph and emit findings.
5. **Output** goes to the CLI and to the local web page.

## Provenance

Every node and edge in the merged graph carries a provenance tag:

| Tag | Meaning |
| --- | --- |
| `STATIC` | Found by reading source only. A hypothesis. It may be wrong: a dynamic `import()`, a decorator or a callback may make the real behaviour differ. |
| `RUNTIME` | Observed during execution. Proof that it happened at least once in the run that was observed. |
| `BOTH` | Predicted statically and observed at runtime. |

Provenance is the reason hindsight exists, so it is never collapsed or averaged away. It
propagates to findings: "function `f` is never called" is a different claim depending on whether
`f` is `STATIC`-only (the analyzer never saw a call site, and might have missed one) or whether the
run covered `f`'s module and the execution count was still zero.

Absence of `RUNTIME` is not proof of absence; it is bounded by what the observed run exercised.
Findings state their evidence, and the UI shows the label next to every finding.
