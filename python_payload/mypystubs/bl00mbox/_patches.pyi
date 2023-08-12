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

    def trigger_state(self, track: int, i: int) -> bool: ...
    def trigger_toggle(self, track: int, i: int) -> None: ...

class sampler(_Patch): ...

class fuzz(_Patch):
    volume: int
    intensity: float
    gate: int

class karplus_strong(_Patch):
    decay: int
