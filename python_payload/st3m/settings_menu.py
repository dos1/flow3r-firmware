"""
Settings menu of flow3r badge.
"""

from st3m import InputState, Responder, logging
from st3m.utils import lerp, ease_out_cubic
from st3m.goose import (
    List,
    Tuple,
    Optional,
    Union,
    TYPE_CHECKING,
)
from st3m.ui.menu import MenuItem, MenuItemBack, MenuItemAction
from st3m.application import BundleMetadata, MenuItemAppLaunch
from st3m.ui.elements.menus import SimpleMenu
from st3m.ui.view import ViewManager
from st3m.settings import *
from ctx import Context

log = logging.Log(__name__, level=logging.INFO)


class TunableWidget(Responder):
    """
    A tunable's widget as rendered in menus.
    """

    @abstractmethod
    def press(self, vm: Optional["ViewManager"]) -> None:
        """
        Called when the menu item is 'pressed', ie. selected/activated. A widget
        should react to this as the primary way to let the users manipulate the
        value of the tunable from a menu.
        """
        ...


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
        self._state = tunable.value is True
        self._prev_state = self._state

        # Value from 0 to 1, representing desired animation position. Linear
        # between both. 1 represents rendering _state, 0 represents render the
        # opposite of _state.
        self._progress = 1.0

    def think(self, ins: InputState, delta_ms: int) -> None:
        animation_duration = 0.2

        self._state = self._tunable.value is True

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

    def press(self, vm: Optional["ViewManager"]) -> None:
        self._tunable.set_value(not self._state)


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

    def press(self, vm: Optional["ViewManager"]) -> None:
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
            ctx.text(
                ("*" * len(str(self._tunable.value))) if self._tunable.value else ""
            )

    def press(self, vm: Optional["ViewManager"]) -> None:
        # Text input not supported at the moment
        pass


tunable_widget_map = {
    Tunable: TunableWidget,
    OnOffTunable: OnOffWidget,
    StringTunable: StringWidget,
    NumberTunable: StringWidget,  # temporary until an editable one is added
    ObfuscatedStringTunable: ObfuscatedValueWidget,
}


class SettingsMenuItem(MenuItem):
    """
    A MenuItem which draws its label offset to the left, and a Tunable's widget
    to the right.
    """

    def __init__(self, tunable: UnaryTunable):
        self.tunable = tunable
        self.widget = tunable_widget_map.get(type(tunable), TunableWidget)(tunable)

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


if TYPE_CHECKING:
    MenuStructureEntry = Union[UnaryTunable, Tuple[str, List["MenuStructureEntry"]]]
    MenuStructure = List[MenuStructureEntry]

# Main settings menu
settings_menu_structure: "MenuStructure" = [
    MenuItemAppLaunch(BundleMetadata("/flash/sys/apps/w1f1")),
    MenuItemAppLaunch(BundleMetadata("/flash/sys/apps/audio_config")),
    MenuItemAppLaunch(BundleMetadata("/flash/sys/apps/appearance")),
    MenuItemAppLaunch(BundleMetadata("/flash/sys/apps/graphics_mode")),
    onoff_wifi,
    onoff_wifi_preference,
    onoff_show_tray,
    onoff_button_swap,
    onoff_show_fps,
    onoff_debug_ftop,
    onoff_debug_touch,
    # onoff_debug,
    MenuItemAction("Restore Defaults", restore_defaults),
]


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
