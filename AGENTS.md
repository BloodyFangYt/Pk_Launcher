# AGENTS.md — Master Instructions for All AI Agents

> **This is the single source of truth for every AI agent working on PkLauncher.**
> Every agent must read this file before starting any work.

---

## Project Identity

**PkLauncher** is a native C++17 + Qt 6 desktop Minecraft launcher. The project builds with CMake, uses Qt 6 Widgets for the UI, Qt Network for HTTP, SQLite (via Qt SQL) for persistence, and Qt Concurrent for background work.

**This is a C++/Qt project only.** No TypeScript, Rust, Node.js, or web frameworks. The only active source tree is `src/`, `include/`, `tests/`, `resources/`, and `cmake/`.

---

## Core Rules

### 1. No Agent Is Alone
> No agent should assume that it is the only AI working on the repository.

Always check `STATUS.md` and `COMMUNICATION.md` before starting work. Always check `git status` and `git log --oneline -10` before modifying files.

### 2. Read Before Write
Before editing any file:
1. Read the current contents of the file.
2. Understand what it does and who owns it.
3. Check if another agent recently changed it (`git log --oneline -5 <file>`).
4. Make the minimal necessary change.

### 3. Ownership Is Sacred
Each agent owns specific files/directories. If you need to modify another agent's owned files:
1. Post a message in `COMMUNICATION.md` explaining what and why.
2. Wait for acknowledgment (or proceed after a reasonable check if the other agent is idle).
3. Make the smallest possible change.
4. Notify the owning agent after the change.

### 4. No Blind Overwrites
- Never overwrite another agent's work without reading it first.
- Never revert another agent's changes without understanding them.
- Never delete files without documented justification.
- Never refactor code that isn't yours unless asked.

### 5. Test Before Completion
A task is NOT done because code was written. Before marking any task complete:
- The build must succeed: `cmake --build build/`
- Relevant tests must pass: `cd build && ctest --output-on-failure`
- No regressions in existing tests.
- Documentation updated if interfaces changed.

### 6. Communicate Changes
After completing work, update:
1. `STATUS.md` — Mark task progress.
2. `COMMUNICATION.md` — Report what changed, what tests pass, what's next.
3. Relevant `.md` docs if architecture or interfaces changed.

---

## Coding Standards

### C++17
- Use modern C++ features: structured bindings, `std::optional`, `std::filesystem`, `if constexpr`.
- Prefer value types over heap allocation.
- Use RAII everywhere — no raw owning pointers.
- Use `QString`, `QByteArray`, and Qt containers at Qt boundaries.
- Forward-declare in headers; include in `.cpp` files.

### Qt 6
- Use `Q_OBJECT`, signals/slots, and the meta-object system.
- Use `QNetworkAccessManager` for all HTTP — never raw sockets.
- Use `QSqlDatabase` with named connections for SQLite.
- Use `QThread` or `QConcurrent` for background work — never block the UI thread.
- Use Qt Style Sheets for theming, not platform-specific code.

### File Organization
```
include/          — Public headers (declarations)
  core/           — Application, Settings, Auth, Update
  launcher/       — LauncherCore, InstanceManager
  ui/             — MainWindow, DownloadManager
    pages/        — Individual page widgets
src/              — Implementations
  core/
  launcher/
  network/        — (empty, reserved for future API clients)
  ui/
    pages/
  utils/          — (empty, reserved for future utilities)
tests/            — Qt Test files
resources/        — Qt resources (.qrc, images, fonts)
cmake/            — CMake modules
```

### Naming Conventions
- Classes: `PascalCase` (`LauncherCore`, `InstanceManager`)
- Methods: `camelCase` (`fetchVersions`, `detectJava`)
- Member variables: `m_camelCase` (`m_networkManager`, `m_cachedVersions`)
- Files: `PascalCase.cpp` / `PascalCase.h`
- Test files: `test_<module>.cpp`
- Constants: `UPPER_SNAKE_CASE` or `kPascalCase`

### Error Handling
- Return typed results or emit error signals — never hide failures.
- Log errors with `qWarning()` or `qCritical()` — never silently swallow.
- Validate all external input (URLs, paths, JSON, downloaded data).

---

## Security Rules

1. **Validate everything**: URLs, paths, JSON responses, downloaded metadata, user input.
2. **Verify checksums**: SHA-1/SHA-256 before extracting or executing downloaded files.
3. **Build arguments safely**: Use `QProcess::setProgram()` + `setArguments()` — never shell concatenation.
4. **No credentials in logs**: Never log passwords, tokens, or access keys.
5. **No cheats or bypasses**: Never add functionality that bypasses authentication or licensing.
6. **Secure storage**: Tokens go in `QSettings` INI files — never in plain JSON or committed files.

---

## Git Rules

1. **Check status before editing**: `git status` and `git log --oneline -5`.
2. **Commit focused changes**: One logical change per commit.
3. **Write descriptive messages**: What changed and why, not just what files.
4. **Never force push** without explicit owner permission.
5. **Never commit secrets**, API keys, tokens, or passwords.
6. **Never commit generated files** (build artifacts, `.o`, `.moc`, `moc_*.cpp`).

---

## Testing Rules

1. **Test files live in `tests/`** and use Qt Test framework.
2. **Test names**: `test_<module>.cpp` matching the module they test.
3. **Register new tests** in `tests/CMakeLists.txt` using the `pklauncher_add_test` macro.
4. **Run tests before completing work**:
   ```bash
   mkdir -p build && cd build
   cmake .. -DBUILD_TESTS=ON
   cmake --build . --parallel
   ctest --output-on-failure
   ```
5. **Test isolation**: Use `QTemporaryDir` for file-based tests. Never write to real user directories.
6. **Tests must pass** before marking a task as DONE.

---

## Task Management

Tasks are tracked in `TASKS.md`. Each task has:
- **TASK-ID**: Unique identifier.
- **Status**: `BACKLOG | READY | IN_PROGRESS | BLOCKED | REVIEW | TESTING | DONE`.
- **Assigned Agent**: One primary owner.
- **Dependencies**: What must be done first.

### Status Flow
```
BACKLOG → READY → IN_PROGRESS → REVIEW → TESTING → DONE
                    ↓
                  BLOCKED (with reason)
```

### Claiming Tasks
1. Read `TASKS.md`.
2. Find a task with status `READY` or `BACKLOG` that matches your role.
3. Change status to `IN_PROGRESS` and set yourself as assigned.
4. Post in `COMMUNICATION.md` that you've claimed it.
5. **Two agents must never unknowingly work on the same task.**

---

## Conflict Resolution

If two agents need to modify the same file:
1. The agent who needs the change posts in `COMMUNICATION.md`.
2. The other agent is notified and can:
   - Approve the change.
   - Suggest an alternative approach.
   - Merge the changes themselves if they're currently editing that area.
3. If no response, the requesting agent may proceed with minimal changes and notify.
4. For architectural disagreements, escalate to `DECISIONS.md`.

---

## Definition of Done

A task is complete when ALL of the following are true:

| Criterion | How to Verify |
|-----------|---------------|
| Code compiles | `cmake --build build/` succeeds with zero errors |
| Tests pass | `ctest --output-on-failure` — all relevant tests pass |
| No regressions | Existing tests still pass after changes |
| Error handling | Errors are surfaced, not hidden |
| Documentation updated | `STATUS.md`, `COMMUNICATION.md`, and relevant docs reflect changes |
| Minimal changes | Only necessary files modified; no gratuitous refactoring |
| No conflicts | No unaddressed overlap with other agents' work |
| Status verified | Task marked `DONE` with verification notes |

### Status Tags for Work Claims
When reporting in `COMMUNICATION.md`, use one of:
- `IMPLEMENTED` — Code written but not yet tested.
- `TESTED` — Code written and tests pass.
- `PARTIALLY TESTED` — Some tests pass, some still needed.
- `NOT TESTED` — Cannot test in current environment.
- `BLOCKED` — Cannot proceed (specify blocker).

---

## Workflow Summary

```
BEFORE STARTING:
  → Read this file (AGENTS.md)
  → Read PROJECT.md
  → Read ARCHITECTURE.md
  → Read TASKS.md
  → Read STATUS.md
  → Read COMMUNICATION.md
  → Read your agent file (agents/agent-N.md)
  → Run: git status && git log --oneline -10

DURING WORK:
  → Inspect existing code before editing
  → Communicate with relevant agents
  → Make minimal necessary changes
  → Test changes (build + ctest)
  → Update documentation as you go

AFTER WORK:
  → Run full build + test
  → Update STATUS.md with task progress
  → Update COMMUNICATION.md with change report
  → Update DECISIONS.md if you made an architectural decision
  → Notify affected agents
  → Mark task status appropriately
```

---

## Quick Reference

| Document | Purpose |
|----------|---------|
| `AGENTS.md` | This file — master rules |
| `PROJECT.md` | What is this project |
| `ARCHITECTURE.md` | How the code is structured |
| `TASKS.md` | Active and planned tasks |
| `STATUS.md` | Current project state |
| `COMMUNICATION.md` | Agent-to-agent messages |
| `DECISIONS.md` | Architectural decisions log |
| `agents/agent-1.md` | Architect agent profile |
| `agents/agent-2.md` | Core Services agent profile |
| `agents/agent-3.md` | UI/UX agent profile |
| `agents/agent-4.md` | Data & Persistence agent profile |
| `agents/agent-5.md` | Quality & Integration agent profile |
| `Phases.md` | Development phases and milestones |
| `Rules.md` | Engineering rules |
| `Design.md` | UI design guidelines |
| `PRD.md` | Product requirements |
| `conversation.md` | Legacy shared work board (read-only for context) |
