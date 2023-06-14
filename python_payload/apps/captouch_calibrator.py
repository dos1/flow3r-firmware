from st3m import logging, menu

log = logging.Log(__name__, level=logging.INFO)
log.info("import")

# python
import math, json

# badge23
from st3m import event, application, ui
from st3m.system import hardware


class AppCaptouchCalibrator(application.Application):
    def on_init(self):
        self.ui.background = True
        self.calib_data = {}
        for petal_index in range(10):
            pads = {}
            self.tick_counter = 0
            petal_data = {
                "pads": pads,
            }
            for pad_index in range(4):
                v = hardware.captouch_get_petal_pad_raw(petal_index, pad_index)
                # print(petal_index, pad_index, v)
                pad_data = {
                    "min": v,
                    "max": v,
                }
                pads[pad_index] = pad_data.copy()

            self.calib_data[petal_index] = petal_data.copy()

        self.ui_petals = ui.GroupPetals()
        self.ui.add(self.ui_petals)

        self.ui_petals.element_center = ui.IconLabel(
            "Touch ALL pads and exit to save", size=20
        )

        self.add_event(
            event.Event(
                name="captouch_calibrator",
                action=self.handle_captouch_event,
                condition=lambda data: data["type"] == "captouch" and data["value"],
            )
        )

    def handle_captouch_event(self, data):
        petal_index = data["index"]
        self.ui_petals.element_center.label = self.get_formatted_data(petal_index)

    def get_formatted_data(self, petal_index):
        s = f"P{petal_index}"
        for pad_index, pad_data in self.calib_data[petal_index]["pads"].items():
            print(pad_index, pad_data)
            if pad_data["min"] == pad_data["max"]:
                continue
            s += f" p{pad_index}:{pad_data['min']}-{pad_data['max']}"
        return s

    def on_foreground(self):
        hardware.captouch_autocalib()

    def _update_calib_data(self):
        for petal_index, petal_data in self.calib_data.items():
            for pad_index, pad_data in petal_data["pads"].items():
                v = hardware.captouch_get_petal_pad_raw(petal_index, pad_index)
                # print(petal_index, pad_index, v)
                # continue
                pad_data["min"] = min(pad_data["min"], v)
                pad_data["max"] = max(pad_data["max"], v)

    def on_exit(self):
        calib = json.dumps(self.calib_data)

        with open("captouch_calibration.json", "w") as f:
            f.write(calib)
            f.close()

    def tick(self):
        self._update_calib_data()
        self.tick_counter += 1
        if self.tick_counter % 50 == 0:
            for petal_index, petal_data in self.calib_data.items():
                log.info(f"Petal {petal_index}: {petal_data}")


app = AppCaptouchCalibrator("calibrate captouch")
# app.run()
