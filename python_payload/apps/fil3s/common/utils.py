from math import pi
from ctx import Context


def fill_screen(ctx: Context, color: tuple[float, float, float]) -> None:
    ctx.rgb(*color)
    ctx.rectangle(
        -120.0,
        -120.0,
        240.0,
        240.0,
    ).fill()


def draw_circle(
    ctx: Context, color: tuple[float, float, float], x: int, y: int, radius: int
) -> None:
    ctx.move_to(x, y)
    ctx.rgb(*color)
    ctx.arc(x, y, radius, -pi, pi, True)
    ctx.fill()
