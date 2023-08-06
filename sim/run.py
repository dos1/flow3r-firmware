import importlib
import importlib.abc
import importlib.machinery
from importlib.machinery import PathFinder, BuiltinImporter
import importlib.util
import os
import sys


projectpath = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

import random
import pygame
import cmath
import wasmer
import wasmer_compiler_cranelift


sys_path_orig = sys.path


class UnderscoreFinder(importlib.abc.MetaPathFinder):
    def __init__(self, builtin, pathfinder):
        self.builtin = builtin
        self.pathfinder = pathfinder

    def find_spec(self, fullname, path, target=None):
        if fullname == "_time":
            return self.builtin.find_spec("time", path, target)
        if fullname in ["random", "math"]:
            return self.builtin.find_spec(fullname, path, target)
        if fullname in ["json"]:
            sys_path_saved = sys.path
            sys.path = sys_path_orig
            res = self.pathfinder.find_spec(fullname, path, target)
            sys.path = sys_path_saved
            return res


# sys.meta_path.insert(0, Hook())

sys.path = [
    os.path.join(projectpath, "python_payload"),
    os.path.join(projectpath, "components", "micropython", "frozen"),
    os.path.join(projectpath, "sim", "fakes"),
]

builtin = BuiltinImporter()
pathfinder = PathFinder()
underscore = UnderscoreFinder(builtin, pathfinder)
sys.meta_path = [pathfinder, underscore]

# Clean up whatever might have already been imported as `time`.
import time

importlib.reload(time)

sys.path_importer_cache.clear()
importlib.invalidate_caches()

sys.modules["time"] = time

simpath = "/tmp/flow3r-sim"
print(f"Using {simpath} as /flash mount")
try:
    os.mkdir(simpath)
except:
    pass


def _path_replace(p):
    if p.startswith("/flash/sys"):
        p = p[len("/flash/sys") :]
        p = projectpath + "/python_payload" + p
        return p
    if p.startswith("/flash"):
        p = p[len("/flash") :]
        p = simpath + p
        return p

    return p


_listdir_orig = os.listdir


def _listdir_override(path):
    return _listdir_orig(_path_replace(path))


os.listdir = _listdir_override

import builtins

_open_orig = builtins.open


def _open_override(file, *args, **kwargs):
    file = _path_replace(file)
    return _open_orig(file, *args, **kwargs)


builtins.open = _open_override

import main
