# AGENTS.md

Operating contract for coding agents working on LibrePaint.
`docs/architecture/DEVELOPMENT.md` owns development procedures and
`docs/architecture/README.md` owns design boundaries.

## Current priority

Drawing performance is the project priority, starting with the delay between
pen input and the displayed result. Before any performance work, read
`docs/perf/CHARTER.md` (requirements, owned by the user) and Issue #108
(status and measurements). Only the user changes the CHARTER.

## Autonomy

- Decide everything that the CHARTER and its primary metric leave open. Do
  not ask the user for confirmation. Record each non-trivial decision as one
  line in `docs/perf/DECISIONS.md`: date, decision, reason, affected metric.
- Every work item names the metric it improves or the CHARTER clause it
  satisfies. Do not start work that can name neither.
- Before deciding on document model, history, failure recovery, precision,
  file formats, extension boundaries, or threading, check how comparable
  established raster and image editors handle it. Do not build guarantees
  beyond common practice unless a measurement requires them.
- Stop and write one line in Issue #108 "停止の記録" only when a CHARTER stop
  condition is met.
- These actions require the user's explicit approval: changing the CHARTER,
  merging into `develop` or `master`, force-pushing or rewriting history,
  deleting branches or tags, and publishing releases.
- Commit and push working branches without asking. Keep the Issue #108
  measurement and phase sections current. Before editing an Issue body, fetch
  the latest body and replace only the lines you change.

## Communication

Use Japanese for plans, progress, blockers, verification results, and final
reports. Keep source code, identifiers, commands, and paths in the repository
language. Reports state the purpose, the result, verification with commands
and outcomes, remaining risk, and the next action.

Keep local operational details out of Issues and tracked documents: key and
credential storage, personal paths, host and device availability, and local
storage inventories. Public text uses only the project facts needed to
describe behavior and verification.

## Development environment

Nix defines tools and dependencies; direnv loads the `test` shell. Work,
builds, and commits happen on the Linux development host. In a shell loaded
by direnv:

```sh
build-incremental native build [target]
build-incremental windows build krita
run-test <target> [ctest-regex]
verify-quick
verify
```

Processes that do not inherit that shell use
`./scripts/run-shared-test-env <command>`. Do not wrap each edit or build in
`nix develop`; that copies new source states into the Nix store. Nix
expressions keep inputs small and separate dependency builds from
application bundling so local changes reuse the binary cache.

## Implementation

1. Read the relevant code, tests, CMake target, and the CHARTER clause.
2. Make the smallest coherent change within one responsibility.
3. Add or update the smallest observable contract test, or a measurement for
   performance work. Measure before and after on the same device and input.
4. Run the target test, then `verify-quick` for boundary changes. `verify`
   runs before a branch is proposed for `develop`.

Prefer direct dependencies on concrete owners. Add an abstraction only when
the active change has multiple production implementations, a required test
seam, or a dependency cycle that relocation cannot remove. Temporary
compatibility code carries its removal condition. Do not create test files
over 2,000 lines. Quarantine a flaky test only with its reproduction and
removal condition recorded in Issue #108.

## Documentation

Documents describe durable design, commands, and maintenance in affirmative
sentences. `docs/architecture/DEVELOPMENT.md` is the single manual for setup,
builds, verification, and deployment; update its section when a procedure
changes. D2 sources own architecture diagrams; regenerate SVGs with
`scripts/docs/render-architecture.sh`.
