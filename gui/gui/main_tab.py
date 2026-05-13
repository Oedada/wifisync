from data_manager import SyncDataManager
from gui_utils import get_btn
import json
from PyQt6.QtNetwork import QNetworkAccessManager, QNetworkRequest, QNetworkReply
from PyQt6.QtCore import Qt, QUrl, QByteArray
from PyQt6.QtWidgets import (
    QGroupBox,
    QLabel,
    QListWidget,
    QMessageBox,
    QSplitter,
    QTextEdit,
    QVBoxLayout,
    QWidget,
    QListWidgetItem,
)


# =========================
# MAIN TAB
# =========================

class MainTab(QWidget):
    def __init__(self, sync_manager: SyncDataManager, http_manager: QNetworkAccessManager):
        super().__init__()

        self.sync_manager = sync_manager
        self.http_manager = http_manager
        self.accept_conn_show = False

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

        self.sync_btn = get_btn("Синхронизировать", self.sync)

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
        self.found_devices = {}

    def confirm_accept_connection(self, name: str):
        result = QMessageBox.question(
            self,
            "Запрос на подключение",
            f"Вы хотите подключиться к {name}",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )

        if result == QMessageBox.StandardButton.Yes:
            return True
        else:
            return False


    def get_devices_form_list(self) -> dict:
        devices = {}
        for i in range(self.devices_list.count()):
            item = self.devices_list.item(i)
            devices[item.text()] = item.data(Qt.ItemDataRole.UserRole)
        return devices

    def update_devices(self, devices):
        for uuid, name in devices.items():
            self.found_devices[uuid] = name

        for uuid, name in self.found_devices.items():
            if name not in self.get_devices_form_list().keys():
                item = QListWidgetItem(name)
                item.setData(Qt.ItemDataRole.UserRole, uuid)
                self.devices_list.addItem(item)

    def log_message(self, text, data=None):
        if isinstance(text, QNetworkReply.NetworkError):
            reply = self.sender()
            self.logs.append("[Error] " + str(reply.errorString()))
        else:
            self.logs.append(str(text))
            

    def sync(self, step: int = 0, data = None):
        match step:
            case 0:
                item = self.devices_list.currentItem()
                if not item:
                    QMessageBox.warning(self, "Ошибка", "Выберите устройство")
                    return
                request = QNetworkRequest(QUrl("http://127.0.0.1:5000/connect"))
                data = QByteArray()
                data.append(json.dumps({"uuid": item.data(Qt.ItemDataRole.UserRole)}).encode("UTF-8"))
                request.setHeader(QNetworkRequest.KnownHeaders.ContentTypeHeader, "application/json")
                self.log_message("Подключение к устройству...")
                reply = self.http_manager.post(request, data)
                reply.errorOccurred.connect(self.log_message)
                reply.setProperty("type", "sync")

            case 1:
                if(data["ok"]):
                    request = QNetworkRequest(QUrl("http://127.0.0.1:5000/missing_uuid"))
                    reply = self.http_manager.get(request)
                    reply.setProperty("type", "missing_uuid")
                else:
                    if(data["error"] == -2):
                        QMessageBox.warning(
                            self,
                            "Синхронизация отменена",
                            "Нет ни одного соответствующего пути для синхронизации с данным устройством"
                        )
                        self.log_message("Нет ни одного соответствующего пути для синхронизации с данным устройством")
                    else:
                        self.log_message("Отказ от другого устройства")
        
    def accepted_connection(self, data=None):
        self.accept_conn_show = False
        self.sync(1, data)

    def accept_connect_dialog(self, data = None):
        if not self.accept_conn_show:
            if data["ok"]:
                print("Yep")
                self.accept_conn_show = True
                accept = self.confirm_accept_connection(self.found_devices[data["uuid"]])
                if accept:
                    request = QNetworkRequest(QUrl("http://127.0.0.1:5000/accept_connect"))
                    send_data = QByteArray()
                    send_data.append(json.dumps({"answer": True, "uuid": data["uuid"]}).encode("UTF-8"))
                    request.setHeader(QNetworkRequest.KnownHeaders.ContentTypeHeader, "application/json")
                    self.log_message("Подключение к устройству...")
                    reply = self.http_manager.post(request, send_data)
                    reply.errorOccurred.connect(self.log_message)
                    reply.setProperty("type", "accept_connection")

    def sync_print(self):
        pass
