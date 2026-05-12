import requests
from functools import partial
from typing import Callable
from PyQt6.QtCore import QThread, QObject, pyqtSignal, pyqtSlot

from dataclasses import dataclass

@dataclass
class Signal:
    name: str
    data: object

def run_task(method: pyqtSlot, signal_name: str, callback: Callable, tasks_list: list[tuple], error_callback: Callable = None):
    thread = QThread()
    if isinstance(method, partial):
        obj = method.func.__self__
    else:
        obj = method.__self__
    print(thread)
    obj.moveToThread(thread)
    thread.started.connect(method)
    obj.register_callback(signal_name, callback)
    if error_callback is not None:
        obj.error_signal.connect(error_callback)
    tasks_list.append((thread, obj))
    thread.start()

class Worker(QObject):
    signals = pyqtSignal(Signal)
    error_signal = pyqtSignal(str)

    def __init__(self):
        super().__init__()
        self._signal_to_callback = {}
        self.signals.connect(self.callback)
        self.running = True

    def callback(self, signal: Signal):
        signal_name = signal.name
        signal_data = signal.data
        self._signal_to_callback[signal_name](signal_data)

    def register_callback(self, signal_name: str, callback: Callable):
        self._signal_to_callback[signal_name] = callback

    @pyqtSlot()
    def load_devices(self):
        while(self.running):
            try:
                r = requests.get("http://127.0.0.1:5000/devices")
                r.raise_for_status()

                self.signals.emit(Signal(name="load_devices",data=r.json()))

            except Exception as e:
                self.error_signal.emit(str(e))
            QThread.msleep(500)
    
    @pyqtSlot(str)
    def connect_request(self, uuid: str):
        try:
            r = requests.post("http://127.0.0.1:5000/connect", json={uuid})
            r.raise_for_status()
            self.signals.emit(Signal(name="connect_request",data=r.json()["ok"]))

        except Exception as e:
            self.error_signal.emit(str(e))

    def stop(self):
        self.running = False
