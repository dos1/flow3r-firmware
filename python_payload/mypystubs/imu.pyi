from typing import Tuple

def acc_read() -> Tuple[float, float, float]:
    """
    Returns current x, y, z accelerations in m/s**2.
    """
    ...

def gyro_read() -> Tuple[float, float, float]:
    """
    Returns current x, y, z rotation rate in degrees/s.
    """
    ...

def pressure_read() -> Tuple[float, float]:
    """
    Returns current pressure in Pa and temperature in degree C.
    """
    ...
