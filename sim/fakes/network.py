STA_IF = 1


def hostname(hostname: str) -> None:
    return


class WLAN:
    _wlan_active = True
    _wlan_connected = True
    _config_data = {"ssid": "Test Wifi Open"}

    def __init__(self, mode):
        pass

    def active(self, active=None):
        if active is not None:
            self._wlan_active = active
        return self._wlan_active

    def scan(self):
        return [
            (b"Test Wifi Open", None, None, -100, 0, False),
            (b"Test Wifi", None, None, -100, 3, False),
        ]

    def connect(self, ssid, key=None):
        self._config_data["ssid"] = ssid
        self._wlan_connected = True

    def disconnect(self):
        self._wlan_connected = False

    def isconnected(self):
        return self._wlan_connected

    def config(self, key: str):
        return self._config_data[key]
