from st3m.application import Application, ApplicationContext
from st3m.input import InputState
from st3m.goose import Optional
from st3m.ui.view import ViewManager
from ctx import Context
import network
import leds
import os
import json
import math
from .k3yboard import TextInputModel, KeyboardView
from .helpers import sd_card_plugged, set_direction_leds, copy_across_devices


class WifiApp(Application):
    WIFI_CONFIG_FILE = "/flash/w1f1_config.json"
    WIFI_CONFIG_FILE_SD = "/sd/w1f1_config.json"
    SETTINGS_JSON_FILE = "/flash/settings.json"

    _scroll_pos: float = 0.0

    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self._petal_pressed = {}
        self._nearby_wlans = []
        self._status_text = "scanning"
        self._error_text = ""
        self._wlan_offset = 0
        self._is_connecting = False
        self._waiting_for_password = False
        self._password_model = TextInputModel("")

        # Use config on SD card whenever possible
        if sd_card_plugged():
            # Move config to SD card from flash if we don't have one on SD card
            if not os.path.exists(self.WIFI_CONFIG_FILE_SD) and os.path.exists(
                self.WIFI_CONFIG_FILE
            ):
                copy_across_devices(self.WIFI_CONFIG_FILE, self.WIFI_CONFIG_FILE_SD)

            # if we have both sd and flash config, remove flash config
            if os.path.exists(self.WIFI_CONFIG_FILE_SD) and os.path.exists(
                self.WIFI_CONFIG_FILE
            ):
                os.remove(self.WIFI_CONFIG_FILE)

            self.WIFI_CONFIG_FILE = self.WIFI_CONFIG_FILE_SD

        if os.path.exists(self.WIFI_CONFIG_FILE):
            with open(self.WIFI_CONFIG_FILE) as f:
                self._wifi_config = json.load(f)
        else:
            self._wifi_config = {
                "config_version": 2,
                "networks": {
                    "Example SSID": {"psk": "Example PSK"},
                    "Camp2023-open": {"psk": None},
                },
            }
            with open(self.WIFI_CONFIG_FILE, "w") as f:
                json.dump(self._wifi_config, f)

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        self._connection_timer = 10
        self._scan_timer = 0
        self._iface = network.WLAN(network.STA_IF)
        self._current_ssid = None
        self._current_psk = None
        # TODO: big error display

    def draw(self, ctx: Context) -> None:
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE
        ctx.font = ctx.get_font_name(8)

        ctx.rgb(0, 0, 0).rectangle(-120, -90, 240, 180).fill()
        ctx.rgb(0.2, 0.2, 0.2).rectangle(-120, -120, 240, 30).fill()
        ctx.rgb(0.2, 0.2, 0.2).rectangle(-120, 90, 240, 30).fill()

        ctx.font_size = 15
        current_ssid = self._iface.config("ssid")

        ctx.save()
        ctx.rgb(1, 1, 1)
        if self._iface.active():
            ctx.rgb(0, 1, 0)
        else:
            ctx.rgb(1, 0, 0)
        ctx.move_to(0, -105)
        ctx.text("^")
        ctx.move_to(0, -100)
        ctx.text("toggle wlan")
        ctx.restore()

        ctx.rgb(1, 1, 1)
        ctx.move_to(0, 100)
        ctx.text(self._status_text)

        wlan_draw_offset = self._wlan_offset * -20

        for wlan in self._nearby_wlans:
            ssid = wlan[0].decode()
            if (
                ssid == current_ssid
                and self._iface.active()
                and self._iface.isconnected()
            ):
                ctx.rgb(0, 1, 0)
            elif ssid == self._is_connecting:
                ctx.rgb(0, 0, 1)
            elif ssid in self._wifi_config["networks"]:
                ctx.rgb(1, 1, 0)
            else:
                ctx.rgb(1, 1, 1)
            if math.fabs(wlan_draw_offset) > 90:
                wlan_draw_offset += 20
                continue

            selected = self._nearby_wlans[self._wlan_offset] == wlan
            open_network = wlan[4] == 0

            ctx.font_size = 25 if selected else 15
            ssid_width = ctx.text_width(ssid)

            xpos = 0
            if selected:
                max_width = 220 if open_network else 200
                if ssid_width > max_width:
                    xpos = math.sin(self._scroll_pos) * (ssid_width - max_width) / 2
                    if not open_network:
                        xpos -= 10

            ctx.move_to(xpos, wlan_draw_offset)
            ctx.text(ssid)

            # TODO: maybe add signal indicator?
            # https://fonts.google.com/icons?selected=Material+Icons+Outlined:network_wifi_1_bar:&icon.query=network+wifi&icon.set=Material+Icons

            # draw a key next to wifi if it isn't open
            if not open_network:
                ctx.save()
                ctx.font = "Material Icons"
                ctx.text_align = ctx.LEFT
                ctx.move_to(xpos + (ssid_width / 2) + 5, wlan_draw_offset + 2)
                ctx.text("\ue897")
                ctx.restore()

            wlan_draw_offset += 20

    def on_exit(self) -> None:
        super().on_exit()
        leds.set_all_rgb(0, 0, 0)
        leds.update()

    def scan_wifi(self):
        # skip hidden WLANs
        self._nearby_wlans = [
            wlan for wlan in self._iface.scan() if not wlan[5] and wlan[0]
        ]
        # TODO: sort by known, then signal strength
        print(self._nearby_wlans)

    def update_settings_json(self, ssid: str, psk: str) -> None:
        # weirdo case
        if os.path.exists(self.SETTINGS_JSON_FILE):
            with open(self.SETTINGS_JSON_FILE) as f:
                settings_json = json.load(f)
        else:
            settings_json = {"system": {}}

        if "wifi" not in settings_json["system"]:
            settings_json["system"]["wifi"] = {
                "enabled": True,
                "ssid": "Camp2023-open",
                "psk": None,
            }
        # clean up old config
        if "camp_wifi_enabled" in settings_json["system"]:
            del settings_json["system"]["camp_wifi_enabled"]

        settings_json["system"]["wifi"]["ssid"] = ssid
        settings_json["system"]["wifi"]["psk"] = psk

        with open(self.SETTINGS_JSON_FILE, "w") as f:
            json.dump(settings_json, f)

    def add_to_config_json(self, ssid: str, psk: str) -> None:
        self._wifi_config["networks"][ssid] = {"psk": psk}

        with open(self.WIFI_CONFIG_FILE, "w") as f:
            json.dump(self._wifi_config, f)

    def connect_wifi(self, ssid: str, psk: str = None) -> None:
        if ssid in self._wifi_config["networks"]:
            psk = self._wifi_config["networks"][ssid]["psk"]

        self._current_ssid = ssid
        self._current_psk = psk

        try:
            self._is_connecting = ssid
            self._iface.connect(
                ssid,
                psk,
            )
            self._status_text = "connecting"
        except OSError as e:
            self._status_text = str(e)
            self._is_connecting = False

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        leds.set_all_rgb(0, 0, 0)
        self._scroll_pos += delta_ms / 1000

        if self.input.buttons.app.left.pressed and self._wlan_offset > 0:
            self._wlan_offset -= 1
            self._scroll_pos = 0.0
        elif (
            self.input.buttons.app.right.pressed
            and self._wlan_offset < len(self._nearby_wlans) - 1
        ):
            self._wlan_offset += 1
            self._scroll_pos = 0.0

        if not self._nearby_wlans and self._iface.active() and self._scan_timer <= 0:
            self._status_text = "scanning"
            self.scan_wifi()
            self._wlan_offset = 0
            self._status_text = "ready"
            self._scan_timer = 1

            if not self._nearby_wlans:
                self._iface.disconnect()

        if not self._nearby_wlans:
            self._scan_timer -= delta_ms / 1000

        if ins.captouch.petals[0].pressed:
            if not self._petal_pressed.get(0, False):
                self._iface.active(not self._iface.active())
                if not self._iface.active():
                    self._nearby_wlans = []
                else:
                    self._status_text = "scanning"
            self._petal_pressed[0] = True
        else:
            self._petal_pressed[0] = False

        if self._iface.active():
            set_direction_leds(0, 0, 1, 0)
        else:
            set_direction_leds(0, 1, 0, 0)
            self._status_text = "wlan off"

        if (
            self.input.buttons.app.middle.pressed
            and self._iface.active()
            and self._nearby_wlans
        ):
            hovered_network = self._nearby_wlans[self._wlan_offset]
            ssid = hovered_network[0].decode()
            if self._iface.isconnected():
                self._iface.disconnect()
            # network[4] = security level, 0 = open
            if ssid in self._wifi_config["networks"] or hovered_network[4] == 0:
                self.connect_wifi(ssid)
            else:
                self._waiting_for_password = True
                self.vm.push(KeyboardView(self._password_model))

        if self._waiting_for_password and (
            not self.vm._history or not isinstance(self.vm._history[-1], WifiApp)
        ):
            ssid = self._nearby_wlans[self._wlan_offset][0].decode()
            psk = self._password_model.text
            print(ssid, psk)
            self.connect_wifi(ssid, psk)
            self._password_model = TextInputModel("")
            self._waiting_for_password = False

        if self._is_connecting:
            self._connection_timer -= delta_ms / 1000
            if self._iface.isconnected():
                self._connection_timer = 10
                self._is_connecting = False

                if self._current_ssid:
                    self.update_settings_json(self._current_ssid, self._current_psk)
                    if self._current_ssid not in self._wifi_config["networks"]:
                        self.add_to_config_json(self._current_ssid, self._current_psk)
            elif self._connection_timer <= 0:
                self._iface.disconnect()
                self._status_text = "conn timed out"
                self._is_connecting = False

        if self._iface.isconnected():
            self._status_text = "connected"

        leds.update()


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_view(WifiApp(ApplicationContext()))
