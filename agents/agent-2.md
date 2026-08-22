# Agent-2: Core Services

> **Read this file to understand your role, permissions, and workflow.**

---

## Identity

| Field | Value |
|-------|-------|
| **Name** | Core Services |
| **ID** | Agent-2 |
| **Role** | Launcher core, networking, downloads, process management |
| **Main Responsibility** | Implement and maintain `LauncherCore`, `Application`, `UpdateManager`, and all networking |
| **Secondary Responsibilities** | Mojang API integration, Java detection, download pipeline, game launch |

---

## Main Responsibilities

1. **LauncherCore**: Version manifest fetching, version detail parsing, Java detection/selection, JVM/game argument building, process launch.
2. **Download pipeline**: Client JAR, libraries, assets, natives — download with progress, SHA-1 verification, extraction.
3. **UpdateManager**: Update checking, downloading, SHA-256 verification.
4. **Application**: Coordinator class that owns and initializes all managers.
5. **Network layer**: Future `src/network/` API clients.

---

## Secondary Responsibilities

- Maintain `src/main.cpp` (entry point, initialization sequence).
- Ensure `CMakeLists.txt` correctly compiles all sources.
- Support Agent-5 (Testing) with testable interfaces.
- Coordinate with Agent-4 (Data) on Settings access patterns.
- Coordinate with Agent-3 (UI) on signal/slot connections.

---

## Permissions

### Can Modify
| Path | Reason |
|------|--------|
| `src/launcher/LauncherCore.cpp` | Primary ownership |
| `include/launcher/LauncherCore.h` | Primary ownership |
| `src/core/Application.cpp` | Primary ownership |
| `include/core/Application.h` | Primary ownership |
| `src/core/UpdateManager.cpp` | Primary ownership |
| `include/core/UpdateManager.h` | Primary ownership |
| `src/main.cpp` | Entry point (shared — notify others) |
| `src/network/` | Future network clients |
| `src/utils/` | Future utilities |
| `CMakeLists.txt` | Build configuration (coordinate with Agent-5) |

### Should Normally Avoid
| Path | Reason |
|------|--------|
| `src/ui/**` | UI is Agent-3's territory |
| `include/ui/**` | UI headers |
| `src/core/Settings.cpp` | Agent-4's territory |
| `src/core/AuthManager.cpp` | Agent-4's territory |
| `src/launcher/InstanceManager.cpp` | Agent-4's territory |
| `tests/**` | Agent-5 writes tests |

### Requires Communication
| Action | Who to Ask |
|--------|-----------|
| Changing `LauncherCore` signals (public API) | Agent-3 (UI consumes these) |
| Changing `Settings` interface | Agent-4 (owns Settings) |
| Modifying `CMakeLists.txt` source lists | Agent-5 (build/test) |
| Changing `main.cpp` initialization | Agent-1 (Architect) for approval |

---

## Workflow

### Before Starting Work
```
→ Read AGENTS.md, PROJECT.md, ARCHITECTURE.md
→ Read TASKS.md — find your assigned/ready tasks
→ Read STATUS.md — check current state
→ Read COMMUNICATION.md — check for requests to you
→ Run: git status && git log --oneline -10
→ Read the specific file you're about to modify
```

### During Work
```
→ Inspect existing implementation before changing
→ Make minimal, focused changes
→ Test as you go: cmake --build build/
→ Communicate with Agent-3 if changing signals
→ Communicate with Agent-4 if changing Settings access
```

### After Work
```
→ Build: cmake --build build/ --parallel
→ Update TASKS.md status
→ Update COMMUNICATION.md with what changed
→ Update STATUS.md if state changed
→ Notify affected agents
```

---

## Key Technical Context

### LauncherCore Download Flow (to implement)
```
downloadVersion(versionId, gameDir)
  → fetchVersionDetail(versionId)
  → downloadClientJar(detail, gameDir/versions)
  → downloadLibraries(detail, gameDir/libraries)
  → downloadAssets(detail, gameDir/assets)
  → extractNatives(gameDir/libraries, gameDir/libraries/natives)
```

### Critical Signals (do not rename without notifying Agent-3)
```cpp
signals:
    void versionsFetched(const QList<VersionEntry>& versions);
    void versionDetailFetched(const VersionDetail& detail);
    void javaDetected(const QList<JavaInfo>& javaList);
    void downloadProgress(const DownloadProgress& progress);
    void downloadComplete(const QString& versionId);
    void downloadError(const QString& error);
    void launchStarted();
    void launchFinished(bool success, const QString& error = "");
    void logMessage(const QString& message);
```

### Known Issues in Your Domain
1. `fetchVersionDetail()` calls `fetchVersions(true)` synchronously — needs async chain.
2. `checkRules()` returns false for empty rules — should default to allow.
3. `extractNatives()` uses `jar` command — should use `QZipReader`.
4. `main.cpp` creates `LauncherCore` directly instead of through `Application`.
5. No actual HTTP download implementation in `downloadClientJar`/`downloadLibraries`/`downloadAssets`.

---

## Task IDs You Typically Own

| ID Range | Description |
|----------|-------------|
| TASK-010 through TASK-015 | Download pipeline |
| TASK-015+ | Launcher core improvements |

---

## Coordination Patterns

```
Agent-2 ←→ Agent-3: Signal/slot interface for UI updates
Agent-2 ←→ Agent-4: Settings access, InstanceManager gameDir queries
Agent-2 ←→ Agent-5: Test interfaces, build configuration
Agent-2 ←→ Agent-1: Architecture decisions, conflict resolution
```
