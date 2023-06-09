from st3m import menu,event,control

from apps import demo_worms, cap_touch_demo
#from apps import demo_sparabo, melodic_demo, harmonic_demo
from apps.flow3r import menu_settings,menu_tinysynth


import time

def get_menu():
    menu_main = menu.Menu("flow3r",has_back=False)
    menu_badge = menu.Menu("badge")
    menu_apps = menu.Menu("apps")
    menu_music = menu.Menu("music")

    #Skipping for mem constraints
    #for app_module in [demo_sparabo,melodic_demo,harmonic_demo]:
    #    menu_music.add(menu.MenuItemApp(app_module.app))

    for app_module in [demo_worms,cap_touch_demo,]:
        menu_apps.add(menu.MenuItemApp(app_module.app))

    #testmenu = menu.Menu("test")

    #item_add = menu.MenuItem("+")
    #item_add.action = lambda x: testmenu.add(menu.MenuItem("new {}".format(len(testmenu.items))))

    #item_sub = menu.MenuItem("-")
    #item_sub.action = lambda x: testmenu.pop() if len(testmenu.items) > 4 else None

    #item_foo = menu.MenuItem("foo")
    #testmenu.add(item_foo)
    #testmenu.add(item_sub)
    #testmenu.add(item_add)
    #menu_badge.add(menu.MenuItemSubmenu(testmenu))

    menu_main.add(menu.MenuItemSubmenu(menu_badge))
    menu_main.add(menu.MenuItemSubmenu(menu_apps))
    #menu_main.add(menu.MenuItemSubmenu(menu_music))
    menu_main.add(menu.MenuItemSubmenu(menu_settings.get_menu()))
    menu_main.add(menu.MenuItemSubmenu(menu_tinysynth.get_menu()))

    return menu_main

