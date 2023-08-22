from typing import Any, Dict, Optional

class Response:
    text: str
    content: bytes
    status_code: int
    headers: Dict[str, str]

    def close(self) -> None: ...
    def json(self) -> Any: ...

def request(
    method: str,
    url: str,
    data: Any = None,
    json: Any = None,
    headers: Optional[Dict[str, str]] = None,
) -> Response: ...
def head(
    url: str,
    data: Any = None,
    json: Any = None,
    headers: Optional[Dict[str, str]] = None,
) -> Response: ...
def get(
    url: str,
    data: Any = None,
    json: Any = None,
    headers: Optional[Dict[str, str]] = None,
) -> Response: ...
def post(
    url: str,
    data: Any = None,
    json: Any = None,
    headers: Optional[Dict[str, str]] = None,
) -> Response: ...
def put(
    url: str,
    data: Any = None,
    json: Any = None,
    headers: Optional[Dict[str, str]] = None,
) -> Response: ...
def patch(
    url: str,
    data: Any = None,
    json: Any = None,
    headers: Optional[Dict[str, str]] = None,
) -> Response: ...
def delete(
    url: str,
    data: Any = None,
    json: Any = None,
    headers: Optional[Dict[str, str]] = None,
) -> Response: ...
