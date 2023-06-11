import importlib
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


class UnderscoreFinder(importlib.abc.MetaPathFinder):
    def __init__(self, builtin):
        self.builtin = builtin

    def find_spec(self, fullname, path, target=None):
        if fullname == '_time':
            return self.builtin.find_spec('time', path, target)
        if fullname in ['random']:
            print(fullname, path, target)
            return self.builtin.find_spec(fullname, path, target)


#sys.meta_path.insert(0, Hook())

sys.path = [
    os.path.join(projectpath, 'python_payload'),
    os.path.join(projectpath, 'sim', 'fakes'),
]

builtin = BuiltinImporter()
pathfinder = PathFinder()
underscore = UnderscoreFinder(builtin)
sys.meta_path = [pathfinder, underscore]

# Clean up whatever might have already been imported as `time`.
import time
print('aaaa', time)
importlib.reload(time)
print('bbbb', time)

sys.path_importer_cache.clear()
importlib.invalidate_caches()

sys.modules['time'] = time

import main