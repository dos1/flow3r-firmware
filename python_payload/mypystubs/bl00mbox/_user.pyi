from typing import Optional, TypeVar, Any, Type, List, overload, Annotated
import bl00mbox

class Signal:
    name: str
    description: str
    unit: str
    hints: str
    connections: List["Signal | ChannelMixer"]

    # mypy doesn't know how to handle asymmetric setters
    # https://github.com/python/mypy/issues/3004
    value: Any
    # value: None | int | float | "Signal" | "ChannelMixer"

    def __init__(self, plugin: "Plugin", signal_num: int): ...

    # SignalInputTrigger functions
    def start(self, velocity: int = 32767) -> None: ...
    def stop(self) -> None: ...

    # SignalInputPitch shorthands
    freq: int
    tone: Any
    # tone: int | float | str

class SignalList:
    def __setattr__(self, name: str, value: Signal | int | "ChannelMixer") -> None: ...
    def __getattr__(self, name: str) -> Signal: ...

class Plugin:
    signals: SignalList
    table: Annotated[List[int], 129]

    def __init__(
        self,
        channel: "Channel",
        plugin_id: int,
        init_var: Optional[int] = 0,
        bud_num: Optional[int] = None,
    ): ...

T = TypeVar("T")
P = TypeVar("P", bound=bl00mbox.patches._Patch)

class Channel:
    background_mute_override: bool
    mixer: "ChannelMixer"
    foreground: bool
    free: bool
    volume: int

    def __init__(self, name: str): ...
    def clear(self) -> None: ...
    def _new_plugin(
        self,
        thing: bl00mbox._plugins._Plugin | int,
        init_var: Optional[int | float] = None,
    ) -> Plugin: ...
    @overload
    def new(self, thing: Type[P], *args: Any, **kwargs: Any) -> P: ...
    @overload
    def new(
        self,
        thing: bl00mbox._plugins._Plugin | int,
        init_var: Optional[int | float] = None,
    ) -> Plugin: ...

class ChannelMixer:
    _channel: Channel
    connections: List[Signal]

    def _unplug_all(self) -> None: ...
