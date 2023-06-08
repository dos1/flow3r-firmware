from st3m.application import Application,menu
from . import menu_main
class myApp(Application):
    def on_init(self):
        self.appmenu = menu_main.get_menu()

    def on_foreground(self):
        menu.set_active_menu(self.appmenu)

    def main_foreground(self):
        menu.render()

app=myApp("flow3r")
