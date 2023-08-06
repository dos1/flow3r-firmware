from st3m.reactor import Reactor, Responder
from st3m.goose import List
from st3m.ui.menu import MenuItem, MenuItemBack, MenuItemForeground, MenuItemNoop
from st3m.ui.elements import overlays
from st3m.ui.view import View, ViewManager, ViewTransitionBlend
from st3m.ui.elements.menus import SimpleMenu, SunMenu
from st3m.application import discover_bundles, BundleMetadata
from st3m import settings, logging

import captouch, audio, leds, gc

log = logging.Log(__name__, level=logging.INFO)


def run_responder(r: Responder) -> None:
    """
    Run a given Responder in the foreground, without any menu or main firmware running in the background.

    This is useful for debugging trivial applications from the REPL.
    """
    reactor = Reactor()
    reactor.set_top(r)
    reactor.run()


def _make_bundle_menu(bundles: List[BundleMetadata], kind: str) -> SimpleMenu:
    entries: List[MenuItem] = [MenuItemBack()]
    for bundle in bundles:
        entries += bundle.menu_entries(kind)
    return SimpleMenu(entries)


def _make_compositor(reactor: Reactor, r: Responder) -> overlays.Compositor:
    """
    Set up top-level compositor (for combining viewmanager with overlays).
    """
    compositor = overlays.Compositor(r)

    # Tie compositor's debug overlay to setting.
    def _onoff_debug_update() -> None:
        compositor.enabled[overlays.OverlayKind.Debug] = settings.onoff_debug.value

    _onoff_debug_update()
    settings.onoff_debug.subscribe(_onoff_debug_update)

    # Configure debug overlay fragments.
    debug = overlays.OverlayDebug()
    debug.add_fragment(overlays.DebugReactorStats(reactor))
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
    return compositor


def run_view(v: View) -> None:
    """
    Run a given View in the foreground, with an empty ViewManager underneath.

    This is useful for debugging simple applications from the REPL.
    """
    vm = ViewManager(ViewTransitionBlend())
    vm.push(v)
    reactor = Reactor()
    compositor = _make_compositor(reactor, vm)
    reactor.set_top(compositor)
    reactor.run()


def run_main() -> None:
    log.info(f"starting main")
    log.info(f"free memory: {gc.mem_free()}")

    captouch.calibration_request()
    audio.set_volume_dB(0)
    leds.set_rgb(0, 255, 0, 0)
    leds.update()
    bundles = discover_bundles("/flash/sys/apps")

    settings.load_all()
    menu_settings = settings.build_menu()
    menu_system = SimpleMenu(
        [
            MenuItemBack(),
            MenuItemForeground("Settings", menu_settings),
            MenuItemNoop("Disk Mode"),
            MenuItemNoop("About"),
            MenuItemNoop("Reboot"),
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
    run_view(menu_main)


__all__ = [
    "run_responder",
    "run_view",
    "run_main",
]
