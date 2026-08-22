# PK Launcher — C++ Qt Edition

A modern, cross-platform Minecraft launcher built with **C++17** and **Qt 6**.

## Features

- 🎮 **Minecraft Version Management** — Fetch, download, and launch any version (release, snapshot, old_beta, old_alpha)
- ☕ **Java Runtime Management** — Auto-detect installed Java; auto-download correct JRE (Java 8/11/17/21)
- 📦 **Instance System** — Multiple profiles with isolated directories (versions, mods, configs, saves)
- ⚙️ **Settings** — RAM allocation, JVM arguments, game resolution, proxy, themes
- 🎨 **Modern Dark UI** — Custom styling with red accent, glassmorphism effects
- 🔐 **Authentication** — Email/password + Microsoft OAuth (PKCE)
- 🔄 **Auto-Updater** — Secure updates with signature verification
- 🖥️ **System Tray** — Minimize to tray, quick launch

## Requirements

- **Qt 6.2+** (Core, Widgets, Network, Sql, Concurrent)
- **C++17** compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- **CMake 3.16+**
- **OpenSSL** (for HTTPS)
- **SQLite** (bundled with Qt)

## Building

### Linux/macOS

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

### Windows (Visual Studio)

```cmd
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### With vcpkg (recommended)

```bash
vcpkg install qt6-base qt6-declarative qt6-webengine openssl sqlite3
cmake -DCMAKE_TOOLCHAIN_FILE=[vcpkg_root]/scripts/buildsystems/vcpkg.cmake ..
```

## Project Structure

```
PkLauncher/
├── CMakeLists.txt              # Build configuration
├── src/
│   ├── main.cpp                # Application entry point
│   ├── core/                   # Core application logic
│   │   ├── Application.cpp/h   # Main app coordinator
│   │   ├── Settings.cpp/h      # Configuration management
│   │   ├── AuthManager.cpp/h   # Authentication (email + Microsoft OAuth)
│   │   └── UpdateManager.cpp/h # Auto-updater
│   ├── launcher/               # Minecraft launch logic
│   │   ├── LauncherCore.cpp/h  # Version fetching, Java, downloads, launch
│   │   └── InstanceManager.cpp/h # Instance CRUD (SQLite)
│   ├── ui/                     # Qt Widgets UI
│   │   ├── MainWindow.cpp/h    # Main window with navigation
│   │   ├── DownloadManager.cpp/h # Download progress UI
│   │   └── pages/              # Individual pages
│   │       ├── HomePage.cpp/h
│   │       ├── PlayPage.cpp/h
│   │       ├── InstancesPage.cpp/h
│   │       ├── VersionsPage.cpp/h
│   │       ├── ModsPage.cpp/h
│   │       ├── ServersPage.cpp/h
│   │       ├── WorldsPage.cpp/h
│   │       ├── CosmeticsPage.cpp/h
│   │       ├── NewsPage.cpp/h
│   │       └── SettingsPage.cpp/h
│   └── utils/                  # Utility classes
├── include/                    # Public headers
│   ├── core/
│   ├── launcher/
│   └── ui/
├── resources/
│   ├── resources.qrc           # Qt resource file
│   ├── images/                 # Icons, splash screen
│   └── fonts/                  # JetBrains Mono, Inter
└── tests/                      # Unit tests
```

## Architecture

### Core Components

| Component | Responsibility |
|-----------|---------------|
| `Application` | Coordinates all subsystems, lifecycle management |
| `LauncherCore` | Minecraft version manifest, Java detection, downloads, game launch |
| `AuthManager` | User authentication, token management, Microsoft OAuth |
| `InstanceManager` | Instance CRUD, SQLite persistence |
| `UpdateManager` | Update checking, downloading, verification |

### Data Flow

```
User Action → MainWindow → Page → LauncherCore/Manager → Backend API / Mojang API
                ↓
            Signals/Slots → UI Updates (progress, status, results)
```

### Settings Storage

- **JSON** (`~/.config/PkLauncher/settings.json`) — App preferences
- **SQLite** (`~/.local/share/PkLauncher/instances.db`) — Instance data
- **INI** (`~/.config/PkLauncher/auth.ini`) — Auth tokens (encrypted)

## Development

### Code Style

- **C++17** with modern features (structured bindings, `std::optional`, `std::filesystem`)
- **Qt 6** best practices (CMake, `Q_OBJECT`, signals/slots, RAII)
- **Header/Source separation** — declarations in `include/`, implementations in `src/`
- **Forward declarations** — Minimize header dependencies

### Adding a New Page

1. Create header in `include/ui/pages/`
2. Create implementation in `src/ui/pages/`
3. Add to `MainWindow` constructor
4. Add navigation item to `m_navItems`

### Running Tests

```bash
cmake -DBUILD_TESTS=ON ..
cmake --build . --target test
```

## Configuration

### Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `PKLAUNCHER_API_URL` | Backend API base URL | `http://localhost:3001/api/v1` |
| `PKLAUNCHER_UPDATE_URL` | Update manifest URL | `https://releases.pklauncher.dev/update.json` |

### Settings File (`settings.json`)

```json
{
  "javaAutoDetect": "",
  "javaCustomPaths": [],
  "defaultJvmArgs": ["-Xmx4G", "-Xms1G", "-XX:+UseG1GC"],
  "autoUpdate": true,
  "closeOnLaunch": false,
  "showConsole": false,
  "language": "en",
  "instancesDir": "~/.local/share/PkLauncher/instances",
  "theme": "dark",
  "accentColor": "#FF0033",
  "animations": true,
  "uiScale": 1.0
}
```

## Building Installers

### Linux (AppImage)

```bash
# Requires linuxdeployqt
linuxdeployqt build/PkLauncher -appimage
```

### Windows (NSIS)

```bash
# Requires NSIS installed
cpack -G NSIS
```

### macOS (DMG)

```bash
cpack -G DragNDrop
```

## License

MIT License — see LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run `cmake --build . --target clang-format` (if available)
5. Submit a PR

## Acknowledgments

- **Mojang** — Minecraft version manifest API
- **Eclipse Temurin** — OpenJDK builds
- **Qt Project** — Excellent cross-platform framework
- **JetBrains Mono** — Beautiful monospace font
- **Inter** — Clean UI font