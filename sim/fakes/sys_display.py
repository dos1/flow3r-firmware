import _sim


def pipe_full():
    return False


def overlay_clip(x0, y0, x1, y1):
    pass


def get_mode():
    return 0


def set_mode(no):
    pass


update = _sim.display_update
get_ctx = _sim.get_ctx
get_overlay_ctx = _sim.get_overlay_ctx
osd = 256


def ctx(foo):
    return _sim.get_ctx()
