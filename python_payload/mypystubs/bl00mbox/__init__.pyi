from bl00mbox import patches

from typing import Optional, TypeVar, Any, Type

patches = patches

class Signal:
    value: int

class SignalList:
    def __setattr__(self, name: str, value: Signal) -> None: ...
    def __getattr__(self, name: str) -> Signal: ...

class Bud:
    signals: SignalList

T = TypeVar("T")
P = TypeVar("P", bound=patches._Patch)

class Channel:
    background_mute_override: bool
    def new(self, thing: Type[T], init_var: Optional[Any] = None) -> T: ...
    def new_patch(self, patch: Type[T], init_var: Optional[Any] = None) -> T: ...
