import math
import cmath
import time
import hardware
from .demo1 import app

offsets_rad = [0, 0, 0, 0, 0]
offsets_phi = [0, 0, 0, 0, 0]
last_value_rad = [0, 0, 0, 0, 0]
last_value_phi = [0, 0, 0, 0, 0]

captouch_scaling_factor = 1000
captouch_deadzone = 3

cycle = 0
calibration_cycles = 10


def run():
    global app
    global cycle
    global last_value_rad
    global last_value_phi

    # Calibrate cap touch after 10 cycles
    if cycle < calibration_cycles:
        cycle += 1

        if cycle == calibration_cycles:
            calibrate_captouch()

    # TODO: Create issue for different default values?!
    left_button_value = hardware.left_button_get()
    if left_button_value != 0:
        app.on_left_button_pressed(left_button_value)

    right_button_value = hardware.right_button_get()
    if right_button_value != 0:
        app.on_right_button_pressed(right_button_value)

    for i in range(5):
        touch_val = hardware.get_captouch(i * 2)
        if touch_val == 1:
            app.on_petal_pressed(i)

    for i in range(5):
        (rad_raw, phi_raw) = read_captouch_rad_phi(i * 2)
        rad = int(rad_raw / captouch_scaling_factor)
        phi = int(phi_raw / captouch_scaling_factor)

        if (
            abs(rad - last_value_rad[i]) > captouch_deadzone
            or abs(phi - last_value_phi[i]) > captouch_deadzone
        ):
            last_value_rad[i] = rad
            last_value_phi[i] = phi

            adjusted_rad = rad - offsets_rad[i]
            adjusted_phi = phi - offsets_phi[i]

            if (
                abs(adjusted_rad) < captouch_deadzone
                and abs(adjusted_phi) < captouch_deadzone
            ):
                app.on_petal_released(i)
            else:
                app.on_petal_touched(i, adjusted_rad, adjusted_phi)

    app.draw()

    hardware.display_update()
    hardware.update_leds()


def read_captouch_xy(i: int):
    size = (hardware.get_captouch(i) * 4) + 4
    size += int(
        max(
            0,
            sum([hardware.captouch_get_petal_pad(i, x) for x in range(0, 3 + 1)])
            / 8000,
        )
    )
    x = 70 + (hardware.captouch_get_petal_rad(i) / 1000)
    x += (hardware.captouch_get_petal_phi(i) / 600) * 1j
    rot = cmath.exp(2j * math.pi * i / 10)
    x = x * rot
    col = (1.0, 0.0, 1.0)
    if i % 2:
        col = (0.0, 1.0, 1.0)

    return (-int(x.imag - (size / 2)), -int(x.real - (size / 2)))


def read_captouch_rad_phi(i: int):
    rad = hardware.captouch_get_petal_rad(i)
    phi = hardware.captouch_get_petal_phi(i)

    return (rad, phi)


def calibrate_captouch():
    global offsets_rad
    global offsets_phi
    global last_value_rad
    global last_value_phi

    for i in range(40):
        hardware.set_led_rgb(i, 30, 0, 0)
        hardware.update_leds()

    for i in range(5):
        (rad, phi) = read_captouch_rad_phi(i * 2)
        rad_scaled = int(rad / captouch_scaling_factor)
        phi_scaled = int(phi / captouch_scaling_factor)
        offsets_rad[i] = rad_scaled
        offsets_phi[i] = phi_scaled
        last_value_rad[i] = rad_scaled
        last_value_phi[i] = phi_scaled

    print("Calibrated touch")

    for i in range(40):
        hardware.set_led_rgb(i, 0, 30, 0)
    hardware.update_leds()

    time.sleep(0.5)

    for i in range(40):
        hardware.set_led_rgb(i, 0, 0, 0)
    hardware.update_leds()


while True:
    app.init()
    run()
