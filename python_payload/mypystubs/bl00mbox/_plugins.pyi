class _Plugin:
    index: int
    plugin_id: int
    name: str
    description: str

class _Plugins:
    def __getattr__(self, name: str) -> _Plugin: ...

plugins = _Plugins()
