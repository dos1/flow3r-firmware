import hardware
from math import sin, cos, pi
from .menu_entry import MenuEntry

from .. import sadgfx as sg
from .. import theme

ctx = hardware.get_ctx()
active_petal = None

last_petal_touched = None
touch_start = None
touch_diff = None

selected_menu = None

menu_texts = ["SCREEN", "VOLUME", "BACK", "CLOSE", "FOO"]

menu1: list[MenuEntry] = [
    MenuEntry(label="SCREEN"),
    MenuEntry(label="VOLUME"),
    MenuEntry(label="BACK"),
    MenuEntry(label="CLOSE"),
    MenuEntry(label="BATTERY"),
]
menu2: list[MenuEntry] = [
    MenuEntry(label="MENU1"),
    MenuEntry(label="MENU2"),
    MenuEntry(label="BACK"),
    MenuEntry(label="CLOSE"),
    MenuEntry(label="MENU3"),
]
menu3: list[MenuEntry] = [
    MenuEntry(label="MENU1"),
    MenuEntry(label="MENU2"),
    MenuEntry(label="BACK"),
    MenuEntry(label="CLOSE"),
    MenuEntry(label="MENU3"),
]

current_menu = menu1


def init():
    reset_petals()


def on_petal_pressed(index: int) -> None:
    global active_petal
    active_petal = index


def on_petal_touched(index: int, rad: int, phi: int):
    global last_petal_touched
    global touch_start
    global touch_diff

    if last_petal_touched != index:
        reset_last_touch()
        last_petal_touched = index
    elif touch_start is None:
        touch_start = (rad, phi)
    else:
        diff_rad = touch_start[0] - rad
        diff_phi = touch_start[1] - phi
        touch_diff = (diff_rad, diff_phi)
        print(f"diff: {diff_rad} / {diff_phi}")
    pass


def on_petal_released(index: int) -> None:
    reset_last_touch()
    print(f"Petal released {index}")


def on_left_button_pressed(state) -> None:
    print(f"Left button pressed {state}")


def on_right_button_pressed(state) -> None:
    global current_menu
    current_menu = menu1
    print(f"Right button pressed {state}")


def reset_last_touch():
    global last_petal_touched
    global touch_start
    global touch_diff
    global current_menu

    print(f"Resetting last touch {touch_diff}")

    if touch_diff is not None and touch_diff[0] < -3:
        selected_label = current_menu[last_petal_touched].label

        # Hacky animation
        for i in range(20):
            draw_selected_menu(i, selected_label)

        if last_petal_touched == 3:
            # CLOSE
            current_menu = None
        elif current_menu == menu1:
            if last_petal_touched == 2:
                # BACK
                pass
            else:
                # OTHER
                current_menu = menu2
        elif current_menu == menu2:
            if last_petal_touched == 2:
                # BACK
                current_menu = menu1
            else:
                current_menu = menu3
        elif current_menu == menu3:
            if last_petal_touched == 2:
                # BACK
                current_menu = menu2
            else:
                pass

    last_petal_touched = None
    touch_start = None
    touch_diff = None


def draw():
    global active_petal
    global last_petal_touched

    if current_menu is None:
        sg.fill_screen(ctx, sg.BLACK)
    elif active_petal is not None:
        for i in range(5):
            if i == active_petal:
                draw_active_petal()
            else:
                draw_inactive_petal(i)

        draw_menu_label(current_menu[active_petal].label)
    else:
        reset_petals()
        for i in range(5):
            draw_inactive_petal(i)

    active_petal = None


def draw_active_petal():
    # print_int(active_petal)

    offset = active_petal * 8

    for i in range(40):
        hardware.set_led_rgb(i, 0, 0, 0)

    for i in range(offset, offset + 7):
        hardware.set_led_rgb(i, *sg.color_to_int(theme.PRIMARY, 0.1))

    petal_angle = 2.0 * pi / 5.0

    circle_x = cos(-petal_angle * float(active_petal) - pi / 2.0) * 120.0
    circle_y = sin(-petal_angle * float(active_petal) - pi / 2.0) * 120.0

    sg.draw_circle(ctx, get_menu_color(), circle_x, circle_y, 60)


def draw_inactive_petal(index: int) -> None:
    petal_angle = 2.0 * pi / 5.0

    circle_x = cos(-petal_angle * float(index) - pi / 2.0) * 120.0
    circle_y = sin(-petal_angle * float(index) - pi / 2.0) * 120.0

    sg.draw_circle(ctx, get_menu_color(), circle_x, circle_y, 60)
    sg.draw_circle(ctx, theme.BACKGROUND, circle_x, circle_y, 50)


def reset_petals():
    sg.fill_screen(ctx, theme.BACKGROUND)

    for i in range(40):
        hardware.set_led_rgb(i, 0, 0, 0)


def print_int(val: int):
    global ctx
    ctx.text_align = ctx.CENTER
    ctx.text_baseline = ctx.MIDDLE
    ctx.font_size = 30
    ctx.rgb(*theme.BACKGROUND_TEXT).move_to(0, 0).text(str(val))


def draw_menu_label(label: str) -> None:
    global ctx

    ctx.rgb(*theme.SECONDARY)
    ctx.round_rectangle(-100, -22, 200, 40, 20).fill()

    ctx.text_align = ctx.CENTER
    ctx.text_baseline = ctx.MIDDLE
    ctx.font_size = 40
    ctx.rgb(*theme.SECONDARY_TEXT).move_to(0, 0).text(label)


def draw_selected_menu(animation_step: int, label: str) -> None:
    # sg.fill_screen(ctx, theme.SECONDARY)

    sg.draw_circle(ctx, theme.SECONDARY, 0, 0, 50 + 10 * animation_step)

    ctx.text_align = ctx.CENTER
    ctx.text_baseline = ctx.MIDDLE
    ctx.font_size = 40
    ctx.rgb(*theme.SECONDARY_TEXT).move_to(0, 0).text(label)

    hardware.display_update()


def get_menu_color():
    if current_menu == menu1:
        return sg.ORANGE
    elif current_menu == menu2:
        return sg.RED1
    elif current_menu == menu3:
        return sg.BLUE1
