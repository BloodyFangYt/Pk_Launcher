# STATUS.md — PkLauncher Current State

> **Last Updated**: 2026-08-22 09:30 UTC
> **Current Phase**: Phase 5 (real vanilla launcher milestone)

---

## Build Status

| Item | Status |
|------|--------|
| CMake configure | ✅ Passes (requires Qt 6 installed) |
| Build (no tests) | ✅ Compiles (warnings-as-errors) |
| Build (with tests) | ✅ Compiles (all targets link successfully) |
| `ctest` | ✅ 8/8 test suites pass (100+ tests) including multi-version E2E with `QT_QPA_PLATFORM=offscreen` |

---

## Implementation Status

### ✅ Fully Implemented
| Component | File(s) | Notes |
|-----------|---------|-------|
| Application coordinator | `src/core/Application.cpp` | Owns all managers, lifecycle |
| Settings singleton | `src/core/Settings.cpp` | JSON load/save, all config fields, directory creation, settingsChanged signal |
| LauncherCore init | `src/launcher/LauncherCore.cpp` | Directory setup, manifest fetch, Java detection |
| Mojang manifest fetch | `src/launcher/LauncherCore.cpp` | HTTP GET, parse, cache with 5-min TTL |
| Version detail fetch | `src/launcher/LauncherCore.cpp` | Per-version JSON, parse libraries/assets/args |
| Java detection | `src/launcher/LauncherCore.cpp` | JAVA_HOME, common paths, PATH, version parsing |
| Java selection | `src/launcher/LauncherCore.cpp` | findBestJava (exact → higher → lower fallback) |
| JVM argument builder | `src/launcher/LauncherCore.cpp` | Java 8/11/17/21 specific args |
| Game argument builder | `src/launcher/LauncherCore.cpp` | Modern + legacy format, placeholder replacement |
| Game launch | `src/launcher/LauncherCore.cpp` | QProcess, safe argument list, exit handling |
| Instance CRUD | `src/launcher/InstanceManager.cpp` | SQLite create/list/get/update/delete + input validation + updated_at fix |
| Auth API calls | `src/core/AuthManager.cpp` | Login, register, refresh, logout, token storage, onMeReply implemented |
| Microsoft OAuth | `src/core/AuthManager.cpp` | Full PKCE flow: browser → local server → Xbox Live → XSTS → Minecraft token |
| Auth UI | `src/ui/pages/AuthPage.cpp` | Login form + Microsoft OAuth button + profile display |
| Update verification | `src/core/UpdateManager.cpp` | SHA256 fix + HMAC-SHA256 signature verification |
| Instance lifecycle tests | `tests/test_instance_lifecycle.cpp` | 14 integration tests |
| Platform packaging | `CMakeLists.txt` | CPack for NSIS, DMG, AppImage |
| Update check | `src/core/UpdateManager.cpp` | Version comparison, download, SHA-256 verify |
| MainWindow | `src/ui/MainWindow.cpp` | Sidebar nav, dark theme, status bar, tray |
| HomePage | `src/ui/pages/HomePage.cpp` | Hero, play card, telemetry grid |
| DownloadManager UI | `src/ui/DownloadManager.cpp` | Per-item + overall progress |
| Unit tests (6 suites) | `tests/test_*.cpp` | Settings (13), InstanceManager (12), LauncherCore (11), DownloadPipeline (21), InstanceLifecycle (14), AuthManager (18) |

### 🔄 Partially Implemented
| Component | What's Missing |
|-----------|---------------|
| `LauncherCore::downloadClientJar()` | ✅ Implemented — HTTP download with SHA1 verification |
| `LauncherCore::downloadLibraries()` | ✅ Implemented — platform-filtered library download |
| `LauncherCore::downloadAssets()` | ✅ Implemented — asset index + object download |
| `LauncherCore::downloadVersion()` | ✅ Implemented — full orchestration pipeline |
| `LauncherCore::getLatestVersions()` | ✅ Implemented — parses manifest "latest" field |
| `LauncherCore::extractNatives()` | Uses `unzip` command — works but not robust |
| `AuthManager::microsoftOAuth()` | ✅ Implemented — full PKCE flow with Xbox/XSTS/MC token chain |

### 🔲 Stub Pages (Label Only)
| Page | Status |
|------|--------|
| PlayPage | Stub — needs instance selection + launch UI |
| InstancesPage | Stub — needs CRUD UI |
| VersionsPage | Stub — needs version browser |
| ModsPage | Stub — future phase |
| ServersPage | Stub — future phase |
| WorldsPage | Stub — future phase |
| CosmeticsPage | Stub — future phase |
| NewsPage | Stub — future phase |
| SettingsPage | Stub — needs full settings editor |

### 📂 Empty (Reserved)
| Directory | Future Purpose |
|-----------|---------------|
| `src/network/` | API client classes |
| `src/utils/` | Utility helpers |

---

## Active Tasks

| Task | Status | Assigned | Notes |
|------|--------|----------|-------|
| TASK-010 | `DONE` | Agent-2 | Client JAR download |
| TASK-011 | `DONE` | Agent-2 | Library download pipeline |
| TASK-012 | `DONE` | Agent-2 | Asset download pipeline |
| TASK-013 | `DONE` | Agent-2 | Native extraction |
| TASK-014 | `DONE` | Agent-2 | Download orchestration |
| TASK-015 | `DONE` | Agent-2 | Manifest latest release/snapshot parsing |
| TASK-040 | `DONE` | Agent-5 | Download pipeline integration tests (13 tests) |
| TASK-020 | `DONE` | Agent-1 | Play page UI implemented with instance selection, launch, progress, logs |
| TASK-021 | `DONE` | Agent-1 | Instances page CRUD UI with create/edit/delete dialogs |
| TASK-022 | `DONE` | Agent-1 | Versions page with search, filter, download buttons |
| TASK-023 | `DONE` | Agent-1 | Settings page already fully implemented |
| TASK-053 | `DONE` | Agent-5 | E2E mocked tests (12 new), 3 pre-existing fixes, 85/85 pass |

## Blocked Tasks

None. Headless test execution requires `QT_QPA_PLATFORM=offscreen`.

---

## Known Issues

1. **`DownloadManager::onDownloadProgress` variable shadowing** — Parameter `progress` shadows type name. Works but should be cleaned up.
2. **No `src/network/` implementation** — All HTTP done directly in LauncherCore and AuthManager. Should be extracted.

---

## Recent Changes

| Date | Change | By |
|------|--------|----|
| 2026-08-22 | Multi-version E2E test (a1.0.15 through 26.2), fixed checkRules OS mapping + features + missing placeholders | Agent-5 |
| 2026-08-22 | TASK-053: E2E mocked tests (12 new), fixed 3 pre-existing test failures, 100% pass rate | Agent-5 |
| 2026-08-22 | TASK-052: Auth hardening — isLoggedIn fix, profile persistence, error surfacing, 18 auth tests | Agent-4 |
| 2026-08-22 | Settings/InstanceManager bug fixes + validation + new tests | Agent-4 |
| 2026-08-22 | AuthManager::onMeReply linker error fixed | Agent-4 |
| 2026-08-22 | Auth UI, Update signature verification, Microsoft OAuth fixes | Agent-4 |
| 2026-08-22 | Auth UI page, instance lifecycle tests, platform packaging | Agent-4 |
| 2026-08-22 | Microsoft OAuth PKCE flow — browser + local server + Xbox/XSTS/MC token chain | Agent-4 |
| 2026-08-22 | Download pipeline integration tests — 13 tests, mock HTTP server (asset download test added) | Agent-5 |
| 2026-08-22 | Download pipeline TASK-010–015 implemented and version-detail fetch sequencing fixed | Agent-2 |
| 2026-08-22 | LauncherCore launch classpath, non-blocking native extraction, and Java detection fixes | Agent-2 |
| 2026-08-22 | Phase 0 test infrastructure added (TASK-001–004) | @Buffy |
| 2026-08-22 | Qt/C++ pivot — all legacy TS/Rust code removed | Owner |
| 2026-08-22 | Documentation migrated to Qt/C++ | Multiple agents |
| 2026-08-22 | Architect documentation consistency review complete — no discrepancies | Agent-1 |

---

## Next Priorities (Recommended Order)

1. **TASK-050**: Complete and manually verify vanilla download-to-launch flow
2. **TASK-051**: Polish vanilla launcher UX
3. **TASK-052**: Harden account and instance integration
4. ~~**TASK-053**: Run end-to-end vanilla validation~~ ✅ DONE
5. **TASK-054**: Coordinate milestone integration and documentation

Modrinth and mod-loader support remain a later phase.

---

## Test Coverage

| Module | Test File | Tests | Pass | Fail | Notes |
|--------|-----------|-------|------|------|-------|
| Settings | `tests/test_settings.cpp` | 11 | 11 | 0 | All pass |
| InstanceManager | `tests/test_instance_manager.cpp` | 12 | 12 | 0 | All pass |
| LauncherCore | `tests/test_launcher_core.cpp` | 9 | 9 | 0 | All pass (Java regex + G1GC fixes) |
| DownloadPipeline | `tests/test_download_pipeline.cpp` | 19 | 19 | 0 | 13 original + 6 E2E mocked (TASK-053) |
| InstanceLifecycle | `tests/test_instance_lifecycle.cpp` | 18 | 18 | 0 | 12 original + 6 launch config (TASK-053) |
| AuthManager | `tests/test_auth_manager.cpp` | 16 | 16 | 0 | Offline login + state tests |
| E2E Real | `tests/test_e2e_real.cpp` | 7 | 7 | 0 | Real Mojang API: Java detect, manifest, version, client JAR, 131 libs, launch |
| **All Versions** | `tests/test_all_versions.cpp` | 9 | 9 | 0 | **a1.0.15, b1.7.3, 1.5.2, 1.8.9, 1.12.2, 1.16.5, 1.20.4, 26.2, 24w14a — download + SHA1 + launch** |
| **Total** | | **101** | **101** | **0** | **100% pass rate** |
| MainWindow | — | No tests | — | — | UI, manual verification |
| UpdateManager | — | No tests | — | — | Requires network |
| DownloadManager | — | No tests | — | — | UI, manual verification |
| HomePage | — | No tests | — | — | UI, manual verification |
