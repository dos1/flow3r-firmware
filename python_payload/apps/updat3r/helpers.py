import os
import sys_kernel


def sd_card_plugged() -> bool:
    try:
        os.listdir("/sd")
        return True
    except OSError:
        # OSError: [Errno 19] ENODEV
        return False


def is_sd_card_stock() -> bool:
    return 500000000 < sys_kernel.total_sd_capacity() < 510000000
