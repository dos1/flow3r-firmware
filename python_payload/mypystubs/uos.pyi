from typing import NewType

StatStruct = NewType(
    "StatStruct", tuple[int, int, int, int, int, int, int, int, int, int]
)

def stat(path: str) -> StatStruct:
    """
    Get the status of a file or directory.

    Returns a tuple with 10 elements:

    - st_mode

    - st_ino

    - st_dev

    - st_nlink

    - sb.st_uid

    - sb.st_gid

    - sb.st_size

    - sb.st_atime

    - sb.st_mtime

    - sb.st_ctime
    """
