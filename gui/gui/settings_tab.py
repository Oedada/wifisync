from data_manager import SyncDataManager
from pathlib import Path

from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import (
    QFileDialog,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QListWidget,
    QMessageBox,
    QPushButton,
    QVBoxLayout,
    QWidget,
    QInputDialog,
    QTreeWidget,
    QTreeWidgetItem,
)

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
        if not devices:
            return
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

        devices = self.manager.load_devices()
        if devices is None:
                    QMessageBox.warning(
                        self,
                        "Сне удалось добавить путь",
                        "Нет ни одного знакомого устройства"
                    )
                    return
        self.manager.add_sync_path(folder)


        print(folder)
        folder = str(Path(folder).resolve())
        for dev_key, dev_data in devices.items():
            if folder not in dev_data["paths"]:
                device_item = self.find_top_level_by_text(dev_data["name"])
                if device_item is None:
                    device_item = QTreeWidgetItem([dev_data["name"]])
                    self.paths_tree.addTopLevelItem(device_item)
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

                    if "paths" not in dev_data or not dev_data["paths"]:
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
