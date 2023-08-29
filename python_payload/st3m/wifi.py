import network
from network import WLAN
from st3m import settings
from st3m.goose import Optional
from st3m.logging import Log

log = Log(__name__)

iface: Optional[WLAN] = None


def setup_wifi() -> None:
    global iface
    enable()
    assert iface
    try:
        if settings.str_wifi_ssid.value:
            iface.disconnect()
            iface.connect(settings.str_wifi_ssid.value, settings.str_wifi_psk.value)
    except OSError as e:
        log.error(f"Could not connect to wifi: {e}")


def enable() -> None:
    global iface
    iface = network.WLAN(network.STA_IF)
    iface.active(True)


def disable() -> None:
    global iface
    if iface is not None:
        iface.active(False)
        iface = None


def enabled() -> bool:
    return iface is not None


def is_connected() -> bool:
    if iface is not None:
        return iface.isconnected()
    else:
        return False


def _onoff_wifi_update() -> None:
    if settings.onoff_wifi.value and not is_connected():
        setup_wifi()
    elif not settings.onoff_wifi.value:
        disable()


def rssi() -> float:
    if iface is None:
        return -120
    try:
        return iface.status("rssi")
    except OSError:
        return -120
