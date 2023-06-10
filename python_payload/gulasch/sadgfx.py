from math import sin, cos, pi

# TODO: Type alias for color
# TODO: Stub for Ctx

WHITE = (1.0, 1.0, 1.0)
BLACK = (0.0, 0.0, 0.0)
RED = (1.0, 0.0, 0.0)
RED1 = (0.98, 0.03, 0.0)
GREEN = (0.0, 1.0, 0.0)
BLUE1 = (0.0, 0.0, 1.0)
BLUE2 = (0.0, 0.6, 1.0)
YELLOW = (0.99, 0.82, 0.0)
ORANGE = (1.0, 0.4, 0.0)
PURPLE1 = (0.83, 0.0, 0.95)
PURPLE2 = (0.65, 0.0, 0.84)
PCB_GREEN = (0.2, 0.8, 0.5)
PCB_GREEN_DARK = (0.1, 0.4, 0.25)


def fill_screen(ctx, color) -> None:
    ctx.rgb(*color)
    ctx.rectangle(
        -120.0,
        -120.0,
        240.0,
        240.0,
    ).fill()


def draw_circle(ctx, color, x: int, y: int, radius: int) -> None:
    ctx.move_to(x, y)
    ctx.rgb(*color)
    ctx.arc(x, y, radius, -pi, pi, True)
    ctx.fill()


def color_to_int(color, scale=1.0):
    return tuple(int(i * 255.0 * scale) for i in color)
