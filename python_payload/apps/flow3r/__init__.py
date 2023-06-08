from st3m.application import Application,menu
from . import menu_main
class myApp(Application):
    def on_init(self):
        self.menu = menu_main.get_menu()

    def on_foreground(self):
        menu.set_active_menu(self.menu)

app=myApp("flow3r")
