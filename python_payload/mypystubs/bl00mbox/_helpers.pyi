import bl00mbox
from typing import Optional, Callable

def terminal_scope(
    signal: bl00mbox.Signal,
    signal_min: int,
    signal_max: int,
    delay_ms: int,
    width: int,
    fun: Optional[Callable[[], None]],
    fun_ms: Optional[int],
) -> None: ...
def sct_to_note_name(sct: int) -> str: ...
def note_name_to_sct(name: str) -> int: ...
def sct_to_freq(sct: int) -> int: ...
