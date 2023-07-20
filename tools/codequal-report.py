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
    column: Union[int, None]


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


def parse_line(s: str, nocol: bool = False) -> Union[Issue, None]:
    nparts = 4
    if nocol:
        nparts = 3
    parts = s.split(":", nparts)
    path = parts[0]
    path = path.removeprefix(os.getcwd() + "/")
    line = int(parts[1])
    if nocol:
        col = None
        level = parts[2].strip()
        rest = parts[3].strip()
    else:
        level = parts[3].strip()
        col = int(parts[2])
        rest = parts[4].strip()
    severity: Severity = {
        "warning": Severity.major,
        "error": Severity.blocker,
    }.get(level, Severity.info)
    check = ""
    if rest.endswith("]"):
        rest, check = rest.split("[")
        rest = rest.strip()
        check = check.rstrip("]")
    location = Location(
        path=path, positions=Positions(begin=Position(line=line, column=col))
    )
    fp = hashlib.md5(f"{check} {path} {line}".encode()).hexdigest()
    return Issue(
        description=rest,
        check_name=check,
        severity=severity,
        location=location,
        fingerprint=fp,
    )


def mypy(p: str) -> List[Issue]:
    res: List[Issue] = []
    with open(p) as f:
        for line in f:
            line = line.strip()
            if line.startswith("Found "):
                continue
            if line.startswith("Success: "):
                continue
            v = parse_line(line, nocol=True)
            if v is not None:
                res.append(v)
    return res


def clang_tidy(p: str) -> List[Issue]:
    res: List[Issue] = []
    with open(p) as f:
        in_checks = False
        in_list = False
        in_context = False
        for line in f:
            line = line.strip()
            if line == "Enabled checks:":
                in_checks = True
                continue

            if in_checks and not line:
                in_list = True
                in_checks = False
                continue

            if in_list:
                if line.split()[0].endswith("clang-tidy"):
                    in_context = False
                    continue
                if not in_context:
                    v = parse_line(line)
                    if v is not None:
                        res.append(v)
                    in_context = True
    return res


def main() -> None:
    if len(sys.argv) < 3:
        sys.stderr.write(f"Usage: {sys.argv[0]} (clang-tidy|mypy) warnings.txt\n")
        sys.stderr.flush()
        sys.exit(1)

    kind = sys.argv[1]
    path = sys.argv[2]
    if kind == "clang-tidy":
        res = clang_tidy(path)
    elif kind == "mypy":
        res = mypy(path)
    else:
        sys.stderr.write(f"Unknown kind {kind}\n")
        sys.stderr.flush()
        sys.exit(1)

    json.dump(res, sys.stdout, cls=MagicJSONEncoder)
    if res != []:
        sys.exit(1)


if __name__ == "__main__":
    main()
