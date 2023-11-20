import os
import machine

import sys_display
import leds
from ctx import Context

from st3m.goose import Optional, List, Set
from st3m.ui.view import ViewManager, ViewTransitionDirection
from st3m import InputState
from st3m import settings_menu as settings
import st3m.wifi
from st3m.ui import led_patterns
from st3m.ui.menu import (
    MenuItem,
    MenuItemBack,
    MenuItemForeground,
    MenuItemAction,
    MenuItemLaunchPersistentView,
)
from st3m.application import (
    BundleManager,
    BundleMetadata,
    MenuItemAppLaunch,
)
from st3m.about import About
from st3m.ui.elements.menus import SimpleMenu, SunMenu


class ApplicationMenu(SimpleMenu):
    def _restore_sys_defaults(self) -> None:
        if (
            not self.vm
            or not self.is_active()
            or self.vm.direction != ViewTransitionDirection.BACKWARD
        ):
            return
        # fall back to system defaults on app exit
        st3m.wifi._onoff_wifi_update()
        # set the default graphics mode, this is a no-op if
        # it is already set
        sys_display.set_mode(0)
        sys_display.fbconfig(240, 240, 0, 0)
        leds.set_slew_rate(100)
        leds.set_gamma(1.0, 1.0, 1.0)
        leds.set_auto_update(False)
        leds.set_brightness(settings.num_leds_brightness.value)
        sys_display.set_backlight(settings.num_display_brightness.value)
        led_patterns.set_menu_colors()
        # media.stop()

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        self._restore_sys_defaults()

    def on_enter_done(self):
        # set the defaults again in case the app continued
        # doing stuff during the transition
        self._restore_sys_defaults()
        leds.update()


def _get_bundle_menu_kinds(mgr: BundleManager) -> Set[str]:
    kinds: Set[str] = set()
    for bundle in mgr.bundles.values():
        kinds.update(bundle.menu_kinds())
    return kinds


def _get_bundle_menu_entries(mgr: BundleManager, kind: str) -> List[MenuItem]:
    entries: List[MenuItem] = []
    ids = sorted(mgr.bundles.keys(), key=str.lower)
    for id in ids:
        bundle = mgr.bundles[id]
        entries += bundle.menu_entries(kind)
    return entries


def _yeet_local_changes() -> None:
    os.remove("/flash/sys/.sys-installed")
    machine.reset()


class MainMenu(SunMenu):
    def __init__(self, bundles: Optional[list] = None) -> None:
        if bundles:
            self._bundles = bundles
        else:
            self._bundles = BundleManager()
            self._bundles.update()

        self.load_menu(reload_bundles=False)

    def load_menu(self, reload_bundles: bool = True) -> None:
        """
        (Re)loads the menu.
        Calling this after it has been loaded is a potential memory leak issue.
        """
        if reload_bundles:
            self._bundles.bundles = {}
            self._bundles.update()
        self.build_menu_items()
        super().__init__(self._items)

    def build_menu_items(self) -> None:
        menu_settings = settings.build_menu()
        menu_system = ApplicationMenu(
            [
                MenuItemBack(),
                MenuItemLaunchPersistentView("About", About),
                MenuItemForeground("Settings", menu_settings),
                MenuItemAppLaunch(BundleMetadata("/flash/sys/apps/gr33nhouse")),
                MenuItemAppLaunch(BundleMetadata("/flash/sys/apps/updat3r")),
                MenuItemAction("Disk Mode (Flash)", machine.disk_mode_flash),
                MenuItemAction("Disk Mode (SD)", machine.disk_mode_sd),
                MenuItemAction("Yeet Local Changes", _yeet_local_changes),
                MenuItemAction("Reboot", machine.reset),
            ],
        )

        app_kinds = _get_bundle_menu_kinds(self._bundles)
        menu_categories = ["Badge", "Music", "Media", "Apps", "Games"]
        for kind in app_kinds:
            if kind not in ["Hidden", "System"] and kind not in menu_categories:
                menu_categories.append(kind)

        categories = [
            MenuItemForeground(kind, ApplicationMenu([MenuItemBack()] + entries))
            for kind in menu_categories
            if (entries := _get_bundle_menu_entries(self._bundles, kind))
        ]
        categories.append(MenuItemForeground("System", menu_system))

        self._items = categories
        # # self._scroll_controller = ScrollController()
        # self._scroll_controller.set_item_count(len(categories))

    def on_enter(self, vm):
        super().on_enter(vm)
        if self.vm.direction == ViewTransitionDirection.FORWARD:
            led_patterns.set_menu_colors()
            leds.set_slew_rate(20)
            leds.update()
