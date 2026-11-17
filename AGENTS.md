# NeruLink Development Guide

## Project Overview

P2P file synchronization system written in C++17 using mDNS discovery and Avahi for peer detection on LAN networks. Supports Linux (via Avahi), targets Windows and Android for future versions.

## Build System

- **Build tool**: CMake
- **C++ standard**: 17 (`set(CMAKE_CXX_STANDARD 17)` with `REQUIRED`)
- **Compiler warnings**: `-Wall -Wextra` enabled in build config
- **Platform requirement**: Linux-only for V1; Avahi dependency required

### Build Commands

```bash
cd <project-root>
mkdir build && cd build
cmake ..
make
# Binary outputs to bin/nerulink
```

## Project Structure

```
NeruLink/
├── bin/           # Generated executables (build output)
├── build/         # CMake build directory
├── include/       # Header files (public API)
│   └── neru/      # Project headers
├── src/           # Source files
│   ├── main.cpp              # Main entry, handles signal safety + discovery callbacks
│   └── discovery/            # mDNS discovery implementation
│       ├── discovery_common.cpp     (empty stub)
│       └── discovery_linux.cpp      # Avahi backend (Linux only)
├── docs/          # Architecture and scope documentation
└── script/        # Utility scripts (TODO)
```

## Key Implementation Details

### Signal Safety Pattern

The `main()` function blocks `SIGINT` and `SIGTERM` before spawning the discovery worker thread. The signal handler (`sigwait`) runs in the main thread, while `discovery->start()` blocks indefinitely on its own thread. If that thread exits early (error), it sends `SIGTERM` to wake up the main loop.

### Discovery Flow

1. Create discovery service via `createDiscoveryService()` → returns Avahi client wrapper
2. Set callbacks: `OnPeerFound`, `OnPeerLost`, `OnError` with mutex-guarded stdout
3. Worker thread calls `discovery->start()` which blocks on Avahi browser loop
4. Main thread waits on `sigwait` for interrupt signals

### Service Type

Looking for peers on service `_miku._tcp` (defined in main.cpp).

## Architecture Decisions (from docs/)

- **Connection model**: P2P with optional "Hybrid" mode (single pivot device)
- **Discovery**: mDNS via Avahi (Linux), QR pairing planned
- **Connection scope**: LAN only for V1; Internet support deferred
- **Conflict resolution**: Last-Write-Wins (LWW) for V1
- **Change detection**: File-watching (not polling) to save CPU on battery devices
- **File transfer**: Rsync algorithm
- **Encryption in transit**: TLS for network transfers
- **Encryption at rest**: Optional DEK/KEK system
- **Backups**: Basic backup system planned

## Constraints & Omitted Features

Do not implement:
- Internet/wide-area connections (V1 scope)
- File versioning (user-deferred)
- Large file support > 20GB
- Polling-based change detection

## Build Prerequisites

Before running CMake, ensure system dependencies are installed:

```bash
# Debian/Ubuntu
sudo apt-get install cmake build-essential libavahi-client-dev

# Fedora
sudo dnf install cmake avahi-devel
```

## Testing Notes

Test directory exists (`test/`) but is currently empty. Before running tests:
1. Ensure `bin/nerulink` binary exists and is executable
2. Run multiple instances on same LAN to verify peer discovery works
3. Check that signal handling works properly (Ctrl+C exits cleanly)

## Platform Support

Current version:
- ✅ Linux (requires Avahi)

Future versions (deferred):
- Windows
- Android
