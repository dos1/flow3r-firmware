from st3m.ui.view import (
    BaseView,
    ViewTransitionSwipeRight,
    ViewTransitionSwipeLeft,
    ViewManager,
)
from st3m.ui.menu import MenuItem
from st3m.input import InputState
from st3m.goose import Optional, List, Enum
from st3m.logging import Log

import toml
import os
import os.path
import stat
import sys

log = Log(__name__)


class ApplicationContext:
    """
    Container for future application context.

    Envisioned are: path to the bundle, bundle data,
    path to a data directory, etc...
    """

    pass


class Application(BaseView):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        self._app_ctx = app_ctx
        super().__init__()

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)


class BundleLoadException(BaseException):
    MSG = "failed to load"

    def __init__(self, msg: Optional[str] = None) -> None:
        res = self.MSG
        if msg is not None:
            res += ": " + msg
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

    __slots__ = ["path", "name", "menu", "_t"]

    def __init__(self, path: str) -> None:
        self.path = path.rstrip("/")
        try:
            f = open(self.path + "/flow3r.toml")
        except Exception:
            raise BundleMetadataNotFound()

        try:
            t = toml.load(f)
        except Exception as e:
            raise BundleMetadataCorrupt(str(e))

        if "app" not in t or type(t["app"]) != dict:
            raise BundleMetadataBroken("missing app section")

        app = t["app"]
        if "name" not in app or type(app["name"]) != str:
            raise BundleMetadataBroken("missing app.name key")
        self.name = app["name"]
        if "menu" not in app or type(app["menu"]) != str:
            raise BundleMetadataBroken("missing app.menu key")
        self.menu = app["menu"]
        if self.menu not in ["Apps", "Music", "Badge"]:
            raise BundleMetadataBroken("app.menu must be either Apps, Music or Badge")

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
            inst = klass(ApplicationContext())
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
            raise BundleMetadataBroken("missing entry section")
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

    def __repr__(self) -> str:
        return f"<BundleMetadata: {self.name} at {self.path}>"


class MenuItemAppLaunch(MenuItem):
    """
    A MenuItem which launches an app from a BundleMetadata.

    The underlying app class is imported and instantiated on first use.
    """

    def __init__(self, bundle: BundleMetadata):
        self._bundle = bundle
        self._instance: Optional[Application] = None

    def press(self, vm: Optional[ViewManager]) -> None:
        if vm is None:
            log.warning(f"Could not launch {self.label()} as no ViewManager is present")
            return

        if self._instance is None:
            try:
                self._instance = self._bundle.load()
            except BundleLoadException as e:
                log.error(f"Could not load {self.label()}: {e}")
                return
        assert self._instance is not None
        vm.push(self._instance, ViewTransitionSwipeLeft())

    def label(self) -> str:
        return self._bundle.name


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
