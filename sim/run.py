import importlib
import importlib.machinery
from importlib.machinery import PathFinder, BuiltinImporter
import importlib.util
import os
import sys


projectpath = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

class Hook:
    """
    Hook implements a importlib.abc.Finder which overwrites resolution order to
    more closely match the resolution order on the badge's Micropython
    environment.
    """
    def find_spec(self, fullname, path, target=None):
        # Attempt to load from python_payload, then python_modules then
        # sim/fakes. Afterwards, the normal import resolution order kicks in.
        paths = [
            os.path.join(projectpath, 'python_payload', fullname+'.py'),
            os.path.join(projectpath, 'python_payload', fullname),
            os.path.join(projectpath, 'python_modules', fullname+'.py'),
            os.path.join(projectpath, 'python_modules', fullname),
            os.path.join(projectpath, 'sim', 'fakes', fullname+'.py'),
        ]
        for p in paths:
            if os.path.exists(p):
                root = os.path.split(p)[:-1]
                return PathFinder.find_spec(fullname, root)
        # As we provide our own micropython-compatible time library, allow
        # resolving the original CPython time through _time
        if fullname == '_time':
            return BuiltinImporter.find_spec('time')
        return None

sys.meta_path.insert(0, Hook())
sys.path_importer_cache.clear()

# Clean up whatever might have already been imported as `time`.
import time
importlib.reload(time)

import main
