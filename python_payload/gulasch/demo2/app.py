import hardware
from math import sin, cos, pi
from .menu_entry import MenuEntry

from .. import sadgfx as sg
from .. import theme

ctx = hardware.get_ctx()
active_petal = None

last_petal_touched = None
touch_ticks = None

selected_menu = None

menu_texts = ["SCREEN", "VOLUME", "BACK", "CLOSE", "FOO"]

menu1: list[MenuEntry] = [
    MenuEntry(label="SCREEN", icon="S"),
    MenuEntry(label="VOLUME", icon="V"),
    MenuEntry(label="BACK", icon="<"),
    MenuEntry(label="CLOSE", icon="X"),
    MenuEntry(label="BATTERY", icon="%"),
]
menu2: list[MenuEntry] = [
    MenuEntry(label="B.NESS", icon="B"),
    MenuEntry(label="TIMEOUT", icon="T"),
    MenuEntry(label="BACK", icon="<"),
    MenuEntry(label="CLOSE", icon="X"),
    MenuEntry(label="STATS", icon="S"),
]
menu3: list[MenuEntry] = [
    MenuEntry(label="MENU1", icon="1"),
    MenuEntry(label="MENU2", icon="2"),
    MenuEntry(label="BACK", icon="<"),
    MenuEntry(label="CLOSE", icon="X"),
    MenuEntry(label="MENU3", icon="3"),
]

current_menu = menu1


def init():
    reset_petals()


def on_petal_pressed(index: int) -> None:
    global active_petal
    active_petal = index


def on_petal_touched(index: int, rad: int, phi: int):
    global last_petal_touched
    global touch_ticks

    if last_petal_touched != index:
        reset_last_touch()
        last_petal_touched = index
    elif touch_ticks is None:
        touch_ticks = 0
    else:
        touch_ticks += 1
        print(f"ticks: {touch_ticks}")
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
    global touch_ticks
    global current_menu

    print(f"Resetting touch ticks {touch_ticks}")

    if last_petal_touched is not None and (touch_ticks is None or touch_ticks < 5):
        selected_label = current_menu[last_petal_touched].label

        # Hacky animation
        for i in range(10):
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
    touch_ticks = None


def draw():
    global active_petal
    global last_petal_touched
    global touch_ticks

    # buf = bytes.fromhex('725601093000300006000000335d399bffe4e4e4ff7e57c2ffffffff192c1e44ff03000007000c0022010054030056002200580022005800260200520300580056005400560054005601000c03000a005600080056000800520200260300080022000a0022000c002203000108000800440300083746c9090048000c004801005403375600480058374600580044020018030058c915375600140054001401003003002a00140028000c0022000c01000c03c909000c0008c90d0008001003000007000c001e010054030056001e0058001e0058002202004e0300580052005400520054005201000c03000a0052000800520008004e020022030008001e000a001e000c001e030002070010001a010050030052001a0054001a0054001e020036030054003a0050003a0050003a01001003000e003a000c003a000c003602001e03000c001a000e001a0010001a03000307000c0020010054030056002000580020005800240200500300580054005400540054005401000c03000a005400080054000800500200240300080020000a0020000c00200300040b000c000c03c909000c0008c90d00080010020012030008c90fc909000e000c000e010022030028000e002a00160030001601005403375600160058c9170058001a020018030058c915375600140054001401003003002a00140028000c0022000c060305050f0b030303038f27012e031427012e8526252ef125722e03e1238c2ff6211d33ec20c437030a20bb3bb81f1f413d204a4303b3202d4580210046ec220046030025f6459f27a1434d297b4003802abd3e4d2df53d0030093e03ae32f03d7b35bd3eae367b40035c38a143fb3af645143d0046037b3e0046483f2d45bd3f4a430343401f41f13fbb3b0f3fc43703053e1d331a3c8c2f0a3a722e03e638d32df637d82d1f37722e030f36352fdc33023000300230031f2c0230ec29352fdc28722e05021e021e0200008f27012e060f280434010a2a02053601052c020638010a2a02073a010f28020638011426020536010f2806f1360536037b370536ec377636ec3706370501fd00fd000000f135063703f135763661360536f136053606f63406380380350638f1357738f13507390501fd00fd000000f633073903f633773866340638f634063806ec3806380376390638e6397738e63907390501fd00fd000000ec37073903ec3777385c380638ec38063806f136073a037b37073aec37783aec37083b0501fd00fd000000f135083b03f135783a6136073af136073a0600')

    # ctx.tinyvg_draw(
    #     buf
    # ).fill()

    if current_menu is None:
        sg.fill_screen(ctx, sg.BLACK)
    elif active_petal is not None:
        if touch_ticks is None:
            touch_ticks = 0
        else:
            touch_ticks += 1
        
        print(f"ticks: {touch_ticks}")

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

    icon_x = cos(-petal_angle * float(active_petal) - pi / 2.0) * 100.0
    icon_y = sin(-petal_angle * float(active_petal) - pi / 2.0) * 100.0

    icon = current_menu[active_petal].icon
    ctx.text_align = ctx.CENTER
    ctx.text_baseline = ctx.MIDDLE
    ctx.font_size = 30
    ctx.rgb(*theme.PRIMARY_TEXT)
    ctx.move_to(icon_x, icon_y)
    ctx.text(icon)


def draw_inactive_petal(index: int) -> None:
    petal_angle = 2.0 * pi / 5.0

    circle_x = cos(-petal_angle * float(index) - pi / 2.0) * 120.0
    circle_y = sin(-petal_angle * float(index) - pi / 2.0) * 120.0

    sg.draw_circle(ctx, get_menu_color(), circle_x, circle_y, 60)
    sg.draw_circle(ctx, theme.BACKGROUND, circle_x, circle_y, 50)

    icon_x = cos(-petal_angle * float(index) - pi / 2.0) * 100.0
    icon_y = sin(-petal_angle * float(index) - pi / 2.0) * 100.0

    icon = current_menu[index].icon
    ctx.text_align = ctx.CENTER
    ctx.text_baseline = ctx.MIDDLE
    ctx.font_size = 30
    ctx.rgb(*theme.BACKGROUND_TEXT)
    ctx.move_to(icon_x, icon_y)
    ctx.text(icon)


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
