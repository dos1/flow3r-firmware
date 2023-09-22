"""
Settings framework for flow3r badge.

We call settings 'tunables', trying to emphasize that they are some kind of
value that can be changed.

Settings are persisted in /flash/settings.json, loaded on startup and saved on
request.
"""

import json

from st3m import logging
from st3m.goose import (
    ABCBase,
    abstractmethod,
    Any,
    Dict,
    List,
    Optional,
    Callable,
)
from st3m.utils import reduce, save_file_if_changed

log = logging.Log(__name__, level=logging.INFO)

SETTINGS_JSON_FILE = "/flash/settings.json"


class Tunable(ABCBase):
    """
    Base class for all settings. An instance of a Tunable is some kind of
    setting with some kind of value. Each setting has exactly one instance of
    Tunable in memory that represents its current configuration.

    Other than a common interface, this also implements a mechanism for
    downstream consumers to subscribe to tunable changes, and allows notifying
    them.
    """

    def __init__(self) -> None:
        self._subscribers: List[Callable[[], None]] = []

    def subscribe(self, s: Callable[[], None]) -> None:
        """
        Subscribe to be updated when this tunable value's changes.
        """
        self._subscribers.append(s)

    def notify(self) -> None:
        """
        Notify all subscribers.
        """
        for s in self._subscribers:
            s()

    @abstractmethod
    def name(self) -> str:
        """
        Human-readable name of this setting.
        """
        ...

    @abstractmethod
    def save(self) -> Dict[str, Any]:
        """
        Return dictionary that contains this setting's persistance data. Will be
        merged with all other tunable's results.
        """
        ...

    @abstractmethod
    def load(self, d: Dict[str, Any]) -> None:
        """
        Load in-memory state from persisted data.
        """
        ...


class UnaryTunable(Tunable):
    """
    Basic implementation of a Tunable for single values. Most settings will be
    UnaryTunables, with notable exceptions being things like lists or optional
    settings.

    UnaryTunable implements persistence by always being saved/loaded to same
    json.style.path (only traversing nested dictionaries).
    """

    def __init__(self, name: str, key: str, default: Any):
        """
        Create an UnaryTunable with a given human-readable name, some
        persistence key and some default value.
        """
        super().__init__()
        self.key = key
        self._name = name
        self._default = default
        self.value: Any = self._default

    def name(self) -> str:
        return self._name

    def set_value(self, v: Any) -> None:
        """
        Call to set value in-memory and notify all listeners.
        """
        self.value = v
        self.notify()

    def save(self) -> Dict[str, Any]:
        res: Dict[str, Any] = {}
        sub = res

        parts = self.key.split(".")
        for i, part in enumerate(parts):
            if i == len(parts) - 1:
                sub[part] = self.value
            else:
                sub[part] = {}
                sub = sub[part]
        return res

    def load(self, d: Dict[str, Any]) -> None:
        def _get(v: Dict[str, Any], k: str) -> Any:
            if k in v:
                return v[k]
            else:
                return {}

        path = self.key.split(".")
        k = path[-1]
        d = reduce(_get, path[:-1], d)
        if k in d:
            self.value = d[k]
        else:
            self.set_value(self._default)


class OnOffTunable(UnaryTunable):
    """
    OnOffTunable is a UnaryTunable that has two values: on or off, and is
    rendered accordingly as a slider switch.
    """

    def __init__(self, name: str, key: str, default: bool) -> None:
        super().__init__(name, key, default)

    def press(self, vm: Optional["ViewManager"]) -> None:
        if self.value is True:
            self.set_value(False)
        else:
            self.set_value(True)


class StringTunable(UnaryTunable):
    """
    StringTunable is a UnaryTunable that has a string value
    """

    def __init__(self, name: str, key: str, default: Optional[str]) -> None:
        super().__init__(name, key, default)

    def press(self, vm: Optional["ViewManager"]) -> None:
        # Text input not supported at the moment
        pass


class NumberTunable(UnaryTunable):
    """
    NumberTunable is a UnaryTunable that has a numeric value
    """

    def __init__(self, name: str, key: int | float, default: Optional[str]) -> None:
        super().__init__(name, key, default)

    def press(self, vm: Optional["ViewManager"]) -> None:
        # Number adjustment not supported at the moment
        pass


# TODO: invert Tunable <-> Widget dependency to be able to define multiple different widget renderings for the same underlying tunable type
class ObfuscatedStringTunable(UnaryTunable):
    """
    ObfuscatedStringTunable is a UnaryTunable that has a string value that should not be revealed openly.
    """

    def __init__(self, name: str, key: str, default: Optional[str]) -> None:
        super().__init__(name, key, default)

    def press(self, vm: Optional["ViewManager"]) -> None:
        # Text input not supported at the moment
        pass


# Actual tunables / settings.
onoff_button_swap = OnOffTunable("Swap Buttons", "system.swap_buttons", False)
onoff_show_fps = OnOffTunable("Show FPS", "system.show_fps", False)
onoff_debug = OnOffTunable("Debug Overlay", "system.debug", False)
onoff_debug_touch = OnOffTunable("Touch Overlay", "system.debug_touch", False)
onoff_debug_ftop = OnOffTunable("Debug: ftop", "system.ftop_enabled", False)
onoff_show_tray = OnOffTunable("Show Icons", "system.show_icons", True)
onoff_wifi = OnOffTunable("Enable WiFi on Boot", "system.wifi.enabled", False)
onoff_wifi_preference = OnOffTunable(
    "Let apps change WiFi", "system.wifi.allow_apps_to_change_wifi", True
)
str_wifi_ssid = StringTunable("WiFi SSID", "system.wifi.ssid", "Camp2023-open")
str_wifi_psk = ObfuscatedStringTunable("WiFi Password", "system.wifi.psk", None)
str_hostname = StringTunable("Hostname", "system.hostname", "flow3r")

num_volume_step_db = StringTunable(
    "Volume Change dB", "system.audio.volume_step_dB", 1.5
)
num_volume_repeat_step_db = StringTunable(
    "Volume Repeat Change dB", "system.audio.volume_repeat_step_dB", 2.5
)
num_volume_repeat_wait_ms = StringTunable(
    "Volume Repeat Wait Time ms", "system.audio.volume_repeat_wait_ms", 800
)
num_volume_repeat_ms = StringTunable(
    "Volume Repeat Time ms", "system.audio.volume_repeat_ms", 300
)

num_speaker_startup_volume_db = StringTunable(
    "Speaker Startup Volume dB", "system.audio.speaker_startup_volume_dB", -10
)
num_headphones_startup_volume_db = StringTunable(
    "Headphones Startup Volume dB", "system.audio.headphones_startup_volume_dB", -10
)
num_headphones_min_db = StringTunable(
    "Min Headphone Volume dB", "system.audio.headphones_min_dB", -45
)
num_speaker_min_db = StringTunable(
    "Min Speaker Volume dB", "system.audio.speakers_min_dB", -40
)
num_headphones_max_db = StringTunable(
    "Max Headphone Volume dB", "system.audio.headphones_max_dB", 3
)
num_speaker_max_db = StringTunable(
    "Max Speaker Volume dB", "system.audio.speakers_max_dB", 14
)

num_display_brightness = StringTunable(
    "Display Brightness", "system.appearance.display_brightness", 100
)
num_leds_brightness = StringTunable(
    "LED Brightness", "system.appearance.leds_brightness", 70
)

num_leds_speed = StringTunable("LED Speed", "system.appearance.leds_speed", 235)
onoff_leds_random_menu = OnOffTunable(
    "Random Menu LEDs", "system.appearance.leds_random_menu", True
)

# List of all settings to be loaded/saved
load_save_settings: List[UnaryTunable] = [
    onoff_show_tray,
    onoff_button_swap,
    onoff_debug,
    onoff_debug_ftop,
    onoff_debug_touch,
    onoff_wifi,
    onoff_wifi_preference,
    onoff_show_fps,
    str_wifi_ssid,
    str_wifi_psk,
    str_hostname,
    num_volume_step_db,
    num_volume_repeat_step_db,
    num_volume_repeat_wait_ms,
    num_volume_repeat_ms,
    num_speaker_startup_volume_db,
    num_headphones_startup_volume_db,
    num_headphones_min_db,
    num_speaker_min_db,
    num_headphones_max_db,
    num_speaker_max_db,
    num_display_brightness,
    num_leds_brightness,
    num_leds_speed,
    onoff_leds_random_menu,
]


def load_all() -> None:
    """
    Load all settings from flash.
    """
    data = {}
    try:
        with open(SETTINGS_JSON_FILE, "r") as f:
            data = json.load(f)
    except Exception as e:
        log.warning("Could not load settings: " + str(e))
        return

    log.info("Loaded settings from flash")
    for setting in load_save_settings:
        setting.load(data)


def restore_defaults() -> None:
    """
    Restore default settings. Relies on save on exit.
    """
    data = {}
    for setting in load_save_settings:
        setting.load(data)


def _update(d: Dict[str, Any], u: Dict[str, Any]) -> Dict[str, Any]:
    """
    Recursive update dictionary.
    """
    for k, v in u.items():
        if isinstance(v, dict):
            d[k] = _update(d.get(k, {}), v)
        else:
            d[k] = v
    return d


def save_all() -> None:
    """
    Save all settings to flash.
    """
    res: Dict[str, Any] = {}
    saved_settings = False
    for setting in load_save_settings:
        res = _update(res, setting.save())
    try:
        saved_settings = save_file_if_changed(SETTINGS_JSON_FILE, json.dumps(res))
    except Exception as e:
        log.warning("Could not save settings: " + str(e))
        return

    log.info(
        "Saved settings to flash"
        if saved_settings
        else "Skipped saving settings to flash as nothing changed"
    )


load_all()
