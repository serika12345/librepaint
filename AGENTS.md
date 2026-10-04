# AGENTS.md

This file defines the operational contract for coding agents working on
LibrePaint. `docs/architecture/DEVELOPMENT.md` owns development procedures;
`docs/architecture/README.md` owns design boundaries. GitHub Issues and Projects
own task planning and progress.

## Communication

Use Japanese for plans, progress, blockers, verification results, and final
reports. Preserve repository language for source code, identifiers, commands,
paths, and quoted diagnostics.

Implementation reports include the change purpose, the resulting capability,
verification commands and results, remaining risk, and the next durable action.
Structural implementation reports lead with the purpose and identify every
starting file or directory together with its destination file or directory.
Internal target and class identifiers may support that mapping, but do not
replace it.

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

For a split, extraction, relocation, or ownership transfer, report the exact
starting files or directories and their destination files or directories as a
traceable mapping. File paths are required review entry points even when class,
target, and test identifiers are unnecessary supporting detail.

## Issue Approval and Sensitive Information

Every Issue write requires an explicit user request, followed by presentation
of the exact proposed text or change and the user's approval of that proposal.
This includes creation, comments, body edits, metadata and state changes, and
deletions. A task request, implementation approval, or repository synchronization
requirement does not authorize an Issue write. Approval applies only to the
presented change. This rule takes precedence over all Issue-recording and
synchronization procedures in this repository, including completion reporting
and correction of existing disclosures.

Treat local work details and security operations as sensitive, even when secret
values are absent. Keep actual key storage and recovery arrangements, credential
configuration status, personal paths, host and device availability, and local
storage inventories out of Issues, Projects, and tracked documents.
Discuss operational details in the conversation; use private
storage only when the user designates it. Generic reproducible procedures and
public software facts belong in repository documentation. Public reports use
only the minimum project facts needed to describe behavior and verification.

## Work Planning

Each Issue defines a concrete outcome, scope, prerequisites, completion
criteria, verification, and stop conditions. Describe prerequisites as required
results together with their owning Issue links. Use GitHub sub-issues for work
decomposition and issue dependencies for work that requires another Issue to
finish. Related work and partial prerequisites remain explicit in the body.

GitHub Projects owns priority and workflow status. Issue open/closed state and
the closure reason record the accepted disposition. Milestones group bounded
deliverables or releases. Each Issue remains understandable from its own body;
titles and acceptance criteria use concrete responsibilities and behavior.

For substantial design and improvement Issues, organize the body in this order:

1. Desired outcome and purpose: explain what users or maintainers gain.
2. Current problems: describe the concrete behavior or structure motivating
   the change.
3. Approaches considered: compare relevant alternatives and their tradeoffs.
4. Selected approach and rationale: explain the choice, responsibility
   boundaries, dependency direction, and constraints requiring validation.
5. Execution stages: define the progression, scope, prerequisites, and
   reviewable implementation units.
6. Completion criteria and verification: state observable success conditions
   and the evidence required to accept the result.
7. Stop conditions: identify findings that require reassessing the design,
   selected approach, or scope before integration.

Lead with the intended outcome and explain implementation choices after the
problem is clear. Keep the comparison and selection rationale in the Issue
body so it remains understandable on its own. Use tables for alternatives
and responsibility mappings, numbered stages for execution order, and
checkboxes for verifiable completion criteria. Scale the structure to the
task: small fixes use only the applicable parts, with detail proportional to
their scope. Sections may be combined when that makes the Issue easier to read.

Production integration requires the prerequisites recorded in its Issue.
Exploration records its findings in the relevant Issue. Behavioral, image,
input, and performance contracts precede changes to painting algorithms,
execution order, scheduling, and synchronization.

## Resume Procedure

GitHub Issues own task scope, prerequisites, completion criteria, and
verification results. Projects owns workflow status and priority. Repository
documents own architecture and development usage. Begin each session with this
sequence:

1. Inspect the repository's GitHub Project and open Issues for active work or
   the earliest ready task.
2. Read the selected Issue, its parent, dependencies, linked pull requests,
   latest verification results, and relevant manual and architecture sections.
3. Inspect the current branch and worktree.
4. Validate the recorded next action against the Issue and current files.
5. Continue that action, or select the earliest ready Issue whose prerequisites
   are complete.

Report task state, verification, and the next operation in the conversation.
Issue updates, including relationship changes and closure, follow the Issue
approval procedure above. Projects tracks workflow status; paused work records
its resumption condition in the owning Issue. Keep public reports limited to
project behavior and verification. Issues, pull requests, and Git history
retain approved public records.

Large tasks use parent Issues and bounded child Issues with an explicit
purpose, scope, completion criteria, verification tier, and stop condition.

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

One evaluated development profile serves the complete source-iteration
session. A shell already loaded by direnv runs the commands above directly.
Automation or another process that does not inherit that shell uses
`./scripts/run-shared-test-env <command> [arguments...]`, including in the
primary worktree. The helper loads the stable `.direnv/flake-profile` through
`nix print-dev-env`; it does not evaluate the current worktree as a new local
flake input.

Direct `nix develop .#test` and `nix develop .#docs` entry remains available
for initial profile creation and after the corresponding development-shell,
flake input, lock file, or source-filter definition changes. Do not wrap each
source edit, target build, test, verification command, or per-target commit in
`nix develop` or `direnv exec`: a distinct local-flake source state enters the
immutable Nix store, and the application source filter can create a second
near-complete source path. Required tool additions target the narrowest
relevant shell and receive one deliberate environment reevaluation.

Before and after a deliberate local-flake reevaluation during a large task,
compare the dead `*-source` and `*-librepaint-source` path count and
recoverable size. Unexpected growth stops further local-flake evaluation; the
session resumes through the last valid cached profile and reports the cause in
the conversation. Keep local storage measurements in the conversation.
Garbage collection is a separate storage
operation: resolve exact dead paths, preserve active profiles and build caches,
and obtain the authority required by the destructive-action rules before
deleting them.

Nix expressions preserve small inputs and reusable cache boundaries.
Source-independent dependencies, LibrePaint compilation, test execution,
application bundling, signing, and deployment use derivations aligned with
their change rates and authority requirements. Policy-source derivations keep
application build outputs reusable across documentation and policy edits.
Clean packaging checkpoints use the named `nix build` output after the
worktree cycle succeeds.

## Implementation Workflow

Every code, build, script, and policy change follows this sequence:

1. Read the relevant implementation, tests, CMake target, and Issue criteria.
2. Identify the smallest coherent change within the intended responsibility.
3. Before editing implementation or contract code, inspect the target-scoped
   incremental work plan and direct CMake dependencies. For a new or expanded
   target, also measure its clean-tree command closure against the nearest
   existing contract. Narrow an overbroad target or dependency before the
   behavioral change; record why a remaining large concrete-owner closure is
   necessary.
4. Add or update the smallest meaningful observable contract.
5. Run the contract and record the expected initial diagnostic.
6. Implement the minimum production change that satisfies the contract.
7. Refactor while the relevant contract remains green.
8. Audit responsibility, dependency direction, ownership, lifetime, public
   API, file growth, and platform impact.
9. Synchronize public-safe repository documentation, fixed test data, and
   baselines. Apply the Issue approval procedure to any requested Issue update.
10. Run the verification tier required by the change scope.

Required checks directly protect a documented responsibility, dependency,
public boundary, observable behavior, or platform artifact boundary. Maintain
one authoritative check for each guarantee. Remove checks that only freeze
implementation spelling, completed relocations, formatting, or inventories
without a current consumer contract. Review retained guarantees when changing
the verification set.

Each reviewable change groups one feature or one structural concern.
Structural preparation receives its own gate when it has an independent
verification boundary.

## Parallel Agent Work

Parallel implementation uses one coordinator and non-overlapping worker
lanes. The coordinator records one base commit and gives every lane a task
packet containing the exact public headers and API identifiers, allowed paths,
owned CMake files and targets, nearest contract, platform scope, build
permission, Git authority, integration order, and stop conditions. Share active
lane packets and their states within the working conversation before workers
start. Each lane identifies its coordinator Issue. Two active lanes
never share a production header,
implementation file, test source, CMake file, or generated artifact.

Each worker lane uses a dedicated Git worktree and a worktree-local Ninja
build tree. Native compiler-cache storage may be shared, while build trees and
configuration markers remain isolated. The coordinator controls concurrent
configure, build, test, and verification capacity so that host load does not
turn target-scoped validation into an accidental full build.

Worker lanes enter the primary worktree's cached test environment through
`./scripts/run-shared-test-env` and execute the lane-local script path. The
helper preserves the lane repository root, build tree, compiler-cache base,
and compilation database while sharing the primary tool environment and
compiler-cache storage. A lane does not evaluate `nix develop .#test` against
its own full source tree because each distinct worktree revision would create
another large Nix store source path. Work that changes the Nix development
environment uses an explicitly assigned primary-worktree lane instead.

The coordinator exclusively owns `AGENTS.md`, shared architecture documents,
and shared generated artifacts unless a task packet
explicitly transfers one of those files. A worker changes only its assigned
production, test, fixture, and package-local CMake paths. It reports behavioral
guarantees and documentation facts as structured handoff data instead of
editing coordinator-owned files.

Workers follow the complete implementation workflow within their lane,
including the unchanged build plan, direct dependencies, clean command
closure, expected first diagnostic, target test, repetition, and platform
result. A lane task packet is the worker's scoped continuation of the global
Issue; the worker does not select the coordinator's next action or
delegate further work unless its packet explicitly authorizes that action. A
worker stops and reports when required work crosses its allowed paths, overlaps
another lane, changes an unassigned public API, needs an unassigned dependency,
or exposes an ambiguous behavior classification.

The coordinator inspects and integrates one ready lane at a time, synchronizes
architecture documents in the integrated change and reruns the affected
contract and governance checks. Lane commits are transport artifacts rather
than completed `develop` changes. Commits, branch creation,
integration, worktree removal, and branch deletion still require the authority
defined by the user and the Completion section below.

The coordinator removes obsolete generated storage as soon as its replacement
is verified. Completed lane worktrees include their lane-local build trees in
the same removal. Keep the reusable primary Ninja tree and shared compiler
cache. Remove obsolete lane build artifacts after integrated tests succeed.
Report retained storage and reclaimed lane storage in the conversation.
Preserve user-owned artifacts and do not
discard the primary incremental tree or shared cache while they remain useful.

The task-packet, worktree, handoff, and integration procedures live in the
"責務単位の並列実装" section of
`docs/architecture/DEVELOPMENT.md`.

## Test-Driven Development

Tests protect behavior and governance checks protect structure.

Existing tests are maintained from their source and CMake definitions. Review
assertions against callers and explicit requirements. Each retained test must
explain what breaks for a caller when it fails. Preserve observable results,
state transitions, necessary notifications, effects, errors, and domain
invariants. Compatibility tests require an explicit compatibility requirement.
Remove declaration-shape and implementation-detail assertions when they carry
no public guarantee; add a behavioral test only for a required observable
contract that existing tests do not cover. API inventories and declaration
coverage quotas are not maintained because they encourage fixing incidental
implementation structure. Record decisions in review descriptions and report
current work in the conversation. Approved Issue updates retain public results.

Before adding or expanding a contract test, identify the consumer, operation,
observable result, and concrete caller-visible failure. Treat one use case or
state transition as the coverage unit; a declaration is not a coverage unit.
Behavioral tests assert observable results. Compatibility tests document the
consumer and stable property that require a declaration or format to remain
unchanged. Review these requirements with the corresponding production callers.

Use these layers:

- one Qt Test target during the red-green cycle;
- the affected component CTest set before local completion;
- `./scripts/verify-quick` for dependency, public-header, and registration boundaries;
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

The package-boundary policy defines ownership and allowed dependency direction.
Structural changes preserve public boundaries through the corresponding
compatibility contracts.

### Refactoring Order and YAGNI

Structural refactoring proceeds in this order:

1. Make dependency paths one-directional.
2. Replace vague package and target names with concrete responsibility names.
3. Split and aggregate existing code by demonstrated areas of concern.
4. Reconstruct logic and introduce abstractions only after the ownership and
   dependency boundaries expose a current need.

YAGNI has priority over speculative extensibility during refactoring. Prefer a
direct dependency on a concrete owner when the direction is correct and one
production implementation satisfies the current behavior. Do not introduce a
use-case layer, port, adapter, repository, service locator, factory, registry,
base class, or generic target solely for hypothetical replacement, future I/O
isolation, or easier mocking.

A new abstraction requires evidence in the active change: multiple current
production implementations, a required deterministic test seam that values
cannot provide, an external boundary that the requested behavior must replace,
or a dependency cycle that ownership and relocation cannot remove. The same
change supplies its production consumer and implementation, observable
contract, ownership and lifetime, and concrete name. Plans do not reserve empty
layers or targets for possible future abstractions.

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
exceptions carry a deletion condition and tracked Issue.

## C++ and Qt

The common CMake configuration defines the language baseline. Language-standard
changes verify supported compilers, standard libraries, Qt versions, generated
code, and platform constraints before changing that baseline.

APIs express ownership, lifetime, nullability, and error behavior. Prefer value
types, RAII, scoped ownership, and established Krita shared-pointer types.
Raw pointers express borrowed Qt or Krita relationships where framework
lifetime rules establish validity; document subtle lifetime relationships.

`QObject` thread affinity, connection delivery mode, event-loop re-entry, and
destruction behavior remain explicit. Stroke queues, update scheduling, image
locking, projection updates, and GUI-thread boundaries receive deterministic
tests or matching dynamic evidence.

When deterministic logic currently requires isolation from filesystem,
process, time, randomness, global state, or platform services, prefer passing
validated values and explicit results. Introduce an adapter only when the
active behavior requires substitution or effect isolation, and validate
external data before document or image state changes.

Single responsibility guides reuse decisions. Shared abstractions emerge after
their owners and reasons to change align.

Source edits follow surrounding SPDX, licensing, formatting, and naming
conventions. Formatting and renaming scope matches the active gate.

## Governance

Governance checks protect the compact package-boundary policy, public-header
visibility, and plugin registration integrity. Their tests exercise rejected
dependencies and invalid boundary inputs. Platform artifact checks inspect the
actual executable, linked resources, and installed runtime data.

Asset manifests and notices retain provenance and adopted scope. Review asset
changes against those sources; binary checks enforce the selected resource
boundary. Documentation maintenance uses source review and diagram generation.

Architecture dependency contracts derive from the current CMake File API graph.
Each platform configure checks target ownership, allowed dependency direction,
and product-target cycles against the compact policy. Generated inventories
support source review; continuing checks protect current consumer contracts.

## Documentation

Documents contain durable design, commands, and maintenance
instructions. Sentences describe purpose, ownership, inputs, outputs,
execution order, and successful end states in affirmative form. Migration
observations belong to implementation reports and repository history.

- GitHub Issues own task scope, prerequisites, acceptance, and verification.
- GitHub Projects owns task priority and workflow status.
- GitHub sub-issues and dependencies own task hierarchy and completion blockers.
- `docs/architecture/README.md` owns shared and platform-specific design boundaries.
- `docs/architecture/DEVELOPMENT.md` is the single development manual for all
  platforms, including setup, builds, verification, deployment, and maintenance.
- Asset manifests and attribution documents own license evidence and adopted scope.

Update the relevant manual section for new procedures. Issue records require
the Issue approval procedure; Git history retains committed public changes.
Link version and inventory
information to its source definition.

D2 sources own architecture diagrams. Diagram updates regenerate their SVG
outputs through the documented render command.

## Verification Matrix

Architecture policy and boundary-check changes run:

```sh
./scripts/run-shared-test-env ./scripts/verify-quick
```

Documentation changes review links and design consistency. D2 changes regenerate
the corresponding SVG with `scripts/docs/render-architecture.sh` in the docs shell.

One native test target runs:

```sh
./scripts/run-shared-test-env ./scripts/run-test <target> [ctest-regex]
```

The complete native gate runs:

```sh
./scripts/run-shared-test-env ./scripts/verify
```

Nix output changes also run:

```sh
nix flake check --no-build --all-systems
```

Platform-boundary changes run the matching build, artifact, simulator, device,
or performance verification. Report commands and results in the conversation.
Requested Issue updates require approval of the exact public-safe text or
metadata change.

## Completion

A completed task has:

- the requested behavior or policy;
- observable contracts for the changed behavior;
- successful required checks in the Nix environment;
- an architecture and platform impact assessment;
- justified baselines and reviewed exceptions;
- synchronized architecture documents and development procedures;
- Issue updates only when explicitly requested and their exact changes approved;
- current generated documentation artifacts;
- preserved user-owned worktree changes.

Commits, pushes, merges, branch deletion, and artifact publication occur after
an explicit user request.
