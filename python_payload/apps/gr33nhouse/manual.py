from st3m.goose import Optional, Enum
from st3m.input import InputController, InputState
from st3m.ui import colours
from st3m.ui.view import BaseView, ViewManager
from ctx import Context
from .confirmation import ConfirmationView
from .background import Flow3rView

import math
import urequests
import time
import gc

PETAL_COLORS = [
    (0, 0, 1),
    (0, 1, 1),
    (1, 1, 0),
    (0, 1, 0),
    (1, 0, 1),
]
PETAL_MAP = [0, 2, 4, 6, 8]
ONE_FIFTH = math.pi * 2 / 5
ONE_TENTH = math.pi * 2 / 10


class ViewState(Enum):
    ENTER_SEED = 1
    LOADING = 2
    SEED_NOT_FOUND = 3


class ManualInputView(BaseView):
    current_petal: Optional[int]
    wait_timer: Optional[int]

    def __init__(self) -> None:
        self.input = InputController()
        self.vm = None
        self.background = Flow3rView()

        self.flow3r_seed = ""
        self.current_petal = None
        self.wait_timer = None
        self.state = ViewState.ENTER_SEED

    def on_enter(self, vm: ViewManager | None) -> None:
        super().on_enter(vm)
        self.flow3r_seed = ""
        self.state = ViewState.ENTER_SEED
        if self.vm is None:
            raise RuntimeError("vm is None")

    def draw(self, ctx: Context) -> None:
        self.background.draw(ctx)

        if self.state == ViewState.ENTER_SEED:
            ctx.save()
            for i in range(5):
                ctx.rgb(*PETAL_COLORS[i])
                ctx.move_to(0, 0)
                ctx.arc(0, 0, 140, -ONE_TENTH - math.pi / 2, ONE_TENTH - math.pi / 2, 0)
                ctx.arc(0, 0, 80, ONE_TENTH - math.pi / 2, -ONE_TENTH - math.pi / 2, 1)
                ctx.fill()

                ctx.rgb(0, 0, 0)
                ctx.arc(0, -100, 18, 0, math.pi * 2, 9).fill()

                ctx.rgb(1, 1, 1)
                ctx.move_to(0, -100)
                ctx.font = "Camp Font 1"
                ctx.font_size = 32
                ctx.text_align = ctx.CENTER
                ctx.text_baseline = ctx.MIDDLE
                ctx.text(str(i))

                ctx.rotate(ONE_FIFTH)
            # ctx.rgb(0, 0, 0)
            # ctx.arc(0, 0, 80, 0, math.pi * 2, 0).fill()
            ctx.restore()

            ctx.rgb(1, 1, 1)
            ctx.move_to(0, 0)
            ctx.font = "Camp Font 1"
            ctx.font_size = 24
            ctx.text_align = ctx.CENTER
            ctx.text_baseline = ctx.MIDDLE
            ctx.text(self.flow3r_seed)
        elif self.state == ViewState.LOADING:
            ctx.rgb(1, 1, 1)
            ctx.font = "Camp Font 3"
            ctx.font_size = 24
            ctx.text_align = ctx.CENTER
            ctx.text_baseline = ctx.MIDDLE
            ctx.move_to(0, -12)
            ctx.text(f"Loading")
            ctx.move_to(0, 12)
            ctx.font = "Camp Font 1"
            ctx.text(self.flow3r_seed)
        elif self.state == ViewState.SEED_NOT_FOUND:
            ctx.rgb(1, 0, 0)
            ctx.font_size = 24
            ctx.text_align = ctx.CENTER
            ctx.text_baseline = ctx.MIDDLE
            ctx.move_to(0, -12)
            ctx.font = "Camp Font 1"
            ctx.text(self.flow3r_seed)
            ctx.move_to(0, 12)
            ctx.font = "Camp Font 3"
            ctx.text(f"not found!")

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.input.think(ins, delta_ms)
        self.background.think(ins, delta_ms)

        if self.state == ViewState.ENTER_SEED:
            if self.current_petal is not None:
                if not ins.captouch.petals[self.current_petal].pressed:
                    self.current_petal = None

            if self.current_petal is None:
                for i, petal in enumerate(PETAL_MAP):
                    if ins.captouch.petals[petal].pressed:
                        self.flow3r_seed += str(i)
                        self.current_petal = petal

            if len(self.flow3r_seed) == 8:
                self.state = ViewState.LOADING
        elif self.state == ViewState.LOADING:
            if self.wait_timer is None:
                self.wait_timer = time.ticks_ms()
            if (time.ticks_ms() - self.wait_timer) < 100:
                return
            self.wait_timer = None

            print(f"Loading app info for seed {self.flow3r_seed}...")
            res = urequests.get(
                f"https://flow3r.garden/api/apps/{self.flow3r_seed}.json"
            )

            if res.status_code != 200:
                # We are hitting RAM limits in this place.  Free up
                # everything we can to keep the following code from
                # hitting allocation errors.
                del res
                gc.collect()
                print(f"No app found for seed {self.flow3r_seed}!")
                self.state = ViewState.SEED_NOT_FOUND
            else:
                if self.vm is None:
                    raise RuntimeError("vm is None")

                app = res.json()
                self.vm.push(
                    ConfirmationView(
                        url=app["tarDownloadUrl"],
                        name=app["name"],
                        author=app["author"],
                    )
                )
        elif self.state == ViewState.SEED_NOT_FOUND:
            if self.wait_timer is None:
                self.wait_timer = time.ticks_ms()
            if (time.ticks_ms() - self.wait_timer) > 2000:
                print("Please enter a new seed!")
                self.flow3r_seed = ""
                self.state = ViewState.ENTER_SEED
                self.wait_timer = None
