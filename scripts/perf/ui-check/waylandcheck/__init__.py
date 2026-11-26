# SPDX-License-Identifier: GPL-3.0-or-later
"""Records how windows, popups and floating dockers appear on the Qt platform in use.

run-ui-check.sh installs this plugin into a fresh data directory and enables
it. After the main window shows, the plugin goes through the states below and
appends one JSON line per state to $LIBREPAINT_UI_CHECK_OUT. Each state is
held for HOLD_MS so the runner can capture the screen while it is shown; the
runner reads the "ready" line of a state, captures, and the plugin moves on.

States: start (main window), popup (brush preset chooser), docker (Layers
docker floating), window (a second main window), done.
"""

import json
import os
import time

from krita import Extension, Krita

try:
    from PyQt6.QtCore import QTimer, Qt
    from PyQt6.QtGui import QGuiApplication
    from PyQt6.QtWidgets import QApplication, QWidget
except ImportError:  # PyQt5 builds
    from PyQt5.QtCore import QTimer, Qt
    from PyQt5.QtGui import QGuiApplication
    from PyQt5.QtWidgets import QApplication, QWidget

START_MS = 6000
HOLD_MS = 5000


def rect(widget):
    """The widget's rectangle in Qt's global coordinates, or None."""
    if widget is None:
        return None
    top_left = widget.mapToGlobal(widget.rect().topLeft())
    return [top_left.x(), top_left.y(), widget.width(), widget.height()]


def visible_popups():
    popups = []
    for widget in QApplication.topLevelWidgets():
        flags = widget.windowFlags()
        if widget.isVisible() and (flags & Qt.WindowType.Popup) == Qt.WindowType.Popup:
            popups.append({"class": type(widget).__name__, "object": widget.objectName(), "rect": rect(widget)})
    return popups


class WaylandCheck(Extension):
    def __init__(self, parent):
        super().__init__(parent)
        self.out = os.environ.get("LIBREPAINT_UI_CHECK_OUT")
        self.steps = [self.start, self.popup, self.docker, self.window, self.done]

    def setup(self):
        if self.out:
            QTimer.singleShot(START_MS, self.next)

    def createActions(self, window):
        pass

    def write(self, state, **values):
        values.update({"state": state, "time": time.time(),
                       "platform": QGuiApplication.platformName()})
        with open(self.out, "a") as stream:
            stream.write(json.dumps(values) + "\n")

    def next(self):
        if self.steps:
            step = self.steps.pop(0)
            try:
                step()
            except Exception as error:  # record and go on, so one failure shows the others
                self.write(step.__name__, error=repr(error))
            QTimer.singleShot(HOLD_MS, self.next)

    def main_window(self):
        window = Krita.instance().activeWindow()
        return window.qwindow() if window else None

    def start(self):
        main = self.main_window()
        screen = QGuiApplication.primaryScreen().availableGeometry()
        self.write("start", main=rect(main), visible=bool(main and main.isVisible()),
                   maximized=bool(main and main.isMaximized()),
                   screen=[screen.x(), screen.y(), screen.width(), screen.height()])

    def popup(self):
        main = self.main_window()
        buttons = [widget for widget in main.findChildren(QWidget)
                   if widget.toolTip() == "Choose brush preset" and widget.isVisible()]
        button = buttons[0] if buttons else None
        if button is not None:
            button.click()
        QApplication.processEvents()
        self.write("popup", button=rect(button), popups=visible_popups())

    def docker(self):
        for popup in QApplication.topLevelWidgets():
            if popup.isVisible() and (popup.windowFlags() & Qt.WindowType.Popup) == Qt.WindowType.Popup:
                popup.hide()
        dockers = [dock for dock in Krita.instance().activeWindow().dockers() if dock.objectName() == "KisLayerBox"]
        dock = dockers[0] if dockers else None
        if dock is not None:
            dock.setFloating(True)
        QApplication.processEvents()
        self.write("docker", found=dock is not None, floating=bool(dock and dock.isFloating()),
                   visible=bool(dock and dock.isVisible()), rect=rect(dock), main=rect(self.main_window()))

    def window(self):
        second = Krita.instance().openWindow()
        second.qwindow().show()
        QApplication.processEvents()
        windows = [w.qwindow() for w in Krita.instance().windows()]
        self.write("window", count=len(windows), visible=[bool(w.isVisible()) for w in windows],
                   rects=[rect(w) for w in windows])

    def done(self):
        self.write("done")


Krita.instance().addExtension(WaylandCheck(Krita.instance()))
