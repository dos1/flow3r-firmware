"""
Settings framework for flow3r badge.

We call settings 'tunables', trying to emphasize that they are some kind of
value that can be changed.

Settings are persisted in /flash/settings.json, loaded on startup and saved on
request.
"""

import json

from st3m import InputState, Responder, logging
from st3m.goose import (
    ABCBase,
    abstractmethod,
    Any,
    Dict,
    List,
    Tuple,
    Optional,
    Callable,
    Union,
    TYPE_CHECKING,
)
from st3m.ui.menu import MenuController, MenuItem, MenuItemBack, MenuItemForeground
from st3m.application import BundleMetadata, MenuItemAppLaunch
from st3m.ui.elements.menus import SimpleMenu
from st3m.ui.view import ViewManager
from st3m.utils import lerp, ease_out_cubic, reduce
from ctx import Context

log = logging.Log(__name__, level=logging.INFO)


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
    def get_widget(self) -> "TunableWidget":
        """
        Widget that will be used to render this setting in menus.
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


class TunableWidget(Responder):
    """
    A tunable's widget as rendered in menus.
    """

    @abstractmethod
    def press(self, vm: Optional[ViewManager]) -> None:
        """
        Called when the menu item is 'pressed', ie. selected/activated. A widget
        should react to this as the primary way to let the users manipulate the
        value of the tunable from a menu.
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
        self.value: Any = default

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


class OnOffTunable(UnaryTunable):
    """
    OnOffTunable is a UnaryTunable that has two values: on or off, and is
    rendered accordingly as a slider switch.
    """

    def __init__(self, name: str, key: str, default: bool) -> None:
        super().__init__(name, key, default)

    def get_widget(self) -> TunableWidget:
        return OnOffWidget(self)

    def press(self, vm: Optional[ViewManager]) -> None:
        if self.value == True:
            self.set_value(False)
        else:
            self.set_value(True)


class OnOffWidget(TunableWidget):
    """
    OnOffWidget is a TunableWidget for OnOffTunables. It renders a slider
    switch.
    """

    def __init__(self, tunable: "OnOffTunable") -> None:
        self._tunable = tunable

        # Value from 0 to animation_duration indicating animation progress
        # (starts at animation_duration, ends at 0).
        self._animating: float = 0

        # Last and previous read value from tunable.
        self._state = tunable.value == True
        self._prev_state = self._state

        # Value from 0 to 1, representing desired animation position. Linear
        # between both. 1 represents rendering _state, 0 represents render the
        # opposite of _state.
        self._progress = 1.0

    def think(self, ins: InputState, delta_ms: int) -> None:
        animation_duration = 0.2

        self._state = self._tunable.value == True

        if self._prev_state != self._state:
            # State switched.

            # Start new animation, making sure to take into consideration
            # whatever animation is already taking place.
            self._animating = animation_duration - self._animating
        else:
            # Continue animation.
            self._animating -= delta_ms / 1000
            if self._animating < 0:
                self._animating = 0

        # Calculate progress value.
        self._progress = 1.0 - (self._animating / animation_duration)
        self._prev_state = self._state

    def draw(self, ctx: Context) -> None:
        # TODO(pippin): graphical glitches without the two next lines
        ctx.rectangle(-200, -200, 10, 10)
        ctx.fill()

        value = self._state
        v = self._progress
        v = ease_out_cubic(v)
        if not value:
            v = 1.0 - v

        ctx.rgb(lerp(0, 0.4, v), lerp(0, 0.6, v), lerp(0, 0.4, v))

        ctx.round_rectangle(0, -10, 40, 20, 5)
        ctx.line_width = 2
        ctx.fill()

        ctx.round_rectangle(0, -10, 40, 20, 5)
        ctx.line_width = 2
        ctx.gray(lerp(0.3, 1, v))
        ctx.stroke()

        ctx.gray(1)
        ctx.round_rectangle(lerp(2, 22, v), -8, 16, 16, 5)
        ctx.fill()

    def press(self, vm: Optional[ViewManager]) -> None:
        self._tunable.set_value(not self._state)


class StringTunable(UnaryTunable):
    """
    StringTunable is a UnaryTunable that has a string value
    """

    def __init__(self, name: str, key: str, default: Optional[str]) -> None:
        super().__init__(name, key, default)

    def get_widget(self) -> TunableWidget:
        return StringWidget(self)

    def press(self, vm: Optional[ViewManager]) -> None:
        # Text input not supported at the moment
        pass


class StringWidget(TunableWidget):
    """
    StringWidget is a TunableWidget for StringTunables. It renders a string.
    """

    def __init__(self, tunable: StringTunable) -> None:
        self._tunable = tunable

    def think(self, ins: InputState, delta_ms: int) -> None:
        # Nothing to do here
        pass

    def draw(self, ctx: Context) -> None:
        ctx.text_align = ctx.LEFT
        ctx.text(str(self._tunable.value) if self._tunable.value else "")

    def press(self, vm: Optional[ViewManager]) -> None:
        # Text input not supported at the moment
        pass


# TODO: invert Tunable <-> Widget dependency to be able to define multiple different widget renderings for the same underlying tunable type
class ObfuscatedStringTunable(UnaryTunable):
    """
    ObfuscatedStringTunable is a UnaryTunable that has a string value that should not be revealed openly.
    """

    def __init__(self, name: str, key: str, default: Optional[str]) -> None:
        super().__init__(name, key, default)

    def get_widget(self) -> TunableWidget:
        return ObfuscatedValueWidget(self)

    def press(self, vm: Optional[ViewManager]) -> None:
        # Text input not supported at the moment
        pass


class ObfuscatedValueWidget(TunableWidget):
    """
    ObfuscatedValueWidget is a TunableWidget for UnaryTunables. It renders three asterisks when the tunable contains a truthy value, otherwise nothing.
    """

    def __init__(self, tunable: UnaryTunable) -> None:
        self._tunable = tunable

    def think(self, ins: InputState, delta_ms: int) -> None:
        # Nothing to do here
        pass

    def draw(self, ctx: Context) -> None:
        ctx.text_align = ctx.LEFT
        if self._tunable.value:
            ctx.text("***")

    def press(self, vm: Optional[ViewManager]) -> None:
        # Text input not supported at the moment
        pass


class SettingsMenuItem(MenuItem):
    """
    A MenuItem which draws its label offset to the left, and a Tunable's widget
    to the right.
    """

    def __init__(self, tunable: UnaryTunable):
        self.tunable = tunable
        self.widget = tunable.get_widget()

    def press(self, vm: Optional[ViewManager]) -> None:
        self.widget.press(vm)

    def label(self) -> str:
        return self.tunable._name

    def draw(self, ctx: Context) -> None:
        ctx.move_to(40, 0)
        ctx.text_align = ctx.RIGHT
        super().draw(ctx)
        ctx.stroke()

        ctx.save()
        ctx.translate(50, 0)
        self.widget.draw(ctx)
        ctx.restore()

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.widget.think(ins, delta_ms)


class SettingsMenu(SimpleMenu):
    """
    SimpleMenu but smol.
    """

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        load_all()

    def on_exit(self) -> None:
        save_all()
        super().on_exit()

    SIZE_LARGE = 20
    SIZE_SMALL = 15


# Actual tunables / settings.
onoff_button_swap = OnOffTunable("Swap Buttons", "system.swap_buttons", False)
onoff_debug = OnOffTunable("Debug Overlay", "system.debug", False)
onoff_debug_touch = OnOffTunable("Touch Overlay", "system.debug_touch", False)
onoff_show_tray = OnOffTunable("Show Icons", "system.show_icons", True)
onoff_wifi = OnOffTunable("Enable WiFi on Boot", "system.wifi.enabled", False)
str_wifi_ssid = StringTunable("WiFi SSID", "system.wifi.ssid", "Camp2023-open")
str_wifi_psk = ObfuscatedStringTunable("WiFi Password", "system.wifi.psk", None)
str_hostname = StringTunable("Hostname", "system.hostname", "flow3r")

# List of all settings to be loaded/saved
load_save_settings: List[UnaryTunable] = [
    onoff_show_tray,
    onoff_button_swap,
    onoff_debug,
    onoff_debug_touch,
    onoff_wifi,
    str_wifi_ssid,
    str_wifi_psk,
    str_hostname,
]

if TYPE_CHECKING:
    MenuStructureEntry = Union[UnaryTunable, Tuple[str, List["MenuStructureEntry"]]]
    MenuStructure = List[MenuStructureEntry]

# Main settings menu
settings_menu_structure: "MenuStructure" = [
    onoff_show_tray,
    onoff_button_swap,
    onoff_debug,
    onoff_debug_touch,
    onoff_wifi,
    MenuItemAppLaunch(BundleMetadata("/flash/sys/apps/w1f1")),
]


def load_all() -> None:
    """
    Load all settings from flash.
    """
    data = {}
    try:
        with open("/flash/settings.json", "r") as f:
            data = json.load(f)
    except Exception as e:
        log.warning("Could not load settings: " + str(e))
        return

    log.info("Loaded settings from flash")
    for setting in load_save_settings:
        setting.load(data)


def _update(d: Dict[str, Any], u: Dict[str, Any]) -> Dict[str, Any]:
    """
    Recursive update dictionary.
    """
    for k, v in u.items():
        if type(v) == type({}):
            d[k] = _update(d.get(k, {}), v)
        else:
            d[k] = v
    return d


def save_all() -> None:
    """
    Save all settings to flash.
    """
    res: Dict[str, Any] = {}
    for setting in load_save_settings:
        res = _update(res, setting.save())
    try:
        with open("/flash/settings.json", "w") as f:
            json.dump(res, f)
    except Exception as e:
        log.warning("Could not save settings: " + str(e))
        return

    log.info("Saved settings to flash")


def build_menu_recursive(items: "MenuStructure") -> SimpleMenu:
    """
    Recursively build a menu for the given setting structure.
    """
    mib: MenuItem = MenuItemBack()
    positions: List[MenuItem] = [
        mib,
    ] + [SettingsMenuItem(t) if isinstance(t, UnaryTunable) else t for t in items]

    return SettingsMenu(positions)


def build_menu() -> SimpleMenu:
    """
    Return a SettingsMenu for all settings.
    """
    return build_menu_recursive(settings_menu_structure)
