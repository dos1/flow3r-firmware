import network
from network import WLAN
from st3m import settings
from st3m.goose import Optional
from st3m.logging import Log
from st3m.ui.view import ViewManager, ViewTransition

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
    return iface is not None and iface.active()


def is_connected() -> bool:
    if iface is not None:
        return iface.isconnected()
    else:
        return False


def is_connecting() -> bool:
    return enabled() and iface.status() == network.STAT_CONNECTING


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


def run_wifi_settings(vm: ViewManager, vt: Optional[ViewTransition] = None):
    from st3m.application import BundleMetadata

    vm.push(BundleMetadata("/flash/sys/apps/w1f1").load(), vt)
