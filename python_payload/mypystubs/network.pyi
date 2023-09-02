from st3m.goose import Optional, Tuple, List, Any
from typing import overload

STA_IF: int

class WLAN:
    def __init__(self, mode: int) -> None: ...
    def active(self, active: Optional[bool] = None) -> bool: ...
    def connect(self, ssid: bytes, key: Optional[bytes] = None) -> None: ...
    def isconnected(self) -> bool: ...
    def status(self, s: str) -> float: ...
    def scan(self) -> List[Tuple[bytes, bytes, int, int, int, bool]]: ...
    @overload
    def ifconfig(self) -> Tuple[str, str, str, str]: ...
    @overload
    def ifconfig(self, cfg: Optional[Tuple[str, str, str, str]]) -> None: ...
    @overload
    def config(self, setting: str) -> Any: ...
    @overload
    def config(
        self,
        *,
        mac: bytes = b"",
        ssid: str = "",
        channel: int = 0,
        hidden: bool = False,
        security: int = 0,
        key: str = "",
        reconnects: int = 0,
        txpower: float = 0.0,
        pm: int = 0,
    ) -> None: ...

@overload
def country() -> str: ...
@overload
def country(country_code: str) -> None: ...
@overload
def hostname() -> str: ...
@overload
def hostname(hostname: str) -> None: ...
