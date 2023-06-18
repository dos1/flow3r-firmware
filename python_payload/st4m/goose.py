"""
Goose implements Goose Typing (tm), a set of shims which allows the st4m
codebase to use both Python type annotations, Abstract Base Classes and run
under Micropython.
"""

# Detect whether we're in MyPy or just executing.
try:
    from typing import TYPE_CHECKING
except:
    TYPE_CHECKING = False

if TYPE_CHECKING:
    # We're in MyPy.
    from abc import ABCMeta, abstractmethod
    class ABCBase(metaclass=ABCMeta):
        pass

    from typing import List, Optional
    from enum import Enum
else:
    # We're in CPython or Micropython.
    class ABCBase:
        pass

    def abstractmethod(f):
        def _fail():
            raise Exception("abstract method call")
        return _fail

    try:
        from typing import List, Optional
        from enum import Enum
    except ImportError:
        # We're in Micropython.
        List = None
        Optional = None

        class Enum:
            pass

__all__ = [
    'TYPE_CHECKING',
    'ABCBase', 'abstractmethod',

    'List', 'Optional',

    'Enum',
]