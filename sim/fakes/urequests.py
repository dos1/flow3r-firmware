from typing import Any


class Response:
    text: str
    content: bytes
    status_code: int

    def close(self) -> None:
        ...

    def json(self) -> Any:
        ...


def request(
    method: str, url: str, data: Any, json: Any, headers: dict[str, str]
) -> Response:
    ...


def head(url: str, **kw: Any) -> Response:
    ...


def get(url: str, **kw: Any) -> Response:
    ...


def post(url: str, **kw: Any) -> Response:
    ...


def put(url: str, **kw: Any) -> Response:
    ...


def patch(url: str, **kw: Any) -> Response:
    ...


def delete(url: str, **kw: Any) -> Response:
    ...
