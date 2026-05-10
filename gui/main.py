import sys
import json
import subprocess
from pathlib import Path

import requests
from PyQt6.QtCore import QThread, pyqtSignal, QTimer
from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import (
    QApplication,
    QFileDialog,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QListWidget,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QSplitter,
    QTabWidget,
    QTextEdit,
    QVBoxLayout,
    QWidget,
    QInputDialog,
    QTreeWidget,
    QTreeWidgetItem,
    QListWidgetItem,
)

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


# =========================
# SETTINGS TAB
# =========================

class SettingsTab(QWidget):
    def __init__(self, manager: SyncDataManager):
        super().__init__()

        self.manager = manager

        layout = QVBoxLayout(self)

        # -------------------------
        # Sync Paths
        # -------------------------

        sync_label = QLabel("Синхронизируемые папки")
        sync_label.setStyleSheet(
            "font-size: 18px; font-weight: bold;"
        )

        self.paths_tree = QTreeWidget()
        self.paths_tree.setColumnCount(2)
        self.paths_tree.setHeaderLabels(["Local path", "Remote path"])
        self.paths_tree.itemChanged.connect(self.on_path_changed)

        add_btn = QPushButton("Добавить папку")
        rm_btn = QPushButton("Удалить папку")

        add_btn.clicked.connect(self.add_sync_folder)
        rm_btn.clicked.connect(self.remove_sync_folder)

        btns = QHBoxLayout()
        btns.addWidget(add_btn)
        btns.addWidget(rm_btn)
        btns.addStretch()

        # -------------------------
        # Ignore dirs
        # -------------------------

        ignore_group = QGroupBox("Игнорируемые директории")

        ignore_layout = QVBoxLayout(ignore_group)

        self.ignore_list = QListWidget()

        add_ignore_btn = QPushButton("Добавить")
        rm_ignore_btn = QPushButton("Удалить")

        add_ignore_btn.clicked.connect(self.add_ignore)
        rm_ignore_btn.clicked.connect(self.remove_ignore)

        ignore_btns = QHBoxLayout()
        ignore_btns.addWidget(add_ignore_btn)
        ignore_btns.addWidget(rm_ignore_btn)

        ignore_layout.addWidget(self.ignore_list)
        ignore_layout.addLayout(ignore_btns)

        layout.addWidget(sync_label)
        layout.addWidget(self.paths_tree)
        layout.addLayout(btns)
        layout.addWidget(ignore_group)
        self.load_paths()
        self.load_ignores()

    def load_paths(self):
        self.paths_tree.blockSignals(True)
        devices = self.manager.load_devices()
        for dev_key, dev_val in devices.items():
            device_item = QTreeWidgetItem([dev_val["name"]])
            device_item.setData(0, Qt.ItemDataRole.UserRole, dev_key)
            self.paths_tree.addTopLevelItem(device_item)
            if not dev_val["paths"]:
                dev_val["paths"] = {}
            for path, other_path in dev_val["paths"].items():
                child = QTreeWidgetItem([path, other_path])
                device_item.addChild(child)
                child.setFlags(child.flags() | Qt.ItemFlag.ItemIsEditable)
        self.paths_tree.blockSignals(False)


    def load_ignores(self):
        for item in self.manager.load_ignores():
            self.ignore_list.addItem(item)

    def on_path_changed(self, item: QTreeWidgetItem, column):
        parent = item.parent()

        # если это не подэлемент
        if parent is None:
            return
        path = item.text(0)
        other_path = item.text(1)
        uuid = parent.data(0, Qt.ItemDataRole.UserRole)
        devices = self.manager.load_devices()
        devices[uuid]["paths"][path] = other_path
        self.manager.save_devices(devices)


    def find_top_level_by_text(self, name: str):
        for i in range(self.paths_tree.topLevelItemCount()):
            item = self.paths_tree.topLevelItem(i)

            if item.text(0) == name:
                return item

        return None

    def add_sync_folder(self):
        self.paths_tree.blockSignals(True)
        folder = QFileDialog.getExistingDirectory(
            self,
            "Выберите папку"
        )

        if not folder:
            return

        added = self.manager.add_sync_path(folder)

        if not added:
            QMessageBox.information(
                self,
                "Информация",
                "Папка уже добавлена"
            )
            return


        devices = self.manager.load_devices()

        folder = str(Path(folder).resolve())

        for dev_key, dev_data in devices.items():
            device_item = self.find_top_level_by_text(dev_data["name"])
            device_item.setData(0, Qt.ItemDataRole.UserRole, dev_key)
            if device_item is None:
                device_item = QTreeWidgetItem([folder])
                self.paths_tree.addTopLevelItem(device_item)
            other_path, ok = QInputDialog.getText(
                self,
                "Соответствующий путь",
                f'Введите путь для:\n'
                f'{dev_data["name"]}'
            )

            if ok and other_path.strip():

                child = QTreeWidgetItem([folder, other_path])
                device_item.addChild(child)
                child.setFlags(child.flags() | Qt.ItemFlag.ItemIsEditable)

                if "paths" not in dev_data:
                    dev_data["paths"] = {}

                dev_data["paths"][folder] = str(
                    Path(other_path)
                )

        self.manager.save_devices(devices)
        self.paths_tree.blockSignals(False)

    def remove_sync_folder(self):
        self.paths_tree.blockSignals(True)
        item = self.paths_tree.currentItem()

        if not item:
            return

        parent = item.parent()

        if parent is not None:
            path = item.text(0)

            self.manager.remove_sync_path(path)

            devices = self.manager.load_devices()

            for dev_key in devices.keys():
                if path in devices[dev_key].get("paths", {}):
                    del devices[dev_key]["paths"][path]

            self.manager.save_devices(devices)
            parent.removeChild(item)
        self.paths_tree.blockSignals(False)

    def add_ignore(self):
        text, ok = QInputDialog.getText(
            self,
            "Игнорируемая директория",
            "Введите имя директории:"
        )

        if not ok or not text.strip():
            return

        ignores = self.manager.load_ignores()

        self.ignore_list.addItem(text.strip())
        ignores.append(text.strip())

        self.manager.save_ignores(ignores)



    def remove_ignore(self):
        item = self.ignore_list.currentItem()

        if not item:
            return
        self.ignore_list.takeItem(
            self.ignore_list.row(item)
        )

        ignores = self.manager.load_ignores()

        if item.text() in ignores:
            ignores.remove(item.text())

        self.manager.save_ignores(ignores)


class HttpRequests(QThread):
    finished = pyqtSignal(requests.Response)

    def __init__(self):
        super().__init__()
        self.path = ""

    def run(self):
        try:
            result = requests.get("http://127.0.0.1:5000" + self.path)
            self.finished.emit(result)
        except:
            print("Не получилось, не фортануло")


# =========================
# MAIN TAB
# =========================

class MainTab(QWidget):
    def __init__(self, manager: SyncDataManager):
        super().__init__()

        self.manager = manager

        layout = QVBoxLayout(self)

        title = QLabel("Обнаруженные устройства")
        title.setStyleSheet(
            "font-size: 20px; font-weight: bold;"
        )

        splitter = QSplitter(Qt.Orientation.Horizontal)

        # LEFT

        left_widget = QWidget()
        left_layout = QVBoxLayout(left_widget)

        self.devices_list = QListWidget()

        self.sync_btn = QPushButton("Синхронизировать")

        self.sync_btn.clicked.connect(self.sync)

        left_layout.addWidget(self.devices_list)
        left_layout.addWidget(self.sync_btn)

        # RIGHT

        right_widget = QWidget()
        right_layout = QVBoxLayout(right_widget)

        conflicts_group = QGroupBox("Конфликты")

        conflicts_layout = QVBoxLayout(conflicts_group)

        self.conflicts = QTextEdit()
        self.conflicts.setReadOnly(True)

        conflicts_layout.addWidget(self.conflicts)

        logs_group = QGroupBox("Логи")

        logs_layout = QVBoxLayout(logs_group)

        self.logs = QTextEdit()
        self.logs.setReadOnly(True)

        logs_layout.addWidget(self.logs)

        right_layout.addWidget(conflicts_group)
        right_layout.addWidget(logs_group)

        splitter.addWidget(left_widget)
        splitter.addWidget(right_widget)

        splitter.setSizes([250, 700])

        layout.addWidget(title)
        layout.addWidget(splitter)

        self.http_req = HttpRequests()
        self.http_req.finished.connect(self.update_devices)
        self.timer = QTimer()
        self.timer.timeout.connect(self.req_devices)
        self.timer.start(3000)
        self.found_devices = {}

    def req_devices(self):
        if not self.http_req.isRunning():
            self.http_req.path = "/devices"
            self.http_req.start()

    def get_devices_form_list(self) -> dict:
        devices = {}
        for i in range(self.devices_list.count()):
            item = self.devices_list.item(i)
            devices[item.text()] = item.data(Qt.ItemDataRole.UserRole)
        return devices

    def update_devices(self, devices):
        for name, uuid in devices.json().items():
            self.found_devices[uuid] = name

        print(self.get_devices_form_list())
        for uuid, name in self.found_devices.items():
            if name not in self.get_devices_form_list().keys():
                item = QListWidgetItem(name)
                item.setData(Qt.ItemDataRole.UserRole, uuid)
                self.devices_list.addItem(item)

            

    def sync(self):
        item = self.devices_list.currentItem()
        self.timer.stop()
        response = requests.post(
            "http://127.0.0.1:5000/connect",
            json={
                "uuid": self.devices_list.currentItem().data(Qt.ItemDataRole.UserRole)
            }
        )
        if not item:
            QMessageBox.warning(
                self,
                "Ошибка",
                "Выберите устройство"
            )
            return

        self.logs.append(
            f"[SYNC] Синхронизация с {item.text()}"
        )
        self.logs.append(response.text)

        # Заглушка
        self.conflicts.append(
            "notes.txt -> конфликт изменений"
        )


# =========================
# MAIN WINDOW
# =========================

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.manager = SyncDataManager()

        self.setWindowTitle("Sync GUI")
        self.resize(1200, 750)

        tabs = QTabWidget()

        self.main_tab = MainTab(self.manager)
        self.settings_tab = SettingsTab(self.manager)

        tabs.addTab(self.main_tab, "Главная")
        tabs.addTab(self.settings_tab, "Настройки")

        self.setCentralWidget(tabs)

        self.setStyleSheet("""
            QWidget {
                font-size: 14px;
            }

            QPushButton {
                padding: 6px 12px;
            }

            QListWidget,
            QTextEdit,
            QTableWidget {
                border: 1px solid #444;
                border-radius: 6px;
            }

            QGroupBox {
                font-weight: bold;
                margin-top: 10px;
            }
        """)


# =========================
# ENTRYPOINT
# =========================

def main():
    app = QApplication(sys.argv)

    window = MainWindow()
    window.show()

    sys.exit(app.exec())


if __name__ == "__main__":
    main()
