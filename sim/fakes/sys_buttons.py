import _sim


def get_left():
    return _sim.get_button_state(1)


def get_right():
    return _sim.get_button_state(0)


PRESSED_LEFT = -1
PRESSED_RIGHT = 1
PRESSED_DOWN = 2
NOT_PRESSED = 0
