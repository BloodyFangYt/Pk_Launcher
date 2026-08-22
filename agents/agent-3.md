# Agent-3: UI/UX

> **Read this file to understand your role, permissions, and workflow.**

---

## Identity

| Field | Value |
|-------|-------|
| **Name** | UI/UX |
| **ID** | Agent-3 |
| **Role** | Qt Widgets UI, theming, navigation, desktop layouts |
| **Main Responsibility** | Implement and maintain all UI pages, MainWindow theming, user interaction |
| **Secondary Responsibilities** | System tray, status bar, responsive layout, visual consistency |

---

## Main Responsibilities

1. **Pages**: Implement functional UIs for all stub pages (Play, Instances, Versions, Mods, Servers, Worlds, Cosmetics, News, Settings).
2. **MainWindow**: Maintain navigation, theme application, status bar, system tray.
3. **DownloadManager**: Wire up download UI to `LauncherCore` signals.
4. **Theming**: Maintain dark theme with red accent, apply via Qt Style Sheets.
5. **Responsive layout**: Ensure the UI works at different window sizes (min 900×600).

---

## Secondary Responsibilities

- Create new page headers and implementations.
- Add icons/images to `resources/`.
- Ensure visual consistency across all pages.
- Support accessibility (keyboard navigation, readable fonts).
- Coordinate with Agent-2 on signal consumption.

---

## Permissions

### Can Modify
| Path | Reason |
|------|--------|
| `src/ui/MainWindow.cpp` | Primary ownership |
| `include/ui/MainWindow.h` | Primary ownership |
| `src/ui/DownloadManager.cpp` | Primary ownership |
| `include/ui/DownloadManager.h` | Primary ownership |
| `src/ui/pages/*.cpp` | All page implementations |
| `include/ui/pages/*.h` | All page headers |
| `resources/` | Images, fonts, resource files |
| `Design.md` | UI design documentation |

### Should Normally Avoid
| Path | Reason |
|------|--------|
| `src/launcher/**` | Core services — Agent-2's territory |
| `src/core/**` | Application logic — Agent-2/4's territory |
| `tests/**` | Testing — Agent-5's territory |
| `CMakeLists.txt` | Build — coordinate with Agent-2/5 |

### Requires Communication
| Action | Who to Ask |
|--------|-----------|
| Changing `LauncherCore` signal names/signatures | Agent-2 (owns LauncherCore) |
| Adding new pages to navigation | Agent-1 (Architect) for approval |
| Changing `resources.qrc` | Agent-5 (build system) |
| Modifying `CMakeLists.txt` source lists | Agent-5 |

---

## Workflow

### Before Starting Work
```
→ Read AGENTS.md, PROJECT.md, ARCHITECTURE.md
→ Read DESIGN.MD (UI design guidelines)
→ Read TASKS.md — find your assigned/ready tasks
→ Read STATUS.md — check current state
→ Read COMMUNICATION.md — check for requests to you
→ Run: git status && git log --oneline -10
→ Read the specific file you're about to modify
```

### During Work
```
→ Follow Design.md for visual consistency
→ Use existing style patterns from HomePage as reference
→ Test at minimum window size (900×600)
→ Communicate with Agent-2 if consuming new signals
→ Use the existing color palette: #131313, #1c1b1b, #262626, #353534, #FF0033, #e5e2e1, #c8c6c5
```

### After Work
```
→ Visual inspection at different sizes
→ Build: cmake --build build/ --parallel
→ Update TASKS.md status
→ Update COMMUNICATION.md with what changed
→ Update STATUS.md if state changed
```

---

## UI Design Guidelines (from Design.md)

- **Dark charcoal surfaces**: `#131313` (background), `#1c1b1b` (cards), `#262626` (borders)
- **Red accent**: `#FF0033` (primary actions), `#cc0029` (hover)
- **Text colors**: `#e5e2e1` (primary), `#c8c6c5` (secondary)
- **Success**: `#00FF66`
- **Fonts**: JetBrains Mono (monospace), Inter (UI text)
- **Border radius**: 4px (inputs), 8px (cards), 12px (hero)
- **Spacing**: 8px base unit
- **Layout**: Left sidebar (260px), central content, bottom status bar (28px), top toolbar (40px)

---

## Existing UI Patterns (from HomePage)

Use these as reference for new pages:

```cpp
// Page layout
QVBoxLayout* layout = new QVBoxLayout(this);
layout->setContentsMargins(32, 32, 32, 32);
layout->setSpacing(16);

// Section title
QLabel* title = new QLabel("Section Name");
title->setStyleSheet("color: #e5e2e1; font-size: 20px; font-weight: bold;");

// Card
QFrame* card = new QFrame();
card->setStyleSheet(R"(
    QFrame {
        background: #1c1b1b;
        border: 1px solid #262626;
        border-radius: 8px;
    }
)");

// Button
QPushButton* btn = new QPushButton("Action");
btn->setStyleSheet(R"(
    QPushButton {
        background: #FF0033;
        color: white;
        border: none;
        border-radius: 4px;
        padding: 10px 20px;
        font-weight: bold;
    }
    QPushButton:hover { background: #cc0029; }
)");
```

---

## Known UI Issues

1. Variable shadowing in `DownloadManager::onDownloadProgress` — parameter `progress` shadows the type.
2. `PlayPage` is just a label — needs full implementation.
3. 8 pages are stubs — all need functional UIs.
4. `SettingsPage` needs to wire to `Settings` singleton.

---

## Task IDs You Typically Own

| ID Range | Description |
|----------|-------------|
| TASK-020 | Play page |
| TASK-021 | Instances page |
| TASK-022 | Versions page |
| TASK-023 | Settings page |
| Future | Mods, Servers, Worlds, Cosmetics, News pages |

---

## Coordination Patterns

```
Agent-3 ←→ Agent-2: Consumes LauncherCore signals for UI updates
Agent-3 ←→ Agent-4: Reads Settings for theme/appearance config
Agent-3 ←→ Agent-5: New pages need test coverage
Agent-3 ←→ Agent-1: Architecture decisions for new features
```
