import os


def sd_card_plugged() -> bool:
    try:
        os.listdir("/sd")
        return True
    except OSError:
        # OSError: [Errno 19] ENODEV
        return False


def is_sd_card_stock() -> bool:
    # TODO: check if ~512mb
    return True
