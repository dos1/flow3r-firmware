import sys
import time
from st3m.goose import Enum


class Level(Enum):
    DEBUG = 0
    INFO = 1
    WARNING = 2
    ERROR = 3


DEBUG = Level.DEBUG
INFO = Level.INFO
WARNING = Level.WARNING
ERROR = Level.ERROR

_log_levels = {
    DEBUG: 0,
    INFO: 1,
    WARNING: 2,
    ERROR: 3,
}

_log_level_names = {
    DEBUG: "DEBUG",
    INFO: "INFO",
    WARNING: "WARNING",
    ERROR: "ERROR",
}


class Log:
    def __init__(self, name: str = "log", level: Level = INFO):
        self.name = name
        self.level = level
        self.logstring = "{timestamp} {name} ({level}): {msg}"

    def debug(self, msg: str) -> None:
        self.message(msg, DEBUG)

    def info(self, msg: str) -> None:
        self.message(msg, INFO)

    def warning(self, msg: str) -> None:
        self.message(msg, WARNING)

    def error(self, msg: str) -> None:
        self.message(msg, ERROR)

    def message(self, msg: str, level: Level) -> None:
        if _log_levels[self.level] <= _log_levels[level]:
            self._emit(
                self.logstring.format(
                    timestamp=time.ticks_ms() / 1000,
                    name=self.name,
                    msg=msg,
                    level=_log_level_names[level],
                )
            )

    def _emit(self, line: str) -> None:
        print(line)
