import random
from st3m.input import InputController, InputState

from st3m.ui.view import BaseView
from ctx import Context


class Flow3rView(BaseView):
    input: InputController

    def __init__(self) -> None:
        self.vm = None
        self.input = InputController()

        self.flowers = []
        for i in range(8):
            self.flowers.append(
                Flower(
                    ((random.getrandbits(16) - 32767) / 32767.0) * 200,
                    ((random.getrandbits(16)) / 65535.0) * 240 - 120,
                    ((random.getrandbits(16)) / 65535.0) * 400 + 25,
                )
            )

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        for c in self.flowers:
            c.y += (10 * delta_ms / 1000.0) * 200 / c.z
            if c.y > 300:
                c.y = -300
            c.rot += delta_ms * c.rot_speed
        self.flowers = sorted(self.flowers, key=lambda c: -c.z)

    def draw(self, ctx: Context) -> None:
        ctx.save()
        ctx.rectangle(-120, -120, 240, 240)
        ctx.rgb(0.1, 0.4, 0.3)
        ctx.fill()

        for f in self.flowers:
            f.draw(ctx)
        ctx.restore()


class Flower:
    def __init__(self, x: float, y: float, z: float) -> None:
        self.x = x
        self.y = y
        self.z = z
        self.rot = 0
        self.rot_speed = (((random.getrandbits(16) - 32767) / 32767.0) - 0.5) / 800

    def draw(self, ctx: Context):
        ctx.save()
        ctx.translate(-78 + self.x, -70 + self.y)
        ctx.translate(50, 40)

        ctx.rotate(self.rot)
        ctx.translate(-50, -40)
        ctx.scale(100 / self.z, 100.0 / self.z)
        ctx.move_to(76.221727, 3.9788409).curve_to(
            94.027758, 31.627675, 91.038918, 37.561293, 94.653428, 48.340473
        ).rel_curve_to(
            25.783102, -3.90214, 30.783332, -1.52811, 47.230192, 4.252451
        ).rel_curve_to(
            -11.30184, 19.609496, -21.35729, 20.701768, -35.31018, 32.087063
        ).rel_curve_to(
            5.56219, 12.080061, 12.91196, 25.953973, 9.98735, 45.917643
        ).rel_curve_to(
            -19.768963, -4.59388, -22.879866, -10.12216, -40.896842, -23.93099
        ).rel_curve_to(
            -11.463256, 10.23025, -17.377386, 18.2378, -41.515124, 25.03533
        ).rel_curve_to(
            0.05756, -29.49286, 4.71903, -31.931936, 10.342734, -46.700913
        ).curve_to(
            33.174997, 77.048676, 19.482194, 71.413009, 8.8631648, 52.420793
        ).curve_to(
            27.471602, 45.126773, 38.877997, 45.9184, 56.349456, 48.518302
        ).curve_to(
            59.03275, 31.351935, 64.893201, 16.103886, 76.221727, 3.9788409
        ).close_path().rgba(
            1.0, 0.6, 0.4, 0.4
        ).fill()
        ctx.restore()
        return
        ctx.move_to(116.89842, 17.221179).rel_curve_to(
            6.77406, 15.003357, 9.99904, 35.088466, 0.27033, 47.639569
        ).curve_to(
            108.38621, 76.191194, 87.783414, 86.487988, 75.460015, 75.348373
        ).curve_to(
            64.051094, 64.686361, 61.318767, 54.582827, 67.499384, 36.894251
        ).curve_to(
            79.03955, 16.606134, 103.60918, 15.612261, 116.89842, 17.221179
        ).close_path().rgb(
            0.5, 0.3, 0.4
        ).fill()

        ctx.move_to(75.608612, 4.2453713).curve_to(
            85.516707, 17.987709, 93.630911, 33.119248, 94.486497, 49.201225
        ).curve_to(
            95.068862, 60.147617, 85.880014, 75.820834, 74.919761, 75.632395
        ).curve_to(
            63.886159, 75.442695, 57.545631, 61.257211, 57.434286, 50.22254
        ).curve_to(
            57.257291, 32.681814, 65.992688, 16.610811, 75.608612, 4.2453713
        ).close_path().rgb(
            0.2, 0.5, 0.8
        ).fill()
        ctx.restore()
