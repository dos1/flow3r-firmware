import menu
import event
import hardware
import control

import demo_worms,demo_sparabo,cap_touch_demo, melodic_demo, harmonic_demo
import menu_settings,menu_tinysynth

import time

hardware.captouch_autocalib()

def start_worms(action):
    menu.menu_stack.append(menu.active_menu)
    menu.active_menu=None
    demo_worms.run()

def start_sparabo(action):
    menu.menu_stack.append(menu.active_menu)
    menu.active_menu=None
    demo_sparabo.run()

def start_captouch(action):
    armed = False
    while True:
        cap_touch_demo.run()
        time.sleep_ms(10)
        if hardware.get_button(1) == 0:
            armed = True
        if armed and hardware.get_button(1) == 2:
            break

def start_melodic(action):
    armed = False
    melodic_demo.init()
    hardware.set_global_volume_dB(20)
    hardware.display_fill(0)
    while True:
        melodic_demo.run()
        time.sleep_ms(10)
        if hardware.get_button(1) == 0:
            armed = True
        if armed and hardware.get_button(1) == 2:
            break

def start_harmonic(action):
    armed = False
    harmonic_demo.init()
    hardware.set_global_volume_dB(20)
    hardware.display_fill(0)
    while True:
        harmonic_demo.run()
        time.sleep_ms(10)
        if hardware.get_button(1) == 0:
            armed = True
        if armed and hardware.get_button(1) == 2:
            break
menu_demo = menu.Menu("demo")
item_worms = menu.MenuItem("worms")
item_worms.action = start_worms
menu_demo.add(item_worms)

item_abo = menu.MenuItem("abo")
item_abo.action = start_sparabo
menu_demo.add(item_abo)

item_cap = menu.MenuItem("captouch")
item_cap.action = start_captouch
menu_demo.add(item_cap)

menu_demo.add(menu.MenuItem("melodic", action=start_melodic))
menu_demo.add(menu.MenuItem("harmonic", action=start_harmonic))

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
