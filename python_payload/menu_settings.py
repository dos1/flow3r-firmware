import menu
import event
import control
import ui
import hardware


ui_input = ui.Icon("")

def render_input_data(data):
    ui_input.label = str(data["angle"])
    print("xxx")
    ui_input.draw()

def set_controls_overlay(value):
    print ("set_controls_overlay")
    if value:
        event_input_overlay = event.Event(
            name="show input overlay", group_id="input-overlay",
            #condition=lambda d: d['type'] in ["captouch","button"],
            condition=lambda d: d['type'] in ["captouch"] and d['value']==1,
            action=render_input_data
        )
    else:
        print("REMOVE")
        event.the_engine.remove("input-overlay")

def set_volume(value):
    db = int(value*60-40)
    print("DB",db)
    hardware.set_global_volume_dB(db)
    

def get_menu():
    m = menu.Menu("settings")

    control_debug_input=control.ControlSwitch(
        name="show inputs",
        action=set_controls_overlay,
        default=False
    )

    item_input_overlay = menu.MenuItemControl("input overlay", control_debug_input)
    m.add(item_input_overlay)

    c = control.ControlKnob(name="Volume",default=0.5,action=set_volume)
    m.add(menu.MenuItemControl("volume",c))

    return m

m = get_menu()