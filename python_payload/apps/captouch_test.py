from st3m import logging, menu

log = logging.Log(__name__, level=logging.INFO)
log.info("import")

# python
import math, json, cmath

# badge23
from st3m import event, application, ui
from st3m.system import hardware


class AppCaptouchTest(application.Application):
    def on_init(self):
        self.ui.background = True
        calib_json = ""
        with open("captouch_calibration.json") as f:
            calib_json = f.read()

        self.calib_data = json.loads(calib_json)

        # print(self.calib_data)
        self.ui_petals = ui.GroupPetals()
        self.ui.add(self.ui_petals)
        self.ui_label1 = ui.IconLabel(label="XY")
        self.ui_label2 = ui.IconLabel(label="phi")
        self.ui.add(self.ui_label1)
        self.ui.add(self.ui_label2)
        self.ui_petals.element_center = ui.IconLabel("Touch a pad", size=20)

        self.add_event(
            event.Event(
                name="captouch_teest",
                action=self.handle_captouch_event,
                condition=lambda data: data["type"] == "captouch" and data["value"],
            )
        )

    def handle_captouch_event(self, data):
        petal_index = data["index"]
        # self.ui_petals.element_center.label = self.get_formatted_data(petal_index)
        # self.ui_petals.element_center.label = str(self.get_relative_pads(petal_index))
        # self.ui_petals.element_center.label = str(self.get_components(petal_index))
        v = self.get_vector(petal_index)
        self.ui_petals.element_center.label = str(v)
        # print(v)
        p = (-v[0] * 120, -v[1] * 120)
        self.ui_label1.origin = p
        angle = math.atan2(p[0], p[1])
        p2 = ui.xy_from_polar(100, angle)
        self.ui_label2.origin = p2

    def get_relative_pads(self, petal_index):
        r = []
        for pad_index in range(4):
            v = hardware.captouch_get_petal_pad_raw(petal_index, pad_index)
            pad_data = self.calib_data[str(petal_index)]["pads"][str(pad_index)]
            print(v, pad_data)
            vmin = pad_data["min"]
            vmax = pad_data["max"]

            v_capped = min(vmax, (max(v, vmin)))
            if vmin == vmax:
                v_rel = -1
            else:
                v_rel = (v - vmin) / (vmax - vmin)

            r.append(round(v_rel, 2))
        return r

    def get_components(self, petal_index):
        v_rel = self.get_relative_pads(petal_index)
        # print("R:", v_rel)
        a = v_rel[1] * 0.8
        b = v_rel[2] * 0.8
        c = v_rel[3] * 0.8

        aa = a - (b + c) / 2
        bb = b - (a + c) / 2
        cc = c - (a + b) / 2

        # simpler, but also pretty good:
        # aa = a
        # bb = b
        # cc = c
        return (round(aa, 2), round(bb, 2), round(cc, 2))

    def get_vector(self, petal_index):
        components = self.get_components(petal_index)
        # components = (1.0,1.0,1.0)
        # print("C:", components)
        vector = cmath.rect(0, 0)

        for index, component in enumerate(components):
            vector += cmath.rect(component, index * (2 * cmath.pi / 3) - cmath.pi / 3)

        return (vector.real, vector.imag)

    def get_formatted_data(self, petal_index):
        s = f"P{str(petal_index)}"
        for pad_index, pad_data in self.calib_data[str(petal_index)]["pads"].items():
            # print(pad_index, pad_data)
            if pad_data["min"] == pad_data["max"]:
                continue
            s += f" p{pad_index}:{pad_data['min']}-{pad_data['max']}"
        return s

    def on_foreground(self):
        pass

    def tick(self):
        pass


app = AppCaptouchTest("test captouch")
# app.run()
