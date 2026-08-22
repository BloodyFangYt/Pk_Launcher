# PkLauncher Engineering Rules

## Required stack

- C++17 or newer with warnings enabled
- Qt 6 Widgets, Network, SQL, and Concurrent
- CMake for configuration and packaging
- Qt Test for automated tests

## Code quality

- Keep headers small and ownership explicit with RAII.
- Prefer value types, `QString`, `QByteArray`, and Qt containers at Qt
  boundaries.
- Do not use raw owning pointers or global mutable state.
- Do not use `system()`, shell concatenation, `eval`, or unchecked paths.
- Surface errors through typed results or explicit error signals; do not hide
  failures.
- Keep UI code separate from networking, persistence, and process execution.

## Security

- Validate external input and downloaded manifests.
- Verify file checksums before extraction or launch.
- Never log passwords or access tokens.
- Use platform secure storage for credentials.
- Do not add cheat, bypass, or account-abuse functionality.
