import sys
import json
from settings_tab import SettingsTab
from data_manager import SyncDataManager
from main_tab import MainTab
from functools import partial
import subprocess
import threading
from PyQt6.QtNetwork import QNetworkAccessManager, QNetworkRequest, QNetworkReply
from PyQt6.QtCore import QUrl, QTimer
from PyQt6.QtWidgets import (
    QApplication,
    QMainWindow,
    QTabWidget,
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


    def __init__(self):
        super().__init__()

        self.core = subprocess.Popen(
            ["build/app"],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1
        )
        self.sync_manager = SyncDataManager()

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
            "accept_connection": self.main_tab.accepted_connection
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

    def stop_core(self):
        self.core.kill()

# =========================
# ENTRYPOINT
# =========================





def main():
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    app.aboutToQuit.connect(window.stop_core)

    sys.exit(app.exec())


if __name__ == "__main__":
    main()
