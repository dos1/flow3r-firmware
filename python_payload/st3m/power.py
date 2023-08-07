import machine
import time


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
