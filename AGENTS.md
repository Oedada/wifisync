# Wifisync — Agent guide

## Repo structure

Multi-language monorepo: **C++ sync core**, **Python PyQt6 GUI** (only on `main`/`dev`), **Dart CLI**, **Flutter app**.

| Directory | Language | Role |
|-----------|----------|------|
| `src/`, `include/` | C++20 | Sync engine (LAN discovery, diff, transport, crypto). Organized into subdirs: `core/`, `crypto/`, `fs/`, `network/`, `util/` |
| `gui/` | Python (PyQt6) | Desktop GUI, starts `build/app` as subprocess, HTTP polls `localhost:5000` |
| `dart/cli/` | Dart | CLI command-parser framework (WIP) |
| `dart/websockets/` | Dart | WebSocket library stub |
| `gui/flutter_app/` | Dart/Flutter | Alternative mobile/desktop GUI (WIP) |
| `external/` | C | Vendored uWebSockets + uSockets |

## Build & run

```sh
# C++ core (Ninja)
cmake -B build -S . -G Ninja
cmake --build build

# Python GUI (only on main/dev branches — absent in current working tree)
uv sync
uv run gui/gui/main.py

# Dart packages (each in its own dir)
dart pub get          # dart/cli/ or dart/websockets/
dart test             # dart/cli/ or dart/websockets/

# Flutter
flutter pub get       # gui/flutter_app/
```

## Codebase state (important — current branch `test-dart-websockets`)

- The `app` binary is built from `src/main.cpp` + `src/threads.cpp` + all `src/**/*.cpp` (see `CMakeLists.txt`). `tests/ws/server.cpp` no longer exists.
- **`app` currently does nothing**: `src/main.cpp`'s `main()` is commented out; `src/threads.cpp`'s `int main(){}` is **empty**. No HTTP handlers are registered and `HTTPServer::start_server` (port 5000) is never called, so the GUI's REST polling is unserved.
- `src/core/dif_work.cpp:20` is **broken mid-refactor**: `fs::path last_shapshot_path = FileAccess::;` — the tree does not compile.
- Working tree has uncommitted changes: the Python GUI (`gui/gui/*.py`, `gui/cli.py`) is **deleted** on this branch (still present in `main`/`dev`), `DataManager::get_name()` was added, and `DifWork` was switched from `env::`/file reads to `DataManager` in the constructor (but `shift_snapshots` was left unfinished).
- Last commit `b7410e0` replaced `SDL_GetPrefPath` with XDG_DATA_HOME and added two layers: `FileAccess` (low-level `get_fin`/`get_fout` in `src/fs/environment.cpp`) and `DataManager`/`DeviceManager` (config + device persistence in `src/fs/data_manager.cpp`). The migration is **incomplete**: `include/fs/environment.hpp` still includes `<SDL2/SDL.h>`, old vs new path constants coexist in `constants.hpp`, and `dif_work.cpp` still mixes `env::` and `FileAccess::`.
- `src/sync.cpp` does not exist — the old `Sync` class was split into `core/{difference,dif_work,dif_walker,transport,change_applier}`.

## Architecture

- **Port 12312** — UDP broadcast for LAN device discovery (`UdpBroadcast`), handshake strings `"Wifisync Hello Wifi:"` / `"Wifisync Connect Request:"` / `"Wifisync Connect Response:"`.
- **Port 12345** — TCP for peer-to-peer file sync. Auth flow: X25519 shared secret + Ed25519 signature of exchanged pubkeys, then `Transport::walk` sends diffs.
- **Port 5000** — HTTP REST API used by the PyQt6 GUI (`HTTPServer`, endpoints like `/devices`, `/incoming_connect`, `/status`, `/conflicts`). Not currently registered/started.
- Data path resolved via XDG_DATA_HOME (`src/fs/environment.cpp`), constants in `src/util/constants.hpp`.
- Session initiation via `SessionInitializer` (broadcast, connect/accept handshake).
- Full sync flow: snapshot diff (`DifWork::shift_snapshots` → `calculate_dif`) → `Transport::walk` send/receive → `ChangeApplier::apply_runit`, conflict detection in `Transport::walk_unit`.

## Key files

| File | What |
|------|------|
| `include/util/constants.hpp` | Ports, protocol strings, message type codes, path names (old + new) |
| `include/fs/environment.hpp` | `FileAccess::get_fin/get_fout`, XDG data dir resolution |
| `include/fs/data_manager.hpp` | `Config`/`Paths`/`Device` structs + `DataManager`/`DeviceManager` persistence |
| `include/network/broadcast.hpp` | `UdpBroadcast` + `SessionInitializer` |
| `include/core/transport.hpp` | `SUnit`/`RUnit`/`Transport` for sync protocol |
| `include/core/difference.hpp` | Tree diff algorithm (last vs current snapshot) |
| `src/threads.cpp` | `full_sync()` orchestration (currently has empty `main()`) |
| `src/main.cpp` | Old tests / entry (main commented out) |
| `gui/gui/main.py` | PyQt6 entrypoint (only on `main`/`dev`) |

## Testing

- **C++**: No test framework. `src/main.cpp` contains ad-hoc `test_*` functions (not wired into a test target).
- **Python**: `tests/change_directory.py` — manual file change simulation.
- **Dart**: `dart test` in each package dir (skeleton tests only).

## Dependencies

- **System** (pacman): `openssl`, `uv` (SDL2 was dropped in `b7410e0` but the header is still included).
- **C++ CMake FetchContent**: cpp-httplib v0.18.3, stduuid v1.2.3
- **Vendored**: `external/uWebSockets/`, `external/uSockets/`
- **Python (uv)**: PyQt6, PySide6, requests — only relevant on `main`/`dev`

## Conventions

- C++20, `-Wall -Wextra -pthread`, target `app`.
- `include/<subdir>` for headers, `src/<subdir>` for implementations (subdirs: core, crypto, fs, network, util). Header/source pairs must stay in the same subdir.
- `compile_commands.json` is a symlink → `build/compile_commands.json`.
- `data/` is gitignored but some test fixtures are committed inside.
- No CI, no pre-commit, no linter/formatter for C++.
- Active branch is `test-dart-websockets`; `main` and `dev` still contain the Python GUI.

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
