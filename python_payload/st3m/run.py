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
from st3m.application import (
    BundleManager,
    BundleMetadata,
    MenuItemAppLaunch,
    ApplicationContext,
)
from st3m.about import About
from st3m import settings_menu as settings, logging, processors, wifi

import captouch, audio, leds, gc, sys_buttons, sys_display, sys_mode
import os

import machine
import network


log = logging.Log(__name__, level=logging.INFO)

#: Can be set to a bundle name that should be started instead of the main menu when run_main is called.
override_main_app: Optional[str] = None


def _make_reactor() -> Reactor:
    reactor = Reactor()

    def _onoff_button_swap_update() -> None:
        left = not settings.onoff_button_swap.value
        sys_buttons.configure(left)

    settings.onoff_button_swap.subscribe(_onoff_button_swap_update)
    _onoff_button_swap_update()

    settings.onoff_wifi.subscribe(wifi._onoff_wifi_update)
    wifi._onoff_wifi_update()
    return reactor


def run_responder(r: Responder) -> None:
    """
    Run a given Responder in the foreground, without any menu or main firmware running in the background.

    This is useful for debugging trivial applications from the REPL.
    """
    reactor = _make_reactor()
    reactor.set_top(r)
    reactor.run()


def _make_bundle_menu(mgr: BundleManager, kind: str) -> SimpleMenu:
    entries: List[MenuItem] = [MenuItemBack()]
    ids = sorted(mgr.bundles.keys())
    for id in ids:
        bundle = mgr.bundles[id]
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

    # Tie compositor's icon visibility to setting.
    def _onoff_show_tray_update() -> None:
        compositor.enabled[
            overlays.OverlayKind.Indicators
        ] = settings.onoff_show_tray.value

    _onoff_show_tray_update()
    settings.onoff_show_tray.subscribe(_onoff_show_tray_update)

    # Add icon tray.
    compositor.add_overlay(overlays.IconTray())
    return compositor


def run_view(v: View, debug_vm=True) -> None:
    """
    Run a given View in the foreground, with an empty ViewManager underneath.

    This is useful for debugging simple applications from the REPL.
    """
    reactor = _make_reactor()
    vm = ViewManager(ViewTransitionBlend(), debug=debug_vm)
    vm.push(v)
    sys_mode.mode_set(2)  # st3m_mode_kind_app
    compositor = _make_compositor(reactor, vm)
    top = processors.ProcessorMidldeware(compositor)
    reactor.set_top(top)
    reactor.run()


def run_app(klass, bundle_path=None):
    run_view(klass(ApplicationContext(bundle_path)), debug_vm=True)


def _yeet_local_changes() -> None:
    os.remove("/flash/sys/.sys-installed")
    machine.reset()


def run_main() -> None:
    log.info(f"starting main")
    log.info(f"free memory: {gc.mem_free()}")

    captouch.calibration_request()

    audio.set_volume_dB(settings.num_startup_volume_db.value)
    audio.headphones_set_minimum_volume_dB(settings.num_headphones_min_db.value)
    audio.speaker_set_minimum_volume_dB(settings.num_speakers_min_db.value)
    audio.headphones_set_maximum_volume_dB(settings.num_headphones_max_db.value)
    audio.speaker_set_maximum_volume_dB(settings.num_speakers_max_db.value)

    leds.set_rgb(0, 255, 0, 0)
    leds.update()
    bundles = BundleManager()
    bundles.update()

    try:
        network.hostname(
            settings.str_hostname.value if settings.str_hostname.value else "flow3r"
        )
    except Exception as e:
        log.error(f"Failed to set hostname {e}")

    menu_settings = settings.build_menu()
    menu_system = SimpleMenu(
        [
            MenuItemBack(),
            MenuItemForeground("Settings", menu_settings),
            MenuItemAppLaunch(BundleMetadata("/flash/sys/apps/graphics_mode")),
            MenuItemAppLaunch(BundleMetadata("/flash/sys/apps/gr33nhouse")),
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
        requested = [b for b in bundles.bundles.values() if b.name == override_main_app]
        if len(requested) > 1:
            raise Exception(f"More than one bundle named {override_main_app}")
        if len(requested) == 0:
            raise Exception(f"Requested bundle {override_main_app} not found")
        run_view(requested[0].load(), debug_vm=True)
    run_view(menu_main, debug_vm=False)


__all__ = [
    "run_responder",
    "run_view",
    "run_main",
]
