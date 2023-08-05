from typing import Tuple


def acc_read() -> Tuple[float, float, float]:
    """
    Returns current x, y, z accelerations in m/s**2.
    """
    return (1.0, 2.0, 3.0)


def gyro_read() -> Tuple[float, float, float]:
    """
    Returns current x, y, z rotation rate in degrees/s.
    """
    return (4.0, 5.0, 6.0)


def pressure_read() -> Tuple[float, float]:
    """
    Returns current pressure in Pa and temperature in degree C.
    """
    return (7.0, 8.0)
