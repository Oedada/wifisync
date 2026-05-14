import sys
import json
from settings_tab import SettingsTab
from data_manager import SyncDataManager
from main_tab import MainTab
from functools import partial
import subprocess
import threading
import time 
from PyQt6.QtNetwork import QNetworkAccessManager, QNetworkRequest, QNetworkReply
from PyQt6.QtCore import QUrl, QTimer
from PyQt6.QtWidgets import (
    QApplication,
    QMainWindow,
    QTabWidget,
    QInputDialog,
)


# =========================
# MAIN WINDOW
# =========================

class MainWindow(QMainWindow):

    def req_devices(self):
        request = QNetworkRequest(QUrl("http://127.0.0.1:5000/devices"))
        reply = self.http_manager.get(request)
        reply.setProperty("type", "devices")
        # reply.errorOccurred.connect(self.main_tab.log_message)

    def check_incoming(self):
        request = QNetworkRequest(QUrl("http://127.0.0.1:5000/incoming_connect"))
        reply = self.http_manager.get(request)
        reply.setProperty("type", "incoming_connect")
        # reply.errorOccurred.connect(self.main_tab.log_message)

    def start_timers(self):
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.req_devices)
        self.timer.timeout.connect(self.check_incoming)
        threading.Thread(target=self.read_core_output_loop, daemon=True).start()
        self.timer.start(500)

    def on_req_finished(self, reply: QNetworkReply):
        req_type = reply.property("type")
        self.req_type_to_callback[req_type](json.loads(reply.readAll().data().decode()))

    def read_core_output_loop(self):
        for line in iter(self.core.stdout.readline, ""):
            if not line:
                break
            print(line, end="", flush=True)

    def start_core(self):
        self.core = subprocess.Popen(
            ["build/app"],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1
        )
        time.sleep(0.5)

    def stop_core(self):
        self.core.kill()

    def __init__(self):
        super().__init__()
        self.start_core()
        
        self.sync_manager = SyncDataManager()
        cfg = self.sync_manager.load_config()
        if(cfg["tmp_name"]):
            name, ok = QInputDialog.getText(
                self,
                "Имя",
                'Введите имя устройства:\n'
            )
            while not ok or name == "":
                name, ok = QInputDialog.getText(
                    self,
                    "Имя",
                    'Введите имя устройства:\n'
                )
            cfg["tmp_name"] = False
            cfg["name"] = name
            with open(self.sync_manager.config_file, "w") as f:
                f.write(json.dumps(cfg))
            self.stop_core()
            time.sleep(0.5)
            self.start_core()

        self.http_manager = QNetworkAccessManager()
        self.http_manager.finished.connect(self.on_req_finished)
        
        self.setWindowTitle("Sync GUI")
        self.resize(1200, 750)

        tabs = QTabWidget()

        self.main_tab = MainTab(self.sync_manager, self.http_manager)
        self.settings_tab = SettingsTab(self.sync_manager)

        tabs.addTab(self.main_tab, "Главная")
        tabs.addTab(self.settings_tab, "Настройки")

        self.setCentralWidget(tabs)

        self.req_type_to_callback = {
            "devices": self.main_tab.update_devices,
            "sync": partial(self.main_tab.sync, 1),
            "incoming_connect": self.main_tab.accept_connect_dialog,
            "accept_connection": self.main_tab.accepted_connection,
            "missing_uuid": partial(self.main_tab.sync, 2),
            "status": self.main_tab.log_step,
        }

        self.start_timers()
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

    try:
        exit_code = app.exec()
    finally:
        window.stop_core()

    sys.exit(exit_code)


if __name__ == "__main__":
    main()
