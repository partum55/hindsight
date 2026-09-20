# Roadmap

Milestones are ordered so that something demonstrable exists as early as possible. Each milestone
ends with a command a user can run and an artifact they can look at.

Nothing below is implemented. The current state is the repository skeleton.

---

## M1 — Static parse and graph

Parse a real project and produce a graph from source alone.

- `parser`: tree-sitter grammars for JS and TS; parallel file walk; symbol extraction (functions,
  classes, methods), imports/exports, call sites.
- `resolver`: cheap in-process resolution for relative specifiers; Node helper process with
  `require.resolve` for everything else; result cache.
- `graph`: CSR graph store; package/file/function nodes; import and call edges.
- `runner`: project detection only (package.json, package manager, workspaces, tsconfig paths).
- `cli`: `hindsight export` writes the graph as JSON.

**Demo:** `hindsight export .` on a medium TypeScript project produces a graph JSON with correct
file-to-file import edges.

---

## M2 — Coverage ingestion and merge

Add the first runtime evidence and the provenance model.

- `runner`: spawn the project's test command with `NODE_V8_COVERAGE` set; collect the output
  directory; clean up.
- `coverage`: parse V8 coverage JSON; map byte ranges onto the functions from M1.
- `sourcemap`: VLQ decoding; map generated positions back to original files (required as soon as the
  project is compiled TS).
- merge: unified graph with per-node and per-edge provenance (`STATIC` | `RUNTIME` | `BOTH`).

**Demo:** the exported graph marks which functions actually executed during the test suite, and
which modules were loaded.

---

## M3 — CDP client

Add live inspection, exceptions and attachment to running processes.

- `cdp`: WebSocket transport; JSON-RPC request/response correlation; event dispatch.
- Discovery via the `--inspect` HTTP endpoint (`GET /json/list` -> `webSocketDebuggerUrl`).
- Debugger, Profiler and Runtime domains: loaded scripts, thrown exceptions with stack frames.
- `--attach <pid>`: `SIGUSR1` to open the inspector on an already running process (POSIX only).

**Demo:** `hindsight scan --attach <pid>` against a running server reports which functions have run
and where an exception was thrown.

---

## M4 — Findings and web UI

The first end-to-end product.

- Algorithms to findings: import cycles (SCC), unreachable code (reachability from entry points),
  dependency weight attributed to functions.
- Findings model: ranked list, each with a provenance label and a source location.
- `server`: local HTTP server.
- Static web UI: graph view plus the ranked findings list.
- `cli`: `hindsight scan` and `hindsight serve` complete.

**Demo:** `npx hindsight` on an unfamiliar project opens a page with the graph and a ranked list,
including "package X costs N MB for one function that never ran".

---

## M5 — Clone detection

- `clones`: AST normalization with identifier names erased; subtree hashing; bucketing of similar
  hashes; refinement pass over candidate pairs to remove false positives.
- Clone findings integrated into the ranked list.

**Demo:** structurally duplicated functions are reported with both locations.

---

## M6 — Benchmarks against knip

- A benchmark harness over a set of real open-source projects.
- Measure wall-clock time and peak memory against knip.
- Measure agreement and disagreement on unused-export findings, and classify each disagreement:
  where runtime evidence proves hindsight right, where it proves hindsight wrong, and where neither
  tool can tell.

**Demo:** a reproducible benchmark report with both the performance numbers and the
correctness comparison, including the cases hindsight gets wrong.
