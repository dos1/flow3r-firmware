"""
Settings menu of flow3r badge.
"""

from st3m import InputState, logging
from st3m.goose import (
    List,
    Tuple,
    Optional,
    Union,
    TYPE_CHECKING,
)
from st3m.ui.menu import MenuItem, MenuItemBack
from st3m.application import BundleMetadata, MenuItemAppLaunch
from st3m.ui.elements.menus import SimpleMenu
from st3m.ui.view import ViewManager
from st3m.settings import (
    UnaryTunable,
    load_all,
    save_all,
    onoff_show_tray,
    onoff_button_swap,
    onoff_debug,
    onoff_debug_touch,
    onoff_wifi,
    onoff_wifi_preference,
    onoff_show_fps,
    str_wifi_ssid,
    str_wifi_psk,
    str_hostname,
)
from ctx import Context

log = logging.Log(__name__, level=logging.INFO)


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


# List of all settings to be loaded/saved
load_save_settings: List[UnaryTunable] = [
    onoff_show_tray,
    onoff_button_swap,
    onoff_debug,
    onoff_debug_touch,
    onoff_wifi,
    onoff_wifi_preference,
    onoff_show_fps,
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
    onoff_show_fps,
    # onoff_debug,
    # onoff_debug_touch,
    onoff_wifi,
    onoff_wifi_preference,
    MenuItemAppLaunch(BundleMetadata("/flash/sys/apps/w1f1")),
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
