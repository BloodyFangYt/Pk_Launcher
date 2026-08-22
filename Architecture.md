# PkLauncher Architecture

## Technology

| Area | Choice |
| --- | --- |
| Desktop application | C++17 |
| UI | Qt 6 Widgets |
| Build | CMake |
| Networking | Qt Network |
| Local data | SQLite through Qt SQL |
| Concurrency | Qt Concurrent and worker objects |
| Tests | Qt Test |

## Boundaries

The desktop process owns the UI, local instance configuration, downloads, Java
detection, and Minecraft process lifecycle. Network access is isolated in
clients under `src/network`; widgets do not construct raw HTTP requests.

```text
Qt UI
  ├── Application services
  │     ├── Instance manager
  │     ├── Java detector
  │     ├── Download manager
  │     └── Launch manager
  ├── Network clients
  │     ├── Mojang manifest client
  │     └── PK Launcher API client
  └── Persistence
        ├── Settings
        └── SQLite instance metadata
```

## Security

- Validate all URLs, paths, versions, and downloaded metadata.
- Verify SHA-1/SHA-256 checksums before using downloaded files.
- Build process arguments as a list; never pass untrusted shell strings.
- Store tokens using the platform keychain, not plain-text settings.
- Keep Qt WebEngine disabled unless a feature requires it.
