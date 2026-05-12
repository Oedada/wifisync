import json
import subprocess
from pathlib import Path


# =========================
# CONFIG
# =========================

GET_PATH_SCRIPT_PATH = "gui/get_data_path"
DETECTING_PATH = "data/detecting_units.ws"
IGNORES_PATH = "data/ignoring_units.ws"


# =========================
# DATA LAYER
# =========================

class SyncDataManager:
    def __init__(self):
        self.data_path = self.get_data_path()

    def get_data_path(self) -> Path:
        result = subprocess.run(
            [GET_PATH_SCRIPT_PATH],
            capture_output=True,
            text=True,
            check=True,
            cwd=".",
        )

        return Path(result.stdout.strip())

    @property
    def devices_file(self):
        return self.data_path / "devices.json"

    @property
    def detecting_file(self):
        return self.data_path / DETECTING_PATH

    @property
    def ignores_file(self):
        return self.data_path / IGNORES_PATH

    def load_devices(self) -> json:
        if not self.devices_file.exists():
            return {}

        with open(self.devices_file, "r") as f:
            return json.load(f)

    def save_devices(self, devices):
        with open(self.devices_file, "w") as f:
            json.dump(devices, f, indent=4)

    def load_sync_paths(self):
        if not self.detecting_file.exists():
            return []

        with open(self.detecting_file, "r") as f:
            return [
                str(Path(line.strip()).resolve())
                for line in f.readlines()
                if line.strip()
            ]

    def add_sync_path(self, path: str):
        path = str(Path(path).resolve())

        existing = self.load_sync_paths()

        if path in existing:
            return False

        with open(self.detecting_file, "a") as f:
            f.write(path + "\n")

        return True

    def remove_sync_path(self, path: str):
        path = str(Path(path).resolve())

        paths = self.load_sync_paths()

        with open(self.detecting_file, "w") as f:
            for p in paths:
                if p != path:
                    f.write(p + "\n")

    def load_ignores(self):
        if not self.ignores_file.exists():
            return []

        with open(self.ignores_file, "r") as f:
            return [
                line.strip()
                for line in f.readlines()
                if line.strip()
            ]

    def save_ignores(self, ignores):
        with open(self.ignores_file, "w") as f:
            for item in ignores:
                f.write(item + "\n")
