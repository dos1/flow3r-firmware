from st3m.application import SimpleApplication

app = SimpleApplication("flow3rdroids")


def on_init():
    ship_position = complex(0, 0)
    ship_direction = complex(0, 0)


def on_tick():
    if abs(ship_position.real) > 240:
        ship_position.real *= -1
    if abs(ship_position.imag) > 240:
        ship_position.imag *= -1

    ship_position += ship_direction


def on_draw():
    ui.Pointer(x=ship_position.imag, y=ship_position.real)


def on_scroll_step(direction):
    # TODO: ship_direction += direction * angle
    pass


def on_enter():
    # TODO:ship_direction += speed
    pass
