# AGENTS.md

This file defines the operational contract for coding agents working on
LibrePaint. Use it with `docs/architecture/TODO.md`,
`docs/architecture/PROGRESS.md`, `docs/architecture/DEVELOPMENT.md`, and the
platform documents relevant to the active task.

## Communication

Use Japanese for plans, progress, blockers, verification results, and final
reports. Preserve repository language for source code, identifiers, commands,
paths, and quoted diagnostics.

Implementation reports include the change purpose, the resulting capability,
verification commands and results, remaining risk, and the next durable action.

Implementation reports and pull request descriptions explain the change in
reviewer-facing domain language before introducing repository identifiers.
Titles state the architectural or behavioral outcome instead of leading with
target names, class names, or migration labels. Use this order, omitting a
section only when it does not apply:

1. State the concrete problem in the previous structure and its effect on
   ownership, dependencies, behavior, or maintenance.
2. Describe the resulting responsibility boundaries and dependency direction
   in conceptual terms.
3. Identify removed compatibility routes, obsolete structure, dead code, and
   unintended dependencies.
4. State the observable behavior and contracts that remain stable.
5. Give reviewers explicit points to inspect in the implementation.
6. Report verification by platform and scope, distinguishing successful
   checks from unrelated baseline failures and remaining risk.
7. Name the next scoped action.

Repository paths, CMake targets, class names, test names, and commands support
the explanation after the purpose and structural result are clear. A list of
internal identifiers is not a substitute for explaining what changed, why it
changed, and how a reviewer can judge correctness.

## Roadmap Order

LibrePaint follows the roadmap in `docs/architecture/TODO.md`.

1. R1 establishes responsibilities, package boundaries, and dependency
   direction.
2. R2 records behavioral, image, input, and performance contracts.
3. R3 optimizes rendering against the R2 contracts.
4. R4 introduces the Vulkan backend through the stable rendering boundaries.
5. R5 optimizes the mobile UI through shared application boundaries.
6. R6 completes C++20 adoption and repository-wide modernization.

Production integration enters each stage after its prerequisite completion
criteria pass. Exploration for later stages records findings in the relevant
roadmap item. R2 compatibility contracts precede R3 changes to painting
algorithms, execution order, scheduling, and synchronization.

## Resume Procedure

Durable project state lives in the repository documents. Architecture,
refactoring, test-foundation, and roadmap sessions begin with this sequence:

1. Read `docs/architecture/PROGRESS.md`.
2. Read the active gate in `docs/architecture/TODO.md` and its linked design
   or platform documents.
3. Inspect the current branch and worktree.
4. Validate the recorded next action against the current files.
5. Continue that action, or select the earliest planned gate whose
   prerequisites are complete.

Roadmap state changes update `docs/architecture/PROGRESS.md` in the same
change. The snapshot records a JST timestamp, state, gate, purpose, completed
work, next action, and verification status. Active work uses `in_progress`;
the next ready action uses `planned`; paused work records its resumption
condition.

Large roadmap items use reviewable gates with an explicit purpose, scope
boundary, completion criteria, verification tier, and stop condition.

## Development Environment

Nix defines development tools and dependencies. Direnv loads the `test` shell
from `.envrc`, adds repository scripts to `PATH`, and assigns the repository
build and compiler-cache roots. After the one-time `direnv allow`, entering the
repository provides these source-iteration commands:

```sh
build-incremental native build [target]
run-test <target> [ctest-regex]
verify-quick
verify
```

`build-incremental` selects persistent Ninja trees and platform-specific
compiler caches. macOS and Linux use the native test preset. iOS uses its
pinned device environment. Android and Windows use source-independent pinned
profiles on the x86_64 Linux build host. `path`, `configure`, `plan`, `build`,
`bootstrap`, and `cache-stats` expose each supported platform cycle.

Direct Nix entry remains available through `nix develop .#test`.
Documentation work may use `nix develop .#docs`. Required tool additions target
the narrowest relevant shell.

Nix expressions preserve small inputs and reusable cache boundaries.
Source-independent dependencies, LibrePaint compilation, test execution,
application bundling, signing, and deployment use derivations aligned with
their change rates and authority requirements. Policy-source derivations keep
application build outputs reusable across documentation and policy edits.
Clean packaging checkpoints use the named `nix build` output after the
worktree cycle succeeds.

## Implementation Workflow

Every code, build, script, and policy change follows this sequence:

1. Read the relevant implementation, tests, CMake target, and roadmap gate.
2. Identify the smallest coherent change within the intended responsibility.
3. Add or update the smallest meaningful observable contract.
4. Run the contract and record the expected initial diagnostic.
5. Implement the minimum production change that satisfies the contract.
6. Refactor while the relevant contract remains green.
7. Audit responsibility, dependency direction, ownership, lifetime, public
   API, file growth, and platform impact.
8. Synchronize TODO, progress, architecture, fixed test data, and baselines.
9. Run the verification tier required by the change scope.

Compiler options, linters, architecture checks, image comparisons, and
verification scripts retain or increase their enforcement strength. A reviewed
exception records its reason, owner, tracked TODO, maximum scope, and removal
condition.

Each reviewable change groups one feature or one structural concern.
Structural preparation receives its own gate when it has an independent
verification boundary.

## Test-Driven Development

Tests protect behavior and governance checks protect structure.

Use these layers:

- one Qt Test target during the red-green cycle;
- the affected component CTest set before local completion;
- `./scripts/verify-quick` for policy, scripts, and architecture documents;
- `./scripts/verify` for the complete native test gate;
- platform, sanitizer, performance, and device suites at their documented
  integration gates.

Tests assert stable behavior, contracts, and diagnostics. Internal refactors
preserve the same observable contracts.

Characterization and image tests fix the brush or tool configuration, canvas
properties, color space, input sequence, random seed, concurrency settings,
and comparison method. Fixed test data records provenance. Diagnostic output
preserves useful actual and expected artifacts. Baseline acceptance includes a
classification as maintained contract, known defect, or open design question.

Flaky-test quarantine records its owner, reproduction evidence, scope, and
removal condition. Restoration proceeds one test at a time with deterministic
evidence.

## Architecture and Packaging

R1 establishes the authoritative package map. During R1, current public
boundaries remain stable while baseline contracts measure structural progress.

Names express responsibility. Packages named `utils`, `helpers`, `common`,
`core`, or `types` require one documented responsibility and a clear
dependency direction.

Keep these concerns distinct:

- process startup and OS lifecycle;
- application orchestration;
- document lifetime and import/export coordination;
- image, layer, tile, projection, and stroke state;
- input interpretation and tool invocation;
- paint operations and rendering implementation;
- UI presentation and interaction wiring;
- plugin discovery and registration;
- filesystem, process, network, serialization, and platform adapters;
- packaging, signing, and deployment.

UI packages own presentation, screen state, and interaction wiring. Document
models, file I/O, rendering jobs, and platform services remain with their
domain owners.

Internal headers remain inside their owner package. Cross-package APIs have an
explicit owner, documented lifetime and error behavior, and the narrowest
surface that serves the use case.

Stable identifiers evolve through a migration plan and compatibility test.
They include MIME and UTI values, plugin IDs and service types, action IDs,
settings paths and keys, CMake target names, desktop IDs, serialized formats,
and scripting APIs.

Temporary forwarding headers, adapters, compatibility branches, and reviewed
exceptions carry a deletion condition and tracked roadmap item.

## C++ and Qt

Common builds use C++17 facilities through R6. The R6 gate records supported
compilers, standard libraries, Qt versions, and platform constraints for the
language-standard transition.

APIs express ownership, lifetime, nullability, and error behavior. Prefer value
types, RAII, scoped ownership, and established Krita shared-pointer types.
Raw pointers express borrowed Qt or Krita relationships where framework
lifetime rules establish validity; document subtle lifetime relationships.

`QObject` thread affinity, connection delivery mode, event-loop re-entry, and
destruction behavior remain explicit. Stroke queues, update scheduling, image
locking, projection updates, and GUI-thread boundaries receive deterministic
tests or matching dynamic evidence.

Filesystem, process, time, randomness, global state, and platform services
enter deterministic logic through explicit adapters. Adapter boundaries
validate external data before document or image state changes.

Single responsibility guides reuse decisions. Shared abstractions emerge after
their owners and reasons to change align.

Source edits follow surrounding SPDX, licensing, formatting, and naming
conventions. Formatting and renaming scope matches the active gate.

## Governance and Baselines

Governance checks encode repository-owned, reproducible contracts. Current
contracts cover UTF-8 text representation, approved control and formatting
characters, production-source size, shell scripts, architecture documents,
links, and generated diagrams.

The source-size baseline records the G0 maximum for each legacy large file.
Each entry identifies its roadmap owner. Reductions update the recorded maximum
in the same change. Reviewed exceptions contain a reason, tracked TODO,
maximum extent, and removal condition.

Architecture dependency contracts derive from CMake targets and actual
includes. R1 records the current graph, defines the intended graph, and then
turns each dependency direction and cycle rule into a reproducible contract.

## Documentation

Documents contain durable project state, design, commands, and maintenance
instructions. Sentences describe purpose, ownership, inputs, outputs,
execution order, and successful end states in affirmative form. Migration
observations belong to implementation reports and repository history. The
progress snapshot contains the current work and its next action.

- `docs/architecture/TODO.md` owns the cross-platform roadmap and gate state.
- `docs/architecture/PROGRESS.md` owns the current resumable work snapshot.
- `docs/architecture/README.md` owns the stable architecture guide.
- `docs/architecture/DEVELOPMENT.md` owns development and verification usage.
- `docs/<platform>/` owns platform design and validation details.

D2 sources own architecture diagrams. Diagram updates regenerate their SVG
outputs through the documented render command.

## Verification Matrix

Documentation and policy changes run:

```sh
nix develop .#test --command ./scripts/verify-quick
```

One native test target runs:

```sh
nix develop .#test --command ./scripts/run-test <target> [ctest-regex]
```

The complete native gate runs:

```sh
nix develop .#test --command ./scripts/verify
```

Nix output changes also run:

```sh
nix flake check --no-build --all-systems
```

Platform-boundary changes run the matching build, artifact, simulator, device,
or performance verification. The active progress snapshot records exact
commands and results.

## Completion

A completed task has:

- the requested behavior or policy;
- observable contracts for the changed behavior;
- successful required checks in the Nix environment;
- an architecture and platform impact assessment;
- justified baselines and reviewed exceptions;
- synchronized TODO, progress, and architecture documents;
- current generated documentation artifacts;
- preserved user-owned worktree changes.

Commits, pushes, merges, branch deletion, and artifact publication occur after
an explicit user request.
