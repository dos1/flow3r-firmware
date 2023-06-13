import math

from st3m import logging, event, ui

log = logging.Log(__name__, level=logging.INFO)
log.info(f"running {__name__}")

from st3m.application import Application
from . import menu_main


class myApp(Application):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.event_input_overlay = event.Event(
            name="show input overlay",
            group_id="input-overlay",
            # condition=lambda d: d['type'] in ["captouch","button"],
            condition=lambda d: d["type"] in ["captouch"] and d["value"] == 1,
            action=self.update_input_data,
            enabled=False,
        )
        self.ui_input = ui.Viewport()
        for i in range(2):
            self.ui_input.add(ui.IconLabel(label="", size=25))
        self.set_input_overlay(False)

    def on_init(self):
        self.menu = menu_main.get_menu(self)

    def on_foreground(self):
        self.menu.start()

    def update_input_data(self, data):
        # label = self.ui_input.children[2]
        phi = data["angle"] / 600
        # deg = data["angle"]/(2*3.14)*180

        idx = data["index"]
        r = -data["radius"] / 600

        angle = math.atan2(r, phi)
        deg = int(angle / math.pi * 180)
        (x, y) = (int(r), int(phi))  # ui.xy_from_polar(r, phi)

        p1 = self.ui_input.children[0]
        p1.label = f"{idx} {(x,y)}"
        p1.origin = (x, y)

        p2 = self.ui_input.children[1]
        p2.label = f"{idx} {deg}"
        p2.origin = ui.xy_from_polar(110, angle)
        # my_petal.s = f"{idx}: phi{phi:1} r{r:1}"

    #        label.label = f"{idx}: {int(x)},{int(y)} r{int(r)} phi{int(phi)}"
    # label.label = f"{idx}: {int(x)},{int(y)} angle{angle:1}"

    def set_input_overlay(self, value):
        self.event_input_overlay.set_enabled(value)
        if value:
            event.the_engine.overlay = self.ui_input
        else:
            event.the_engine.overlay = None

    def get_input_overlay(self):
        return self.event_input_overlay.enabled


app = myApp("flow3r", exit_on_menu_enter=False)
