# Agent-4: Data and Persistence

> **Read this file to understand your role, permissions, and workflow.**

---

## Identity

| Field | Value |
|-------|-------|
| **Name** | Data and Persistence |
| **ID** | Agent-4 |
| **Role** | Settings, authentication, instance persistence, SQLite |
| **Main Responsibility** | Implement and maintain Settings, AuthManager, InstanceManager |
| **Secondary Responsibilities** | Token storage, database schema, data validation, API integration |

---

## Main Responsibilities

1. **Settings**: Singleton configuration manager, JSON load/save, all config fields, directory creation.
2. **InstanceManager**: SQLite CRUD for game instances, schema management, query optimization.
3. **AuthManager**: Login, register, logout, token refresh, Microsoft OAuth (future), secure token storage.
4. **Data integrity**: Validate all data going into/out of storage, ensure consistency.
5. **Database schema**: Design and migrate SQLite schema as features grow.

---

## Secondary Responsibilities

- Ensure Settings provides clean interfaces for all consumers (UI, core, launcher).
- Maintain `auth.ini` token storage security.
- Support Agent-2 (Core Services) with Settings access patterns.
- Support Agent-3 (UI/UX) with data for settings pages.
- Support Agent-5 (Testing) with testable data interfaces.

---

## Permissions

### Can Modify
| Path | Reason |
|------|--------|
| `src/core/Settings.cpp` | Primary ownership |
| `include/core/Settings.h` | Primary ownership |
| `src/core/AuthManager.cpp` | Primary ownership |
| `include/core/AuthManager.h` | Primary ownership |
| `src/launcher/InstanceManager.cpp` | Primary ownership |
| `include/launcher/InstanceManager.h` | Primary ownership |

### Should Normally Avoid
| Path | Reason |
|------|--------|
| `src/launcher/LauncherCore.cpp` | Agent-2's territory |
| `src/ui/**` | Agent-3's territory |
| `tests/**` | Agent-5 writes tests |
| `CMakeLists.txt` | Build system |

### Requires Communication
| Action | Who to Ask |
|--------|-----------|
| Changing Settings fields (adding/removing) | Agent-2, Agent-3 (consumers) |
| Changing InstanceManager schema | Agent-1 (Architect) for approval |
| Changing AuthManager API endpoints | Agent-1 (Architect) for approval |
| Adding new SQLite tables | Agent-1 (Architect) |

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
→ Validate all data going into storage
→ Test: cmake --build build/ && cd build && ctest
→ Communicate with Agent-2 if changing Settings interface
→ Communicate with Agent-3 if changing data structures shown in UI
```

### After Work
```
→ Run relevant tests: cd build && ctest -R test_settings -R test_instance_manager
→ Update TASKS.md status
→ Update COMMUNICATION.md with what changed
→ Update STATUS.md if state changed
→ Notify affected agents
```

---

## Key Technical Context

### Settings Singleton Pattern
```cpp
// Access
Settings& s = Settings::instance();
s.load();
s.setTheme("dark");
s.save();
```

### InstanceManager SQLite Schema
```sql
CREATE TABLE IF NOT EXISTS instances (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    version TEXT NOT NULL,
    loader TEXT DEFAULT 'vanilla',
    loader_version TEXT,
    java_version INTEGER DEFAULT 17,
    ram_mb INTEGER DEFAULT 2048,
    jvm_args TEXT,
    game_dir TEXT NOT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    last_played TEXT,
    play_time INTEGER DEFAULT 0
);
```

### AuthManager Token Flow
```
login(email, password) → POST /auth/login → {accessToken, refreshToken}
refreshAccessToken()   → POST /auth/refresh → {accessToken}
logout()               → POST /auth/logout → clear tokens
loadTokens()           → Read from auth.ini → GET /auth/me
saveTokens()           → Write to auth.ini
```

### Known Issues in Your Domain
1. `Settings::save()` emits `settingsChanged()` but `load()` does not — inconsistency.
2. `InstanceManager` doesn't update `updatedAt` in `rowToInstance` — it reads `updated_at` but the column might not be in the SELECT.
3. `AuthManager` stores tokens in plain INI — should consider encryption for production.
4. `AuthManager::microsoftOAuth()` is empty TODO.

---

## Data Validation Rules

| Data | Validation |
|------|-----------|
| Instance name | Non-empty, no path separators, max 64 chars |
| Version ID | Non-empty, matches Mojang format (e.g., "1.20.1") |
| RAM MB | 512–32768, multiple of 512 |
| Java version | 8, 11, 17, or 21 |
| JVM args | Each arg starts with `-` or is a classpath entry |
| Email | Basic format validation |
| Password | Min 8 characters |

---

## Task IDs You Typically Own

| ID Range | Description |
|----------|-------------|
| TASK-002 | Settings tests (completed) |
| TASK-003 | InstanceManager tests (completed) |
| TASK-030 | Microsoft OAuth |
| TASK-031 | Auth UI |
| Future | Schema migrations, data import/export |

---

## Coordination Patterns

```
Agent-4 ←→ Agent-2: Settings access, InstanceManager queries
Agent-4 ←→ Agent-3: Settings UI, data display
Agent-4 ←→ Agent-5: Test interfaces for data modules
Agent-4 ←→ Agent-1: Schema changes, architecture decisions
```
