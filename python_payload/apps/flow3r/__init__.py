from st3m import logging

log = logging.Log(__name__, level=logging.INFO)
log.info(f"running {__name__}")
from st3m.application import Application

log.info(f"import app")
from . import menu_main

log.info(f"import menu")


class myApp(Application):
    def on_init(self):
        self.menu = menu_main.get_menu()

    def on_foreground(self):
        self.menu.start()


app = myApp("flow3r", exit_on_menu_enter=False)
