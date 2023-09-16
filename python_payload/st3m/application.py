from st3m.ui.view import (
    BaseView,
    ViewTransitionSwipeLeft,
    ViewManager,
)
from st3m.ui.menu import MenuItem
from st3m.input import InputState
import st3m.wifi
from st3m.goose import Optional, List, Dict
from st3m.logging import Log
from st3m import settings
from ctx import Context

import toml
import os
import os.path
import stat
import sys
import sys_display
import random
from math import sin

log = Log(__name__)


class ApplicationContext:
    """
    Container for application context.

    Further envisioned are: path to bundle data,
    path to a data directory, etc...
    """

    _bundle_path: str
    _bundle_metadata: dict

    def __init__(self, bundle_path: str = "", bundle_metadata: dict = None) -> None:
        self._bundle_path = bundle_path
        self._bundle_metadata = bundle_metadata

    @property
    def bundle_path(self) -> str:
        return self._bundle_path

    @property
    def bundle_metadata(self) -> str:
        return self._bundle_metadata


class Application(BaseView):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        self._app_ctx = app_ctx
        if app_ctx and app_ctx.bundle_metadata and settings.onoff_wifi_preference.value:
            self._wifi_preference = app_ctx.bundle_metadata["app"].get(
                "wifi_preference"
            )
        else:
            self._wifi_preference = None
        super().__init__()

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        # Try to connect/disconnect from wifi if requested by app
        if self._wifi_preference is True and not st3m.wifi.is_connected():
            st3m.wifi.setup_wifi()
        elif self._wifi_preference is False:
            st3m.wifi.disable()
        super().on_enter(vm)

    def on_exit(self) -> None:
        fully_exiting = not self.vm._history or not isinstance(
            self.vm._history[-1], type(self)
        )
        # If the app requested to change wifi state
        # fall back to system defaults on exit
        if fully_exiting and self._wifi_preference is not None:
            st3m.wifi._onoff_wifi_update()
        super().on_exit()
        # set the default graphics mode, this is a no-op if
        # it is already set
        if fully_exiting:
            sys_display.set_mode(0)

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)


class BundleLoadException(BaseException):
    MSG = "failed to load"

    def __init__(self, msg: Optional[str] = None) -> None:
        res = self.MSG
        if msg is not None:
            res += ": " + msg
        self.msg = res
        super().__init__(res)


class BundleMetadataNotFound(BundleLoadException):
    MSG = "flow3r.toml not found"


class BundleMetadataCorrupt(BundleLoadException):
    MSG = "flow3r.toml corrupt"


class BundleMetadataBroken(BundleLoadException):
    MSG = "flow3r.toml broken"


class BundleMetadata:
    """
    Collects data from a flow3r.toml-defined 'bundle', eg. a redistribuable app.

    A flow3r.toml file contains the following sections:

       [app]
       # Required, displayed in the menu.
       name = "Name of the application"
       # One of "Apps", "Badge", "Music". Picks which menu the bundle's class
       # will be loadable from.
       menu = "Apps"

       [entry]
       # Required for app to actually load. Defines the name of the class that
       # will be imported from the __init__.py next to flow3r.toml. The class
       # must inherit from st3m.application.Application.
       class = "DemoApp"

       # Optional, but recommended. Might end up getting displayed somewhere in
       # a distribution web page or in system menus.
       [metadata]
       author = "Hans Acker"
       # A SPDX-compatible license identifier.
       license = "..."
       url = "https://example.com/demoapp"

    This data is used to discover bundles and load them as applications.
    """

    __slots__ = ["path", "name", "menu", "_t", "version"]

    def __init__(self, path: str) -> None:
        self.path = path.rstrip("/")
        try:
            f = open(self.path + "/flow3r.toml")
        except Exception:
            raise BundleMetadataNotFound()

        try:
            t = toml.load(f)
        except toml.TomlDecodeError as e:
            f.close()
            raise BundleMetadataCorrupt(str(e))
        except Exception as e:
            f.close()
            raise BundleMetadataCorrupt(str(e))
        f.close()

        if "app" not in t or type(t["app"]) != dict:
            raise BundleMetadataBroken("missing app section")

        app = t["app"]
        if "name" not in app or type(app["name"]) != str:
            raise BundleMetadataBroken("missing app.name key")
        self.name = app["name"]
        if "menu" not in app or type(app["menu"]) != str:
            raise BundleMetadataBroken("missing app.menu key")
        self.menu = app["menu"]
        if self.menu not in ["Apps", "Music", "Badge", "Hidden"]:
            raise BundleMetadataBroken("app.menu must be either Apps, Music or Badge")

        version = 0
        if t.get("metadata") is not None:
            version = t["metadata"].get("version", 0)
        self.version = version

        self._t = t

    @staticmethod
    def _sys_path_set(v: List[str]) -> None:
        # Can't just assign to sys.path in Micropython.
        sys.path.clear()
        for el in v:
            sys.path.append(el)

    def _load_class(self, class_entry: str) -> Application:
        # Micropython doesn't have a good importlib-like API for doing dynamic
        # imports of modules at custom paths. That means we have to, for now,
        # resort to good ol' sys.path manipulation.
        #
        # TODO(q3k): extend micropython to make this less messy
        old_sys_path = sys.path[:]

        log.info(f"Loading {self.name} via class entry {class_entry}...")
        containing_path = os.path.dirname(self.path)
        package_name = os.path.basename(self.path)

        if sys.path[0].endswith("python_payload"):
            # We are in the simulator. Hack around to get this to work.
            prefix = "/flash/sys"
            assert containing_path.startswith(prefix)
            containing_path = containing_path.replace(prefix, sys.path[0])

        new_sys_path = old_sys_path + [containing_path]
        self._sys_path_set(new_sys_path)
        try:
            m = __import__(package_name)
            self._sys_path_set(old_sys_path)
            log.info(f"Loaded {self.name} module: {m}")
            klass = getattr(m, class_entry)
            log.info(f"Loaded {self.name} class: {klass}")
            inst = klass(ApplicationContext(self.path, self._t))
            log.info(f"Instantiated {self.name} class: {inst}")
            return inst  # type: ignore
        except Exception as e:
            self._sys_path_set(old_sys_path)
            raise BundleLoadException(f"load error: {e}")

    def load(self) -> Application:
        """
        Return Application loaded form this bundle.

        Raises a BundleMetadataException if something goes wrong.
        """
        entry = self._t.get("entry", None)
        if entry is None:
            return self._load_class("App")
        if "class" in entry and type(entry["class"]) == str:
            class_entry = entry["class"]
            return self._load_class(class_entry)

        raise BundleMetadataBroken("no valid entry method specified")

    def menu_entries(self, kind: str) -> List["MenuItemAppLaunch"]:
        """
        Returns MenuItemAppLauch entries for this bundle for a given menu kind.

        Kind is one of 'Apps', 'Badge', 'Music'.
        """
        if self.menu != kind:
            return []
        return [MenuItemAppLaunch(self)]

    @property
    def source(self) -> str:
        return os.path.dirname(self.path)

    @property
    def id(self) -> str:
        return os.path.basename(self.path)

    def __repr__(self) -> str:
        return f"<BundleMetadata: {self.id} at {self.path}>"


class LoadErrorView(BaseView):
    def __init__(self, e: BundleLoadException) -> None:
        super().__init__()
        self.e = e
        self.header = "oh no"

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        self.header = random.choice(
            [
                "oh no",
                "aw shucks",
                "whoopsie",
                "ruh-roh",
                "aw crud",
            ]
        )

    def think(self, ins: InputState, delta_ms: int) -> None:
        pass

    def draw(self, ctx: Context) -> None:
        ctx.rgb(0.8, 0.1, 0.1)
        ctx.rectangle(-120, -120, 240, 240)
        ctx.fill()

        ctx.gray(1)
        ctx.font_size = 20
        ctx.font = "Camp Font 1"
        ctx.text_align = ctx.MIDDLE
        ctx.move_to(0, -70)
        ctx.text(self.header)

        lines: List[List[str]] = []
        msg = self.e.msg
        for word in msg.split():
            if len(lines) == 0:
                lines.append([word])
                continue
            lastline = lines[-1][:]
            lastline.append(word)
            if sum(len(l) for l in lastline) + len(lastline) - 1 > 30:
                lines.append([word])
            else:
                lines[-1].append(word)

        ctx.gray(0)
        ctx.rectangle(-120, -60, 240, 240).fill()
        y = -40
        ctx.gray(1)
        ctx.font_size = 15
        ctx.font = "Arimo Regular"
        ctx.text_align = ctx.LEFT
        for line in lines:
            ctx.move_to(-90, y)
            ctx.text(" ".join(line))
            y += 15


class MenuItemAppLaunch(MenuItem):
    """
    A MenuItem which launches an app from a BundleMetadata.

    The underlying app class is imported and instantiated on first use.
    """

    def __init__(self, bundle: BundleMetadata):
        self._bundle = bundle
        self._instance: Optional[Application] = None
        self._scroll_pos = 0.0
        self._highlighted = False

    def press(self, vm: Optional[ViewManager]) -> None:
        if vm is None:
            log.warning(f"Could not launch {self.label()} as no ViewManager is present")
            return

        if self._instance is None:
            try:
                self._instance = self._bundle.load()
            except BundleLoadException as e:
                log.error(f"Could not load {self.label()}: {e}")
                err = LoadErrorView(e)
                vm.push(err)
                return
        assert self._instance is not None
        vm.push(self._instance, ViewTransitionSwipeLeft())

    def label(self) -> str:
        return self._bundle.name

    def highlight(self, active: bool) -> None:
        self._highlighted = active
        self._scroll_pos = 0.0

    def draw(self, ctx: Context) -> None:
        ctx.save()
        if self._highlighted and (width := ctx.text_width(self.label())) > 220:
            ctx.translate(sin(self._scroll_pos) * (width - 220) / 2, 0)
        super().draw(ctx)
        ctx.restore()

    def think(self, ins: InputState, delta_ms: int) -> None:
        if self._highlighted:
            self._scroll_pos += delta_ms / 1000


class BundleManager:
    """
    The BundleManager maintains information about BundleMetadata at different
    locations in the badge filesystem.

    It also manages updating/reloading bundles.
    """

    def __init__(self) -> None:
        self.bundles: Dict[str, BundleMetadata] = {}

    @staticmethod
    def _source_trumps(a: str, b: str) -> bool:
        prios = {
            "/flash/sys/apps": 200,
            "/sd/apps": 120,
            "/flash/apps": 100,
        }
        prio_a = prios.get(a, 0)
        prio_b = prios.get(b, 0)
        return prio_a > prio_b

    def _discover_at(self, path: str) -> None:
        path = path.rstrip("/")
        try:
            l = os.listdir(path)
        except Exception as e:
            log.warning(f"Could not discover bundles in {path}: {e}")
            l = []

        for d in l:
            dirpath = path + "/" + d
            st = os.stat(dirpath)
            if not stat.S_ISDIR(st[0]):
                continue

            tomlpath = dirpath + "/flow3r.toml"
            try:
                st = os.stat(tomlpath)
                if not stat.S_ISREG(st[0]):
                    continue
            except Exception:
                continue

            try:
                b = BundleMetadata(dirpath)
            except BundleLoadException as e:
                log.error(f"Failed to bundle from {dirpath}: {e}")
                continue

            bundle_name = b.name
            if bundle_name not in self.bundles:
                self.bundles[bundle_name] = b
                continue
            ex = self.bundles[bundle_name]

            # Do we have a newer version?
            if b.version > ex.version:
                self.bundles[bundle_name] = b
                continue
            # Do we have a higher priority source?
            if self._source_trumps(b.source, ex.source):
                self.bundles[bundle_name] = b
                continue
            log.warning(
                f"Ignoring {bundle_name} at {b.source} as it already exists at {ex.source}"
            )

    def update(self) -> None:
        self._discover_at("/flash/sys/apps")
        self._discover_at("/flash/apps")
        self._discover_at("/sd/apps")


def discover_bundles(path: str) -> List[BundleMetadata]:
    """
    Discover valid bundles (directories containing flow3r.toml) inside a given
    path.

    Only direct descendents will be checks - this function doesn't check
    directories recursively.

    Invalid bundles will be skipped and an error will be logged.
    """
    path = path.rstrip("/")
    try:
        l = os.listdir(path)
    except Exception as e:
        log.warning(f"Could not discover bundles in {path}: {e}")
        l = []

    bundles = []
    for d in l:
        dirpath = path + "/" + d
        st = os.stat(dirpath)
        if not stat.S_ISDIR(st[0]):
            continue

        tomlpath = dirpath + "/flow3r.toml"
        try:
            st = os.stat(tomlpath)
            if not stat.S_ISREG(st[0]):
                continue
        except Exception:
            continue

        try:
            b = BundleMetadata(dirpath)
        except BundleLoadException as e:
            log.error(f"Failed to bundle from {dirpath}: {e}")
            continue
        bundles.append(b)
    return bundles
