from st3m.reactor import Reactor, Responder
from st3m.goose import List, Optional
from st3m.ui.menu import (
    MenuItem,
    MenuItemBack,
    MenuItemForeground,
    MenuItemNoop,
    MenuItemAction,
    MenuItemLaunchPersistentView,
)
from st3m.ui.elements import overlays
from st3m.ui.view import View, ViewManager, ViewTransitionBlend
from st3m.ui.elements.menus import SimpleMenu, SunMenu
from st3m.application import discover_bundles, BundleMetadata
from st3m.about import About
from st3m import settings, logging, processors

import captouch, audio, leds, gc
import os

import machine


log = logging.Log(__name__, level=logging.INFO)

#: Can be set to a bundle name that should be started instead of the main menu when run_main is called.
override_main_app: Optional[str] = None


def _make_reactor() -> Reactor:
    reactor = Reactor()

    def _onoff_button_swap_update() -> None:
        reactor.set_buttons_swapped(settings.onoff_button_swap.value)

    settings.onoff_button_swap.subscribe(_onoff_button_swap_update)
    _onoff_button_swap_update()
    return reactor


def run_responder(r: Responder) -> None:
    """
    Run a given Responder in the foreground, without any menu or main firmware running in the background.

    This is useful for debugging trivial applications from the REPL.
    """
    reactor = _make_reactor()
    reactor.set_top(r)
    reactor.run()


def _make_bundle_menu(bundles: List[BundleMetadata], kind: str) -> SimpleMenu:
    entries: List[MenuItem] = [MenuItemBack()]
    for bundle in bundles:
        entries += bundle.menu_entries(kind)
    return SimpleMenu(entries)


def _make_compositor(reactor: Reactor, vm: ViewManager) -> overlays.Compositor:
    """
    Set up top-level compositor (for combining viewmanager with overlays).
    """
    compositor = overlays.Compositor(vm)

    volume = overlays.OverlayVolume()
    compositor.add_overlay(volume)

    # Tie compositor's debug overlay to setting.
    def _onoff_debug_update() -> None:
        compositor.enabled[overlays.OverlayKind.Debug] = settings.onoff_debug.value

    _onoff_debug_update()
    settings.onoff_debug.subscribe(_onoff_debug_update)

    # Configure debug overlay fragments.
    debug = overlays.OverlayDebug()
    debug.add_fragment(overlays.DebugReactorStats(reactor))
    debug.add_fragment(overlays.DebugBattery())
    compositor.add_overlay(debug)

    debug_touch = overlays.OverlayCaptouch()

    # Tie compositor's debug touch overlay to setting.
    def _onoff_debug_touch_update() -> None:
        compositor.enabled[
            overlays.OverlayKind.Touch
        ] = settings.onoff_debug_touch.value

    _onoff_debug_touch_update()
    settings.onoff_debug_touch.subscribe(_onoff_debug_touch_update)
    compositor.add_overlay(debug_touch)

    # Add icon tray.
    compositor.add_overlay(overlays.IconTray())
    return compositor


def run_view(v: View) -> None:
    """
    Run a given View in the foreground, with an empty ViewManager underneath.

    This is useful for debugging simple applications from the REPL.
    """
    vm = ViewManager(ViewTransitionBlend())
    vm.push(v)
    reactor = _make_reactor()
    compositor = _make_compositor(reactor, vm)
    top = processors.ProcessorMidldeware(compositor)
    reactor.set_top(top)
    reactor.run()


def _yeet_local_changes() -> None:
    os.remove("/flash/sys/.sys-installed")
    machine.reset()


def run_main() -> None:
    log.info(f"starting main")
    log.info(f"free memory: {gc.mem_free()}")

    captouch.calibration_request()
    # TODO(q3k): volume control. but until then, make slightly less loud on startup.
    audio.set_volume_dB(-10)
    leds.set_rgb(0, 255, 0, 0)
    leds.update()
    bundles = discover_bundles("/flash/sys/apps")

    settings.load_all()
    menu_settings = settings.build_menu()
    menu_system = SimpleMenu(
        [
            MenuItemBack(),
            MenuItemForeground("Settings", menu_settings),
            MenuItemAction("Disk Mode (Flash)", machine.disk_mode_flash),
            MenuItemAction("Disk Mode (SD)", machine.disk_mode_sd),
            MenuItemLaunchPersistentView("About", About),
            MenuItemAction("Yeet Local Changes", _yeet_local_changes),
            MenuItemAction("Reboot", machine.reset),
        ],
    )
    menu_main = SunMenu(
        [
            MenuItemForeground("Badge", _make_bundle_menu(bundles, "Badge")),
            MenuItemForeground("Music", _make_bundle_menu(bundles, "Music")),
            MenuItemForeground("Apps", _make_bundle_menu(bundles, "Apps")),
            MenuItemForeground("System", menu_system),
        ],
    )
    if override_main_app is not None:
        requested = [b for b in bundles if b.name == override_main_app]
        print([b.name for b in bundles])
        if len(requested) > 1:
            raise Exception(f"More than one bundle named {override_main_app}")
        if len(requested) == 0:
            raise Exception(f"Requested bundle {override_main_app} not found")
        run_view(requested[0].load())
    run_view(menu_main)


__all__ = [
    "run_responder",
    "run_view",
    "run_main",
]
