# PkLauncher Product Requirements

## Product

PkLauncher is a native cross-platform Minecraft launcher for Windows, macOS,
and Linux. It provides a fast desktop experience for managing Minecraft
versions and local instances without bundling proprietary game files.

## MVP requirements

### Desktop shell

- Native Qt 6 interface with dark theme and red accent.
- Home page showing the selected instance and launcher status.
- Navigation for Home, Play, Instances, Versions, Mods, Servers, Worlds,
  Cosmetics, News, and Settings.

### Minecraft lifecycle

- Retrieve the official Mojang version manifest.
- Detect compatible Java installations.
- Download client, libraries, assets, and natives with progress reporting.
- Verify checksums before extraction or launch.
- Launch selected instances with safely constructed arguments.

### Local management

- Create, edit, select, and remove instances.
- Persist settings and instance metadata locally.
- Configure RAM and JVM arguments.
- Display process output and crash logs.

## Later releases

- Microsoft account authentication and secure credential storage.
- Forge, Fabric, and Quilt profiles.
- Mod and resource-pack management.
- Optional PK Launcher account, cosmetics, and community services.
- Signed application updates and platform installers.

## Non-goals

The product will not distribute Minecraft assets, implement cheat features,
or bypass Microsoft/Mojang authentication and licensing requirements.
