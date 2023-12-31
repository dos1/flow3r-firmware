from st3m.application import Application, ApplicationContext
from st3m.input import InputState
from st3m.goose import Optional
from st3m.ui.view import ViewManager
from st3m.utils import save_file_if_changed, sd_card_plugged
import st3m.settings
import st3m.wifi
from ctx import Context
import network
import leds
import os
import json
import math
from .k3yboard import TextInputModel, KeyboardView
from .helpers import (
    set_direction_leds,
    copy_across_devices,
    mark_unknown_characters,
)


class WifiApp(Application):
    WIFI_CONFIG_FILE = "/flash/w1f1_config.json"
    WIFI_CONFIG_FILE_SD = "/sd/w1f1_config.json"

    _scroll_pos: float = 0.0

    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self._petal_pressed = {}
        self._nearby_wlans = []
        self._status_text = "scanning"
        self._error_text = ""
        self._wifi_config = {}
        self._wlan_offset = 0
        self._is_connecting = False
        self._waiting_for_password = False
        self._password_model = TextInputModel("")

        self.attempt_load_wifi_config(self.WIFI_CONFIG_FILE)

        # Copy config to flash from SD card if we don't have one on flash
        if (
            sd_card_plugged()
            and os.path.exists(self.WIFI_CONFIG_FILE_SD)
            and not os.path.exists(self.WIFI_CONFIG_FILE)
        ):
            copy_across_devices(self.WIFI_CONFIG_FILE_SD, self.WIFI_CONFIG_FILE)
            self.attempt_load_wifi_config(self.WIFI_CONFIG_FILE)

        if not self._wifi_config:
            self._wifi_config = {
                "config_version": 2,
                "networks": {
                    "Example SSID": {"psk": "Example PSK"},
                    "Camp2023-open": {"psk": None},
                },
            }
            self.save_config_json()

    def attempt_load_wifi_config(self, config_path):
        if os.path.exists(config_path):
            try:
                with open(config_path) as f:
                    self._wifi_config = json.load(f)
            except ValueError:
                broken_filename = f"{config_path}.broken"
                print(
                    "FYI: Your wifi config file has a syntax error "
                    f"and has been moved to {broken_filename}."
                )
                if os.path.exists(broken_filename):
                    os.remove(broken_filename)
                os.rename(config_path, broken_filename)

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        self._connection_timer = 10
        self._scan_timer = 0
        if st3m.wifi.iface:
            self._iface = st3m.wifi.iface
        else:
            self._iface = network.WLAN(network.STA_IF)
            st3m.wifi.iface = self._iface
        self._current_ssid = None
        self._current_psk = None
        # TODO: big error display
        if self._waiting_for_password:
            ssid = self._nearby_wlans[self._wlan_offset][0].decode()
            psk = self._password_model.text
            print(ssid, psk)
            if psk:
                self.connect_wifi(ssid, psk)
            self._password_model = TextInputModel("")
            self._waiting_for_password = False

    def draw(self, ctx: Context) -> None:
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE

        ctx.rgb(0, 0, 0).rectangle(-120, -90, 240, 180).fill()
        ctx.rgb(0.2, 0.2, 0.2).rectangle(-120, -120, 240, 30).fill()
        ctx.rgb(0.2, 0.2, 0.2).rectangle(-120, 90, 240, 30).fill()

        ctx.font_size = 15
        current_ssid = self._iface.config("ssid")

        ctx.save()
        ctx.rgb(1, 1, 1)
        ctx.font = "Arimo Bold"
        if self._iface.active():
            ctx.rgb(0, 1, 0)
        else:
            ctx.rgb(1, 0, 0)
        ctx.move_to(0, -110)
        ctx.text("^")
        ctx.move_to(0, -100)
        ctx.text("toggle wlan")
        ctx.restore()

        ctx.rgb(1, 1, 1)
        ctx.move_to(0, 100)
        ctx.text(self._status_text)

        wlan_draw_offset = self._wlan_offset * -20

        for wlan in self._nearby_wlans:
            base_ssid = wlan[0].decode()
            ssid = wlan[-1]
            if (
                base_ssid == current_ssid
                and self._iface.active()
                and self._iface.isconnected()
            ):
                ctx.rgb(0, 1, 0)
            elif base_ssid == self._is_connecting:
                ctx.rgb(0, 0, 1)
            elif base_ssid in self._wifi_config["networks"]:
                ctx.rgb(1, 1, 0)
            else:
                ctx.rgb(1, 1, 1)
            if math.fabs(wlan_draw_offset) > 90:
                wlan_draw_offset += 20
                continue

            selected = self._nearby_wlans[self._wlan_offset] == wlan
            open_network = wlan[4] == 0

            ctx.font = "Arimo Bold" if selected else "Arimo Regular"
            ctx.font_size = 25 if selected else 15
            ssid_width = ctx.text_width(ssid)

            xpos = 0
            if selected:
                max_width = 220 if open_network else 200
                if ssid_width > max_width:
                    xpos = math.sin(self._scroll_pos) * (ssid_width - max_width) / 2
                    if not open_network:
                        xpos -= 7

            ctx.move_to(xpos, wlan_draw_offset)
            ctx.text(ssid)

            # TODO: maybe add signal indicator?
            # https://fonts.google.com/icons?selected=Material+Icons+Outlined:network_wifi_1_bar:&icon.query=network+wifi&icon.set=Material+Icons

            # draw a key next to wifi if it isn't open
            if not open_network:
                ctx.save()
                ctx.font = "Material Icons"
                ctx.text_align = ctx.LEFT
                ctx.move_to(xpos + (ssid_width / 2) + 2, wlan_draw_offset + 2)
                ctx.text("\ue897")
                ctx.restore()

            wlan_draw_offset += 20

    def on_exit(self) -> None:
        leds.set_all_rgb(0, 0, 0)
        leds.update()

    def scan_wifi(self):
        """
        scans for nearby wifi networks, hides private ones and sorts appropriately
        helpful: https://docs.micropython.org/en/latest/library/network.WLAN.html#network.WLAN.scan
        """
        # skip hidden WLANs
        detected_wlans = self._iface.scan()

        known_wlans = []
        unknown_wlans = []
        for wlan in detected_wlans:
            # skip hidden or invisible WLANs
            if wlan[5] or not wlan[0].strip():
                continue

            wlan_list = list(wlan)
            base_ssid = wlan[0].decode()
            clean_ssid = mark_unknown_characters(base_ssid).strip()
            wlan_list.append(clean_ssid)

            if base_ssid in self._wifi_config["networks"]:
                known_wlans.append(wlan_list)
            else:
                unknown_wlans.append(wlan_list)

        # sort by signal strength
        known_wlans.sort(key=lambda wlan: wlan[3], reverse=True)
        unknown_wlans.sort(key=lambda wlan: wlan[3], reverse=True)
        self._nearby_wlans = known_wlans + unknown_wlans
        print(self._nearby_wlans)

    def update_settings(self, ssid: str, psk: str) -> None:
        st3m.settings.str_wifi_ssid.set_value(ssid)
        st3m.settings.str_wifi_psk.set_value(psk)
        st3m.settings.save_all()

    def add_wlan_to_config_json(self, ssid: str, psk: str) -> None:
        self._wifi_config["networks"][ssid] = {"psk": psk}
        self.save_config_json()

    def save_config_json(self) -> None:
        config_str = json.dumps(self._wifi_config)
        save_file_if_changed(self.WIFI_CONFIG_FILE, config_str)

        if sd_card_plugged():
            try:
                save_file_if_changed(self.WIFI_CONFIG_FILE_SD, config_str)
            except OSError as e:
                print("SD issue:", str(e), ":(")

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
        self._scroll_pos += delta_ms / 1000

        leds.set_all_rgb(0, 0, 0)

        if self.input.buttons.app.left.pressed and self._wlan_offset > 0:
            self._wlan_offset -= 1
            self._scroll_pos = 0.0
        elif (
            self.input.buttons.app.right.pressed
            and self._wlan_offset < len(self._nearby_wlans) - 1
        ):
            self._wlan_offset += 1
            self._scroll_pos = 0.0

        if (
            not self._nearby_wlans
            and st3m.wifi.enabled()
            and self._scan_timer <= 0
            and not self.vm.transitioning
        ):
            self._status_text = "scanning"
            self.scan_wifi()
            self._wlan_offset = 0
            self._status_text = "connecting" if st3m.wifi.is_connecting() else "ready"
            self._scan_timer = 1

            if not self._nearby_wlans:
                self._iface.disconnect()

        if not self._nearby_wlans:
            self._scan_timer -= delta_ms / 1000

        if ins.captouch.petals[0].pressed:
            if not self._petal_pressed.get(0, False):
                if st3m.wifi.enabled():
                    st3m.wifi.disable()
                    st3m.wifi.iface = self._iface
                else:
                    st3m.wifi.setup_wifi()
                self._scan_timer = 1
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

        if self._is_connecting:
            self._connection_timer -= delta_ms / 1000
            if self._iface.isconnected():
                self._connection_timer = 10
                self._is_connecting = False

                if self._current_ssid:
                    self.update_settings(self._current_ssid, self._current_psk)
                    if self._current_ssid not in self._wifi_config["networks"]:
                        self.add_wlan_to_config_json(
                            self._current_ssid, self._current_psk
                        )
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

    st3m.run.run_app(WifiApp)
