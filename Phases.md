# PkLauncher Development Phases

## Phase 0 — Qt foundation

- [x] Replace the previous multi-runtime scaffold with CMake and Qt
- [x] Add the application, settings, launcher-core, and main-window classes
- [ ] Add a reproducible CMake build and Qt Test target

## Phase 1 — Launcher core

- [ ] Fetch and cache the Mojang version manifest
- [ ] Detect compatible Java installations
- [ ] Download libraries, assets, natives, and client jars
- [ ] Verify checksums and report progress
- [ ] Build safe process arguments and launch an instance

## Phase 2 — Instance management

- [ ] Create, edit, select, and delete local instances
- [ ] Persist metadata with SQLite
- [ ] Add RAM and JVM argument settings
- [ ] Show game logs and crash reports

## Phase 3 — Account and service integration

- [ ] Add Microsoft authentication using an external browser flow
- [ ] Add PK Launcher API integration
- [ ] Store refresh credentials in the platform keychain

## Phase 4 — Distribution and quality

- [ ] Package Windows, macOS, and Linux builds
- [ ] Add update signature verification
- [ ] Add unit, integration, and smoke tests
- [ ] Document release and support procedures
