# Wifisync — Agent guide

## Repo structure

Multi-language monorepo: **C++ sync core**, **Python PyQt6 GUI**, **Dart CLI**, **Flutter app**.

| Directory | Language | Role |
|-----------|----------|------|
| `src/`, `include/` | C++20 | Sync engine (LAN discovery, diff, transport, crypto) |
| `gui/gui/` | Python (PyQt6) | Desktop GUI, starts `build/app` as subprocess, HTTP polls `localhost:5000` |
| `gui/cli.py` | Python | CLI to add/remove sync paths |
| `dart/cli/` | Dart | CLI command-parser framework (WIP) |
| `dart/websockets/` | Dart | WebSocket library stub |
| `gui/flutter_app/` | Dart/Flutter | Alternative mobile/desktop GUI (WIP) |
| `external/` | C | Vendored uWebSockets + uSockets |

## Build & run

```sh
# C++ core (Ninja)
cmake -B build -S . -G Ninja
cmake --build build

# Python GUI
uv sync
uv run gui/gui/main.py

# Dart packages (each in its own dir)
dart pub get          # dart/cli/ or dart/websockets/
dart test             # dart/cli/ or dart/websockets/

# Flutter
flutter pub get       # gui/flutter_app/
```

## Codebase state (important)

- The `app` binary's entrypoint is `tests/ws/server.cpp` (uWebSockets echo server on port 9001).
- The real sync main was in `src/threads.cpp` — **commented out** along with all HTTP handler registration (port 5000 API).
- `src/sync.cpp` (the `Sync` class) is **not compiled** in `CMakeLists.txt`.
- The GUI launches `build/app` expecting an HTTP REST API on port 5000, but that requires uncommenting `src/threads.cpp`'s main and handlers.

## Architecture

- **Port 12312** — UDP broadcast for LAN device discovery (`UdpBroadcast`).
- **Port 12345** — TCP for peer-to-peer file sync.
- **Port 5000** — HTTP REST API used by PyQt6 GUI (`HTTPServer`).
- Data path resolved via `SDL_GetPrefPath("Oedada", "wifisync")` (`src/environment.cpp`).
- Session initiation via `SessionInitializer` (broadcast, connect/accept handshake).
- Full sync flow: snapshot diff → `Transport::walk` send/receive → `ChangeApplier::apply_runit`.

## Key files

| File | What |
|------|------|
| `include/constants.hpp` | All ports, protocol strings, file paths |
| `include/environment.hpp` | Data path resolution |
| `include/broadcast.hpp` | `UdpBroadcast` + `SessionInitializer` |
| `include/transport.hpp` | `SUnit`/`RUnit`/`Transport` for sync protocol |
| `include/difference.hpp` | Tree diff algorithm |
| `gui/gui/main.py` | PyQt6 entrypoint, spawns core subprocess |
| `gui/gui/api_worker.py` | Background HTTP polling helper |

## Testing

- **C++**: No test framework. `tests/ws/server.cpp` is compiled as the `app` entrypoint, not a test.
- **Python**: `tests/change_directory.py` — manual file change simulation.
- **Dart**: `dart test` in each package dir (skeleton tests only).

## Dependencies

- **System** (pacman): `openssl`, `sdl2`, `uv`
- **C++ CMake FetchContent**: cpp-httplib v0.18.3, stduuid v1.2.3
- **Vendored**: `external/uWebSockets/`, `external/uSockets/`
- **Python (uv)**: PyQt6, PySide6, requests

## Conventions

- C++20, `-Wall -Wextra -pthread`, target `app`.
- `include/` for headers, `src/` for implementations, no subdirectories.
- `compile_commands.json` is a symlink → `build/compile_commands.json`.
- `data/` is gitignored but some test fixtures are committed inside.
- No CI, no pre-commit, no linter/formatter for C++.

# Your role
Do not optimize for being agreeable.

Your job is to improve my engineering decisions.

If my proposal is flawed:
- explain why;
- provide evidence;
- suggest alternatives.

If my proposal is good:
- explain why it is good;
- identify remaining risks.

Avoid praise unless it contains technical justification.
