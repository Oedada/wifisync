from typing import Callable
from PyQt6.QtWidgets import QPushButton

def get_btn(text: str, callback: Callable):
    btn = QPushButton(text=text)
    btn.clicked.connect(callback)
    return btn
