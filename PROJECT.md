# PROJECT.md — PkLauncher Project Overview

## What Is PkLauncher?

PkLauncher is a **native cross-platform Minecraft launcher** for Windows, macOS, and Linux. It provides a fast, modern desktop experience for managing Minecraft versions and local game instances.

**Stack**: C++17, Qt 6 (Widgets, Network, SQL, Concurrent), CMake, Qt Test.

## Product Goals

### MVP (Current Focus)
- ✅ Native Qt 6 dark-themed interface with red accent
- ✅ Home page with instance overview and status
- ✅ Navigation for 10 pages: Home, Play, Instances, Versions, Mods, Servers, Worlds, Cosmetics, News, Settings
- ✅ Mojang version manifest fetching (structure complete)
- ✅ Java runtime detection (structure complete)
- ✅ Instance CRUD with SQLite persistence (structure complete)
- ✅ Settings persistence (JSON)
- 🔄 Download system with progress UI (UI exists, backend partially implemented)
- 🔄 Game launch with safe argument construction (structure exists)
- ⬜ Full download pipeline (client, libraries, assets, natives)
- ⬜ Checksum verification before extraction/launch
- ⬜ Play page with instance selection
- ⬜ Functional Instances, Versions, Settings pages

### Later Releases
- Microsoft account authentication (OAuth PKCE)
- PK Launcher API integration
- Forge / Fabric / Quilt mod loader support
- Mod and resource pack management
- Cosmetics (skins, capes)
- Signed application updates
- Platform installers (AppImage, NSIS, DMG)

### Non-Goals
- Distributing Minecraft game assets
- Cheat or exploit features
- Bypassing Microsoft/Mojang authentication

## Technical Context

### Build System
```bash
# Configure
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON

# Build
cmake --build . --parallel

# Test
ctest --output-on-failure
```

### Dependencies
| Dependency | Purpose |
|-----------|---------|
| Qt 6.2+ Core | Foundation, events, I/O |
| Qt 6 Widgets | Desktop UI |
| Qt 6 Network | HTTP requests (Mojang API, updates) |
| Qt 6 SQL | SQLite persistence |
| Qt 6 Concurrent | Background downloads |
| Qt 6 Test | Unit/integration tests |
| OpenSSL | HTTPS support |

### External APIs
| API | Purpose | Status |
|-----|---------|--------|
| Mojang Version Manifest | `https://launchermeta.mojang.com/mc/game/version_manifest_v2.json` | Implemented (fetch + cache) |
| Mojang Version Details | Per-version JSON URLs from manifest | Implemented (fetch + parse) |
| PK Launcher Backend | `http://localhost:3001/api/v1` | Auth endpoints stubbed |
| PK Launcher Updates | `https://releases.pklauncher.dev/update.json` | Update check implemented |

### Data Storage
| Storage | Format | Location | Purpose |
|---------|--------|----------|---------|
| Settings | JSON | `~/.config/PkLauncher/settings.json` | App preferences |
| Auth tokens | INI | `~/.config/PkLauncher/auth.ini` | Access/refresh tokens |
| Instances | SQLite | `~/.local/share/PkLauncher/instances.db` | Instance metadata |
| Game files | Directories | `~/.local/share/PkLauncher/instances/` | Per-instance game data |

## Current Implementation State

### Fully Implemented
- `Settings` singleton with JSON load/save, all config fields
- `InstanceManager` with SQLite CRUD (create, list, get, update, delete)
- `LauncherCore` initialization, Mojang manifest fetch/parse, version detail fetch
- `LauncherCore` Java detection (JAVA_HOME, common paths, PATH)
- `LauncherCore` JVM argument building (legacy + modern format)
- `LauncherCore` Game argument building (legacy + modern format)
- `LauncherCore` Process launch with QProcess
- `MainWindow` with sidebar navigation, dark theme, status bar, system tray
- `HomePage` fully built with hero section, play button, telemetry cards
- `DownloadManager` UI with progress tracking
- `AuthManager` API calls (login, register, logout, refresh, token storage)
- `UpdateManager` update check and download with SHA-256 verification

### Partially Implemented (TODO in code)
- `LauncherCore::downloadClientJar` — Structure exists, actual download not implemented
- `LauncherCore::downloadLibraries` — Placeholder, needs download + filtering
- `LauncherCore::downloadAssets` — Placeholder, needs asset index + object download
- `LauncherCore::getLatestVersions` — Returns empty pair, needs manifest latest parsing
- `LauncherCore::extractNatives` — Uses `jar` command, needs proper extraction
- `LauncherCore::downloadVersion` — Orchestrator exists but calls TODO methods
- `AuthManager::microsoftOAuth` — Empty TODO

### Stub Pages (Header + label only)
- `PlayPage` — Has constructor, just shows a label
- `InstancesPage` — Stub
- `VersionsPage` — Stub
- `ModsPage` — Stub
- `ServersPage` — Stub
- `WorldsPage` — Stub
- `CosmeticsPage` — Stub
- `NewsPage` — Stub
- `SettingsPage` — Stub

### Empty Directories (Reserved)
- `src/network/` — For future API client classes
- `src/utils/` — For future utility classes

## What Does NOT Exist

These are explicitly NOT in scope for the current codebase:
- No mod loader integration (Forge, Fabric, Quilt)
- No real authentication backend
- No Microsoft OAuth implementation
- No file integrity verification pipeline
- No native library extraction for modern versions
- No asset downloading pipeline
- No library download pipeline
- No CI/CD configuration
- No packaging/installer scripts
- No localization system
- No update notification UI
- No crash reporting

## Project Files

### Documentation
| File | Purpose |
|------|---------|
| `AGENTS.md` | Master rules for all agents |
| `PROJECT.md` | This file |
| `ARCHITECTURE.md` | System architecture |
| `TASKS.md` | Task tracking |
| `STATUS.md` | Current state |
| `COMMUNICATION.md` | Agent communication |
| `DECISIONS.md` | Decision log |
| `PRD.md` | Product requirements |
| `Architecture.md` | Original architecture doc |
| `Phases.md` | Development phases |
| `Design.md` | UI design guidelines |
| `Rules.md` | Engineering rules |

### Source Code
| Directory | Contents |
|-----------|----------|
| `src/core/` | Application, Settings, AuthManager, UpdateManager |
| `src/launcher/` | LauncherCore, InstanceManager |
| `src/ui/` | MainWindow, DownloadManager, page implementations |
| `include/` | All header files (mirrors `src/` structure) |
| `tests/` | Qt Test files + CMakeLists.txt |
| `resources/` | Qt resource files, images, fonts |
