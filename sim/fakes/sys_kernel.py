class FakeHeapKindStats:
    def __init__(self, kind):
        self.kind = kind
        self.total_free_bytes = 1337
        self.total_allocated_bytes = 1337
        self.largest_free_block = 1337


class FakeHeapStats:
    general = FakeHeapKindStats("general")
    dma = FakeHeapKindStats("dma")


def heap_stats():
    return FakeHeapStats()


def usb_connected():
    return True


def usb_console_active():
    return True


def freertos_sleep(ms):
    import _time

    _time.sleep(ms / 1000.0)


def i2c_scan():
    return [16, 44, 45, 85, 109, 110]
