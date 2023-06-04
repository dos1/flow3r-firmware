import menu
import event
import hardware
import control
import application

import demo_worms,demo_sparabo,cap_touch_demo, melodic_demo, harmonic_demo
import menu_settings,menu_tinysynth

import time

hardware.captouch_autocalib()
hardware.set_global_volume_dB(0)


menu_demo = menu.Menu("demo")

for app_module in [demo_worms,demo_sparabo,cap_touch_demo,melodic_demo,harmonic_demo]:
    menu_demo.add(menu.MenuItemApp(app_module.app))


testmenu = menu.Menu("test")

item_add = menu.MenuItem("+")
item_add.action = lambda x: testmenu.add(menu.MenuItem("new {}".format(len(testmenu.items))))

item_sub = menu.MenuItem("-")
item_sub.action = lambda x: testmenu.pop() if len(testmenu.items) > 4 else None

item_foo = menu.MenuItem("foo")
testmenu.add(item_foo)
testmenu.add(item_sub)
testmenu.add(item_add)

menu_main = menu.Menu("main",has_back=False)
menu_main.add(menu.MenuItemSubmenu(testmenu))
menu_main.add(menu.MenuItemSubmenu(menu_demo))
menu_main.add(menu.MenuItemSubmenu(menu_settings.get_menu()))
menu_main.add(menu.MenuItemSubmenu(menu_tinysynth.get_menu()))

menu.set_active_menu(menu_main)
menu.render()

event.the_engine.eventloop()    
