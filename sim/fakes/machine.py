import sys


class Pin:
    IN = None

    def __init__(self, _1, _2, pull=None):
        pass

    def value(self):
        return 0


class ADC:
    ATTN_11DB = None

    def __init__(self, _1, atten):
        pass

    def read_uv(self):
        # A half full battery as seen by the ADC
        return 3.8e6 / 2


class I2C:
    def __init__(self, chan, freq=None):
        pass

    def scan(self):
        return []


def reset():
    print("beep boop i have reset")
    sys.exit(0)


def disk_mode_flash():
    print("beep boop i'm now in flash disk mode")
    sys.exit(0)


def disk_mode_sd():
    print("beep boop i'm now in sd card disk mode")
    sys.exit(0)
