from st3m.goose import Optional

STA_IF: int

class WLAN:
    def __init__(self, mode: int) -> None:
        pass
    def active(self, active: bool) -> bool:
        return False
    def connect(self, ssid: bytes, key: Optional[bytes] = None) -> None:
        pass
    def isconnected(self) -> bool:
        return False
