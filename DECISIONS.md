# DECISIONS.md — Architectural Decision Log

> **Every important technical decision is recorded here.**
> Agents must consult this file before proposing changes that contradict existing decisions.

---

## Decision Format

```
### DECISION-XXX: Title

- **Date**: YYYY-MM-DD
- **Decision**: What was decided
- **Reason**: Why this was chosen
- **Alternatives Considered**: What else was evaluated
- **Affected Components**: What this impacts
- **Agents Involved**: Who participated
- **Status**: ACCEPTED | SUPERSEDED | DEPRECATED
```

---

## Decisions

### DECISION-001: C++17 + Qt 6 is the sole technology stack

- **Date**: 2026-08-22
- **Decision**: PkLauncher will be built exclusively with C++17 and Qt 6. No TypeScript, Rust, Node.js, or web frameworks.
- **Reason**: Owner directive. Native desktop performance, cross-platform support, mature widget toolkit.
- **Alternatives Considered**: Tauri + Rust, Electron + TypeScript, Leptos + WASM
- **Affected Components**: All source code, build system, documentation
- **Agents Involved**: All (owner decision)
- **Status**: ACCEPTED

### DECISION-002: CMake as the build system

- **Date**: 2026-08-22
- **Decision**: Use CMake 3.16+ for build configuration and packaging. No Conan, vcpkg, or other package managers in the core build.
- **Reason**: Qt 6 officially supports CMake. Cross-platform. Well-understood.
- **Alternatives Considered**: Conan, vcpkg, Meson, qmake
- **Affected Components**: `CMakeLists.txt`, build instructions
- **Agents Involved**: Agent-2, Agent-5
- **Status**: ACCEPTED

### DECISION-003: Qt Style Sheets for theming

- **Date**: 2026-08-22
- **Decision**: Use Qt Style Sheets (QSS) for all visual theming. Dark charcoal surfaces with red accent (`#FF0033`).
- **Reason**: Cross-platform, no native styling dependencies, easy to maintain.
- **Alternatives Considered**: Custom QPainter rendering, QProxyStyle, platform-native styles
- **Affected Components**: MainWindow, all pages, DownloadManager
- **Agents Involved**: Agent-3 (UI/UX)
- **Status**: ACCEPTED

### DECISION-004: SQLite via Qt SQL for persistence

- **Date**: 2026-08-22
- **Decision**: Use `QSqlDatabase` with the `QSQLITE` driver for instance metadata. No ORM, no raw SQLite C API.
- **Reason**: Qt-native, cross-platform, no extra dependencies, simple schema.
- **Alternatives Considered**: Raw SQLite C API, QSettings for all data, PostgreSQL
- **Affected Components**: InstanceManager, future persistence needs
- **Agents Involved**: Agent-4 (Data & Persistence)
- **Status**: ACCEPTED

### DECISION-005: JSON for settings, INI for tokens

- **Date**: 2026-08-22
- **Decision**: Application settings stored as JSON (`settings.json`). Auth tokens stored as INI (`auth.ini`) via `QSettings`.
- **Reason**: JSON is human-readable and supports complex structures. INI via QSettings provides platform-standard token storage.
- **Alternatives Considered**: All JSON, all INI, TOML, YAML
- **Affected Components**: Settings, AuthManager
- **Agents Involved**: Agent-4 (Data & Persistence)
- **Status**: ACCEPTED

### DECISION-006: QNetworkAccessManager for all HTTP

- **Date**: 2026-08-22
- **Decision**: All network requests use `QNetworkAccessManager`. No libcurl, no raw sockets, no third-party HTTP libraries.
- **Reason**: Qt-native, integrated with event loop, handles HTTPS via OpenSSL, consistent API.
- **Alternatives Considered**: libcurl, cpp-httplib, Boost.Beast
- **Affected Components**: LauncherCore, AuthManager, UpdateManager, future network clients
- **Agents Involved**: Agent-2 (Core Services)
- **Status**: ACCEPTED

### DECISION-007: No mod loaders in MVP

- **Date**: 2026-08-22
- **Decision**: Forge, Fabric, and Quilt mod loader support is deferred to a later release. MVP focuses on vanilla Minecraft.
- **Reason**: Scope management. Mod loaders add significant complexity (custom installers, version-specific patches).
- **Alternatives Considered**: Include Fabric in MVP, include Forge in MVP
- **Affected Components**: InstanceManager (loader field exists but unused), LauncherCore
- **Agents Involved**: Agent-1 (Architect)
- **Status**: ACCEPTED

### DECISION-008: QProcess for game launch

- **Date**: 2026-08-22
- **Decision**: Use `QProcess` with `setProgram()` + `setArguments()` for launching Minecraft. Never shell concatenation.
- **Reason**: Security (no shell injection), cross-platform, integrates with Qt event loop, captures stdout/stderr.
- **Alternatives Considered**: `system()`, `fork()+exec()`, `std::system`
- **Affected Components**: LauncherCore::launchGame()
- **Agents Involved**: Agent-2 (Core Services)
- **Status**: ACCEPTED

### DECISION-009: Static library for shared sources

- **Date**: 2026-08-22
- **Decision**: All source files except `main.cpp` are compiled into a static library `PkLauncherLib`. Both the main executable and test executables link against this library.
- **Reason**: Avoids recompilation for each test target. Clean separation of app vs. test entry points.
- **Alternatives Considered**: Object library, header-only, recompile per target
- **Affected Components**: `CMakeLists.txt`, `tests/CMakeLists.txt`
- **Agents Involved**: Agent-5 (Quality & Integration)
- **Status**: ACCEPTED

### DECISION-010: Qt Test framework

- **Date**: 2026-08-22
- **Decision**: Use Qt Test (`QTest`, `QTEST_MAIN`) for all unit and integration tests. No Google Test, no Catch2.
- **Reason**: Native to Qt, no extra dependencies, good signal/slot testing, integrates with CTest.
- **Alternatives Considered**: Google Test, Catch2, doctest
- **Affected Components**: All test files in `tests/`
- **Agents Involved**: Agent-5 (Quality & Integration)
- **Status**: ACCEPTED
