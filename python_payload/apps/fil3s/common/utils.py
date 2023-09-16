from math import tau
from ctx import Context
import uos
import stat


def fill_screen(ctx: Context, color: tuple[float, float, float]) -> None:
    ctx.rgb(*color)
    ctx.rectangle(
        -120.0,
        -120.0,
        240.0,
        240.0,
    ).fill()


def draw_circle(
    ctx: Context,
    color: tuple[float, float, float],
    x: int,
    y: int,
    radius: int,
    progress: float = 0.0,
) -> None:
    ctx.move_to(x, y)
    ctx.rgb(*color)
    ctx.arc(x, y, radius, tau * progress, tau, 0)
    ctx.fill()


def is_dir(path: str) -> bool:
    st_mode = uos.stat(path)[0]  # Can fail with OSError
    return stat.S_ISDIR(st_mode)


def rmdirs(base_path):
    for entry in uos.listdir(base_path):
        path = f"{base_path}/{entry}"
        if is_dir(path):
            rmdirs(path)
        else:
            uos.remove(path)
            print(f"deleted file: {path}")
    uos.rmdir(base_path)
    print(f"deleted folder: {base_path}")
