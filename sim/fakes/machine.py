class Pin:
    IN = None

    def __init__(self, _1, _2):
        pass


class ADC:
    ATTN_11DB = None

    def __init__(self, _1, atten):
        pass

    def read_uv(self):
        # A half full battery as seen by the ADC
        return 3.8e6 / 2
