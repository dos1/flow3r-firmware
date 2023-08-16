import network
from st3m import settings

iface = None


def setup_camp_wifi() -> None:
    global iface
    iface = network.WLAN(network.STA_IF)
    iface.active(True)
    iface.connect(b"Camp2023-open")


def disable() -> None:
    if iface is not None:
        iface.active(False)


def is_connected() -> bool:
    if iface is not None:
        return iface.isconnected()
    else:
        return False


def _onoff_camp_wifi_update() -> None:
    if settings.onoff_camp_wifi.value:
        setup_camp_wifi()
    else:
        disable()
