import machine
import time
import sys_kernel
from st3m import logging

log = logging.Log(__name__, level=logging.DEBUG)


class Power:
    """
    Collects information about the power state (e.g. battery voltage) of the badge.

    Member of st3m.input.InputState, should be acquired from there, not instantiated manually.
    """

    def __init__(self) -> None:
        self._adc_pin = machine.Pin(9, machine.Pin.IN, pull=None)
        # with no battery will be pulled down from the voltage divider
        # current too low with 1M for ADC so need to check via GPIO
        bat_read = 0
        for i in range(5):
            time.sleep(0.002)
            bat_read += self._adc_pin.value()
        if(bat_read > 4):
            self._has_battery = True
        else:
            self._has_battery = False
        
        self._adc = machine.ADC(self._adc_pin, atten=machine.ADC.ATTN_11DB)
        self._battery_voltage = self._battery_voltage_sample()
        self._prev_battery_percentages = [1,1,1]
        self._battery_percentage = 1
        # speeding up the process to get an intial settled value because recursion is hard
        for i in range(5):
            self._approximate_battery_percentage()
        self._ts = time.ticks_ms()

    def _battery_voltage_sample(self) -> float:
        return self._adc.read_uv() * 2 / 1e6

    def _update(self) -> None:
        ts = time.ticks_ms()
        if time.ticks_diff(ts, self._ts) > 2000:
            # Sampling takes time, don't do it too often
            log.debug("has battery: " + str(self._has_battery))
            log.debug("is charging: " + str(self.battery_charging))
            self._battery_voltage = self._battery_voltage_sample()
            self._battery_percentage = self._approximate_battery_percentage()
            self._ts = ts

    @property
    def battery_voltage(self) -> float:
        self._update()
        return self._battery_voltage

    @property
    def has_battery(self) -> bool:
        return self._has_battery

    @property
    def battery_charging(self) -> bool:
        """
        True if the battery is currently being charged.
        """
        return sys_kernel.battery_charging()

    @property
    def battery_percentage(self) -> int:
        self._update()
        return self._battery_percentage
    
    def _approximate_battery_percentage(self) -> int:
        """
        Returns approximate battery percentage ([0,100]) based on battery voltage.
        """
        if not self._has_battery:
            return 100
        
        percentage = 0
        num_samples = 10
        voltage_readings = []
        for i in range(num_samples):
            voltage_readings.append(self._battery_voltage_sample())

        voltage_readings.sort()
        voltage = voltage_readings[int(num_samples / 2)]
        
        # print(voltage)

        if voltage > 4.120:
            percentage = 100
        # LUT created from Joulescope measurement of "official" 2Ah Battery at 650mW discharge at 26°C and decimated from ~42k samples
        batLUT = [
            (99, 4.114),
            (98, 4.109),
            (97, 4.091),
            (96, 4.076),
            (95, 4.061),
            (94, 4.048),
            (93, 4.036),
            (92, 4.024),
            (91, 4.012),
            (90, 4.001),
            (89, 3.989),
            (88, 3.978),
            (87, 3.967),
            (86, 3.956),
            (85, 3.945),
            (84, 3.934),
            (83, 3.923),
            (82, 3.912),
            (81, 3.901),
            (80, 3.890),
            (79, 3.879),
            (78, 3.869),
            (77, 3.858),
            (76, 3.847),
            (75, 3.837),
            (74, 3.827),
            (73, 3.817),
            (72, 3.807),
            (71, 3.797),
            (70, 3.788),
            (69, 3.778),
            (67, 3.769),
            (66, 3.759),
            (65, 3.750),
            (64, 3.741),
            (63, 3.732),
            (62, 3.723),
            (61, 3.715),
            (60, 3.706),
            (59, 3.698),
            (58, 3.690),
            (57, 3.682),
            (56, 3.674),
            (55, 3.666),
            (54, 3.659),
            (53, 3.652),
            (52, 3.645),
            (51, 3.639),
            (50, 3.633),
            (49, 3.627),
            (48, 3.622),
            (47, 3.617),
            (46, 3.609),
            (45, 3.605),
            (44, 3.602),
            (43, 3.598),
            (42, 3.595),
            (41, 3.591),
            (40, 3.588),
            (39, 3.584),
            (38, 3.581),
            (37, 3.576),
            (36, 3.573),
            (35, 3.569),
            (34, 3.566),
            (33, 3.563),
            (32, 3.562),
            (31, 3.556),
            (30, 3.552),
            (29, 3.549),
            (28, 3.545),
            (27, 3.541),
            (26, 3.537),
            (25, 3.533),
            (24, 3.529),
            (23, 3.525),
            (22, 3.520),
            (21, 3.515),
            (20, 3.510),
            (20, 3.505),
            (19, 3.499),
            (18, 3.493),
            (17, 3.486),
            (16, 3.479),
            (15, 3.472),
            (14, 3.464),
            (13, 3.455),
            (12, 3.446),
            (11, 3.437),
            (10, 3.426),
            (9, 3.415),
            (8, 3.404),
            (7, 3.391),
            (6, 3.378),
            (5, 3.365),
            (4, 3.348),
            (3, 3.324),
            (2, 3.286),
            (1, 3.223),
            (1, 3.121),
            (0, 2.958),
            (0, 0),
        ]


        for i in range(len(batLUT)):
            if voltage >= batLUT[i][1]:
                percentage = batLUT[i][0]
                break

        self._prev_battery_percentages.pop(0)
        self._prev_battery_percentages.append(percentage)
        log.debug("percentage: " + str(percentage) + " %")
        log.debug("prev: " + str(self._prev_battery_percentages) + " %")
        percent_list = self._prev_battery_percentages

        # avoid division by zero in weird edge cases
        if (sum(percent_list) == 0) or (percent_list[0] == 0):
            return 0
        
        for i in range(len(percent_list)):
            if sum(percent_list) / percent_list[0] == len(percent_list):
                # all values are the same, we settled on a value (might be the same as before but that's ok)
                return percentage
            else:
                #we're still settling on a value, return previously settled value
                return self._battery_percentage
                    
        




