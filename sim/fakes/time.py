
def sleep_ms(ms):
    import _time
    _time.sleep(ms * 0.001)

def ticks_ms():
    import _time
    return int(_time.time() * 1000)
