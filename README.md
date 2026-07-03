# Wifisync
A small utility for file synchronization over a local network.
## Installing
```bash
pacman -S openssl sdl2 uv
git clone https://github.com/Oedada/wifisync.git
cd wifisync
cmake -B build -S . -G Ninja
cmake --build build
uv sync
uv run gui/gui/main.py
```
## Main features
- Finding devices in local network
- Synchronization of the corresponding paths
- Detecting conflicts

The project is still in development, the code is provided for review only.
