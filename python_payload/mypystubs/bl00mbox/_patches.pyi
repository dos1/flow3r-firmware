import bl00mbox

from typing import List

class _Patch:
    plugins: _PatchPluginList
    signals: _PatchSignalList

class _PatchSignalList:
    def __getattr__(self, name: str) -> bl00mbox.Signal: ...
    def __setattr__(
        self,
        name: str,
        value: int | float | bl00mbox.Signal | bl00mbox.ChannelMixer,
    ) -> None: ...

class _PatchPluginList:
    def __getattr__(self, name: str) -> bl00mbox.Plugin: ...
    def __setattr__(
        self,
        name: str,
        value: bl00mbox.Plugin,
    ) -> None: ...

class tinysynth(_Patch): ...
class tinysynth_fm(tinysynth): ...

class sequencer(_Patch):
    bpm: int

    def trigger_start(self, track: int, step: int, val: int = 32767) -> None: ...
    def trigger_stop(self, track: int, step: int, val: int = 32767) -> None: ...
    def trigger_clear(self, track: int, step: int) -> None: ...
    def trigger_state(self, track: int, step: int) -> int: ...
    def trigger_toggle(self, track: int, step: int) -> None: ...

class sampler(_Patch): ...

class fuzz(_Patch):
    volume: int
    intensity: float
    gate: int

class karplus_strong(_Patch):
    decay: int
