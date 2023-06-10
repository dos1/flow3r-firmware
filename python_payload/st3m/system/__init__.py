import hardware as _hardware


class NamedObject:
    def __init__(self, name="foo"):
        self.__name = name

    def __repr__(self):
        return self.__name


class MockObject(NamedObject):
    def __getattr__(self, attr):
        attr_name = "{}.{}".format(str(self), attr)
        print("mock attr", attr_name)
        return MockObject(attr_name)

    def __call__(self, *args, **kwargs):
        call_name = "{}({}{})".format(str(self), args, kwargs)
        print("mock call", call_name)
        return MockObject(call_name)


try:
    import audio as _audio
except ModuleNotFoundError:
    print("no real audio, using mock module")
    _audio = MockObject("audio")


try:
    import audio as _audio
except ModuleNotFoundError:
    print("no real audio, using mock module")
    _audio = MockObject("audio")


hardware = _hardware
audio = _audio

ctx = hardware.get_ctx()
