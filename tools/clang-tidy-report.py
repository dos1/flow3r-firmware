#!/usr/bin/env python3
import sys
import os
import enum
import json
import dataclasses
import hashlib
from dataclasses import dataclass
from typing import Literal, Union, List, Any


class MagicJSONEncoder(json.JSONEncoder):
    def default(self, o: Any) -> Any:
        if dataclasses.is_dataclass(o):
            return dataclasses.asdict(o)
        return super().default(o)


@dataclass
class Position:
    line: int
    column: int


@dataclass
class Positions:
    begin: Position


@dataclass
class Location:
    path: str
    positions: Positions


class Severity(str, enum.Enum):
    info = "info"
    minor = "minor"
    major = "major"
    critical = "critical"
    blocker = "blocker"


@dataclass
class Issue:
    description: str
    check_name: Union[str, None]
    severity: Severity
    location: Location
    fingerprint: str


def parse_line(s: str) -> Union[Issue, None]:
    parts = s.split(':', 4)
    path = parts[0]
    path = path.removeprefix(os.getcwd() + '/')
    line = int(parts[1])
    col = int(parts[2])
    level = parts[3].strip()
    severity: Severity = {
        'warning': Severity.major,
        'error': Severity.blocker,
    }.get(level, Severity.info)
    rest = parts[4].strip()
    check = ''
    if rest.endswith(']'):
        rest, check = rest.split('[')
        rest = rest.strip()
        check = check.rstrip(']')
    location = Location(path=path, positions=Positions(begin=Position(line=line, column=col)))
    fp = hashlib.md5(f'{check} {path} {line}'.encode()).hexdigest()
    return Issue(description=rest, check_name=check, severity=severity, location=location, fingerprint=fp)


def main() -> None:
    if len(sys.argv) < 2:
        sys.stderr.write(f"Usage: {sys.argv[0]} warnings.txt\n")
        sys.stderr.flush()
        sys.exit(1)

    res: List[Issue] = []
    with open(sys.argv[1]) as f:
        in_checks = False
        in_list = False
        in_context = False
        for line in f:
            line = line.strip()
            if line == 'Enabled checks:':
                in_checks = True
                continue

            if in_checks and not line:
                in_list = True
                in_checks = False
                continue

            if in_list:
                if line.split()[0].endswith('clang-tidy'):
                    in_context = False
                    continue
                if not in_context:
                    v = parse_line(line)
                    if v is not None:
                        res.append(v)
                    in_context = True
    json.dump(res, sys.stdout, cls=MagicJSONEncoder)
    if res != []:
        sys.stderr.write("Clang-tidy report not empty")
        sys.stderr.flush()
        os.exit(1)

if __name__ == '__main__':
    main()
