import machine
import time
import sys_kernel


class Power:
    """
    Collects information about the power state (e.g. battery voltage) of the badge.

    Member of st3m.input.InputState, should be acquired from there, not instantiated manually.
    """

    def __init__(self) -> None:
        self._adc_pin = machine.Pin(9, machine.Pin.IN)
        self._adc = machine.ADC(self._adc_pin, atten=machine.ADC.ATTN_11DB)
        self._battery_voltage = self._battery_voltage_sample()
        self._ts = time.ticks_ms()

    def _battery_voltage_sample(self) -> float:
        return self._adc.read_uv() * 2 / 1e6

    def _update(self) -> None:
        ts = time.ticks_ms()
        if ts >= self._ts + 1000:
            # Sampling takes time, don't do it too often
            self._battery_voltage = self._battery_voltage_sample()
            self._ts = ts

    @property
    def battery_voltage(self) -> float:
        self._update()
        return self._battery_voltage

    @property
    def battery_charging(self) -> bool:
        """
        True if the battery is currently being charged.
        """
        return sys_kernel.battery_charging()


def approximate_battery_percentage(voltage: float) -> float:
    """
    Returns approximate battery percentage ([0,100]) based on battery voltage
    (in volts).
    """
    if voltage > 4.20:
        return 100
    piecewise = [
        (100, 4.20),
        (90, 4.06),
        (80, 3.98),
        (70, 3.92),
        (60, 3.87),
        (50, 3.82),
        (40, 3.79),
        (30, 3.77),
        (20, 3.74),
        (10, 3.68),
        (5, 3.45),
        (0, 3.00),
    ]
    for (p1, v1), (p2, v2) in zip(piecewise, piecewise[1:]):
        if voltage > v1 or voltage < v2:
            continue
        vr = v1 - v2
        pr = p1 - p2
        vd = voltage - v2
        p = vd / vr
        return pr * p + p2
    return 0
