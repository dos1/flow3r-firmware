import menu
import event
import hardware

import demo_worms,demo_sparabo

def start_worms(action):
    menu.menu_stack.append(menu.active_menu)
    menu.active_menu=None
    demo_worms.run()

def start_sparabo(action):
    menu.menu_stack.append(menu.active_menu)
    menu.active_menu=None
    demo_sparabo.run()

menu_demo = menu.Menu("demo")
item_worms = menu.MenuItem("worms")
item_worms.action = start_worms
menu_demo.add(item_worms)

item_abo = menu.MenuItem("abo")
item_abo.action = start_sparabo
menu_demo.add(item_abo)

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
menu_main.add(menu.MenuItem("nix"))

menu.set_active_menu(menu_main)
menu.render()


event.the_engine.eventloop()    
