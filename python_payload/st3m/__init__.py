from st3m.reactor import Reactor, Responder
from st3m.input import InputState, InputController
from st3m.goose import Optional
from ctx import Context, RGBA8


class Texture:
    """
    An off-screen texture with an attached Context.

    Texture data buffer lives as long as the Texture object itelf, allowing for
    progressive/partial drawing using ctx. Then, the texture can be drawn to a
    main ctx (eg. in a Responder draw call).

    TODO(q3k): Currently textures are always full-screen.
    """

    def __init__(self) -> None:
        self._buffer = bytearray(240 * 240 * 4)
        self._ctx = Context(
            width=240, height=240, stride=240 * 4, format=RGBA8, buffer=self._buffer
        )
        self._ctx.apply_transform(1, 0, 120, 0, 1, 120, 0, 0, 1)

    @property
    def ctx(self) -> Context:
        return self._ctx

    def draw(self, ctx: Context) -> None:
        eid = ctx.texture(self._buffer, RGBA8, 240, 240, 240 * 4)
        ctx.draw_texture(eid)


__all__ = [
    "Reactor",
    "Responder",
    "InputState",
    "InputController",
]
