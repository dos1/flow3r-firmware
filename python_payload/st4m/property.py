class Property:
    pass


class PropertyFloat(Property):
    def __init__(self):
        self.min = 0.0
        self.max = 1.0

    def set_value(self, value):
        self._value = value

    def mod_value(self, delta):
        self._value += delta_ms

    def get_value(self):
        return self._value


# Define a few RGB (0.0 to 1.0) colors
BLACK = (0, 0, 0)
RED = (1, 0, 0)
GREEN = (0, 1, 0)
BLUE = (0, 0, 1)
WHITE = (1, 1, 1)
GREY = (0.5, 0.5, 0.5)
GO_GREEN = (63 / 255, 255 / 255, 33 / 255)
PUSH_RED = (251 / 255, 72 / 255, 196 / 255)


class Color:
    @classmethod
    def from_rgb888(cls, r: int = 0, g: int = 0, b: int = 0):
        return cls(r / 255, g / 255, b / 255)

    def __init__(self, r: float = 0.0, g: float = 0.0, b: float = 0.0) -> None:
        self.r = r
        self.g = g
        self.b = b

    def as_normal_tuple(self) -> tuple[float]:
        return (self.r, self.g, self.b)


print(Color.from_rgb888(255, 255, 128).as_normal_tuple())
