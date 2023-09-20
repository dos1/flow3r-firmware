import bl00mbox
import leds
import captouch

from st3m.application import Application, ApplicationContext
from st3m.input import InputState
from st3m.goose import Tuple, Iterator, Optional, Callable, List, Any, TYPE_CHECKING
from ctx import Context
from st3m.ui.view import View, ViewManager

import json
import errno

if TYPE_CHECKING:
    Number = float | int
    Color = Tuple[Number, Number, Number]
    ColorLeds = Tuple[int, int, int]
    Rendee = Callable[[Context, Any], None]
else:
    Number = Color = ColorLeds = Rendee = None


class GayDrums(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        try:
            self.bundle_path = app_ctx.bundle_path
        except Exception:
            self.bundle_path = "/flash/sys/apps/harmonic_demo"
        self.blm = None
        self.num_samplers = 6
        self.seq = None

        self.kick: Optional[bl00mbox.patches._Patch] = None
        self.hat: Optional[bl00mbox.patches._Patch] = None
        self.close: Optional[bl00mbox.patches._Patch] = None
        self.open: Optional[bl00mbox.patches._Patch] = None
        self.crash: Optional[bl00mbox.patches._Patch] = None
        self.snare: Optional[bl00mbox.patches._Patch] = None

        self.track_names = ["kick", "hihat", "snare", "crash", "nya", "woof"]
        self.ct_prev: Optional[captouch.CaptouchState] = None
        self.track = 1
        self.tap_tempo_press_counter = 0
        self.track_back_press_counter = 0
        self.stopped = True
        self.tapping = False
        self.tap = {
            "sum_y": float(0),
            "sum_x": float(1),
            "sum_xy": float(0),
            "sum_xx": float(1),
            "last": float(0),
            "time_ms": 0,
            "count": 0,
        }
        self.track_back = False

        self.bpm = 0
        self.samples_loaded = 0
        self.samples_total = len(self.track_names)
        self.loading_text = ""
        self.init_complete = False
        self.load_iter = self.iterate_loading()
        self._render_list: List[Tuple[Rendee, Any]] = []

        self._group_highlight_on = [False] * 4
        self._group_highlight_redraw = [False] * 4
        self._group_gap = 5
        self.background_col: Color = (0, 0, 0)
        self.bar_col = (0.5, 0.5, 0.5)
        self.highlight_col: Color = self.bar_col
        self.highlight_outline: bool = True

        self.bar = 0
        self._tracks_empty: bool = True

        self._bpm_saved = None
        self._steps_saved = None
        self._seq_table_saved = None
        self._file_settings = None

        self._load_settings()

    def _try_load_settings(self, path):
        try:
            with open(path, "r") as f:
                return json.load(f)
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise  # ignore file not found

    def _try_write_default_settings(self, path: str, default_path: str) -> None:
        with open(path, "w") as outfile, open(default_path, "r") as infile:
            outfile.write(infile.read())

    def _try_save_settings(self, path, settings):
        try:
            with open(path, "w+") as f:
                f.write(json.dumps(settings))
                f.close()
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise  # ignore file not found

    def _save_settings(self):
        default_path = self.bundle_path + "/gay_drums-default.json"
        settings_path = "/flash/gay_drums.json"
        settings = self._try_load_settings(default_path)
        assert settings is not None, "failed to load default settings"

        file_difference = False
        for i, beat in enumerate(settings["beats"]):
            beat["bpm"] = self.bpm
            beat["steps"] = self.steps
            beat["sequencer_table"] = self.seq.plugins.seq.table

            if self._file_settings is None:
                file_difference = True
            else:
                file_beat = self._file_settings["beats"][i]
                for i in beat:
                    if beat.get(i) != file_beat.get(i):
                        file_difference = True
                        break
        if file_difference:
            self._try_save_settings(settings_path, settings)
            self._file_settings = settings

    def _load_settings(self):
        default_path = self.bundle_path + "/gay_drums-default.json"
        settings_path = "/flash/gay_drums.json"

        settings = self._try_load_settings(default_path)
        assert settings is not None, "failed to load default settings"

        user_settings = self._try_load_settings(settings_path)

        if user_settings is None:
            self._try_write_default_settings(settings_path, default_path)
            self._file_settings = settings
            self.stopped = False
        else:
            settings.update(user_settings)
            self._file_settings = user_settings
            for i, beat in enumerate(settings["beats"]):
                if self.blm is None:
                    self._bpm_saved = beat["bpm"]
                    self._steps_saved = beat["steps"]
                    self._seq_table_saved = beat["sequencer_table"]
                else:
                    self.bpm = beat["bpm"]
                    if not self.stopped:
                        self.seq.signals.bpm.value = self.bpm
                    self.steps = beat["steps"]
                    self.seq.plugins.seq.table = beat["sequencer_table"]

    @property
    def steps(self):
        if self.blm is not None:
            return self.seq.signals.step_end.value + 1
        return 0

    @steps.setter
    def steps(self, val):
        if self.blm is not None:
            self.seq.signals.step_start = 0
            self.seq.signals.step_end = val - 1

    def iterate_loading(self) -> Iterator[Tuple[int, str]]:
        if self.blm is None:
            self.blm = bl00mbox.Channel("gay drums")
        self.seq = self.blm.new(bl00mbox.patches.sequencer, num_tracks=8, num_steps=32)

        if self._bpm_saved is None:
            bpm = 80
        else:
            bpm = self._bpm_saved
        if self._steps_saved is None:
            steps = 16
        else:
            steps = self._steps_saved

        self.steps = steps
        self.seq.signals.bpm = bpm
        self.bpm = self.seq.signals.bpm.value

        if self._seq_table_saved is not None:
            self.seq.plugins.seq.table = self._seq_table_saved
        if self.stopped:
            self.seq.signals.bpm = 0
            self.seq.signals.sync_in.start()
            self._render_list += [(self.draw_bpm, None)]
            self.blm.foreground = False

        yield 0, "kick.wav"
        self.nya = self.blm.new(bl00mbox.patches.sampler, "nya.wav")
        self.nya.signals.output = self.blm.mixer
        self.nya.signals.trigger = self.seq.plugins.seq.signals.track6
        self.kick = self.blm.new(bl00mbox.patches.sampler, "kick.wav")
        self.kick.signals.output = self.blm.mixer
        self.kick.signals.trigger = self.seq.plugins.seq.signals.track0
        yield 1, "hihat.wav"
        self.woof = self.blm.new(bl00mbox.patches.sampler, "bark.wav")
        self.woof.signals.output = self.blm.mixer
        self.woof.signals.trigger = self.seq.plugins.seq.signals.track7
        self.hat = self.blm.new(bl00mbox.patches.sampler, "hihat.wav")
        self.hat.signals.output = self.blm.mixer
        self.hat.signals.trigger = self.seq.plugins.seq.signals.track1
        yield 2, "close.wav"
        self.close = self.blm.new(bl00mbox.patches.sampler, "close.wav")
        self.close.signals.output = self.blm.mixer
        self.close.signals.trigger = self.seq.plugins.seq.signals.track2
        yield 3, "open.wav"
        self.open = self.blm.new(bl00mbox.patches.sampler, "open.wav")
        self.open.signals.output = self.blm.mixer
        self.open.signals.trigger = self.seq.plugins.seq.signals.track3
        yield 4, "snare.wav"
        self.snare = self.blm.new(bl00mbox.patches.sampler, "snare.wav")
        self.snare.signals.output = self.blm.mixer
        self.snare.signals.trigger = self.seq.plugins.seq.signals.track4
        yield 5, "crash.wav"
        self.crash = self.blm.new(bl00mbox.patches.sampler, "crash.wav")
        self.crash.signals.output = self.blm.mixer
        self.crash.signals.trigger = self.seq.plugins.seq.signals.track5
        yield 6, ""

    def _highlight_petal(self, num: int, r: int, g: int, b: int) -> None:
        for i in range(5):
            leds.set_rgb((4 * num - i + 2) % 40, r, g, b)

    def _track_col(self, track: int) -> ColorLeds:
        rgb = (20, 20, 20)
        if track == 0:
            rgb = (120, 0, 137)
        elif track == 1:
            rgb = (0, 75, 255)
        elif track == 2:
            rgb = (0, 130, 27)
        elif track == 3:
            rgb = (255, 239, 0)
        elif track == 4:
            rgb = (255, 142, 0)
        elif track == 5:
            rgb = (230, 0, 0)
        return rgb

    def draw_background(self, ctx: Context, data: None) -> None:
        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        self.draw_bars(ctx, None)

        ctx.font = ctx.get_font_name(1)

        ctx.font_size = 15
        ctx.rgb(0.6, 0.6, 0.6)

        ctx.move_to(0, -80)
        ctx.text("(hold) stop")

        ctx.move_to(0, -95)
        ctx.text("tap tempo")

        self.draw_track_name(ctx, None)
        self.draw_bpm(ctx, None)

        for track in range(self.num_samplers):
            for step in range(16):
                self.draw_track_step_marker(ctx, (track, step))

    def tracks_are_empty(self):
        for track in range(6):
            for step in range(16):
                if self.track_get_state(track, step):
                    return False
        return True

    def track_get_state(self, track: int, step: int) -> int:
        sequencer_track = track
        if track > 1:
            sequencer_track += 2
        if track == 1:
            if self.seq.trigger_state(1, step) > 0:
                return 1
            elif self.seq.trigger_state(2, step) > 0:
                return 2
            elif self.seq.trigger_state(3, step) > 0:
                return 3
            else:
                return 0
        else:
            state = self.seq.trigger_state(sequencer_track, step)
            if state == 0:
                return 0
            elif state == 32767:
                return 3
            elif state < 16384:
                return 1
            else:
                return 2

    def track_incr_state(self, track: int, step: int) -> None:
        sequencer_track = track
        step = step % 16
        if track > 1:
            sequencer_track += 2
        if track == 1:
            state = self.track_get_state(track, step)
            if state == 0:
                while step < 32:
                    self.seq.trigger_start(1, step)
                    self.seq.trigger_stop(2, step)
                    self.seq.trigger_stop(3, step)
                    step += 16
            if state == 1:
                while step < 32:
                    self.seq.trigger_stop(1, step)
                    self.seq.trigger_start(2, step)
                    self.seq.trigger_stop(3, step)
                    step += 16
            if state == 2:
                while step < 32:
                    self.seq.trigger_stop(1, step)
                    self.seq.trigger_stop(2, step)
                    self.seq.trigger_start(3, step)
                    step += 16
            if state == 3:
                while step < 32:
                    self.seq.trigger_clear(1, step)
                    self.seq.trigger_clear(2, step)
                    self.seq.trigger_clear(3, step)
                    step += 16
        else:
            state = self.seq.trigger_state(sequencer_track, step)
            if state == 0:
                new_state = 16000
            elif state == 32767:
                new_state = 0
            else:
                new_state = 32767
            self.seq.trigger_start(sequencer_track, step, new_state)
            self.seq.trigger_start(sequencer_track, step + 16, new_state)

    def draw_track_step_marker(self, ctx: Context, data: Tuple[int, int]) -> None:
        track, step = data
        rgb = self._track_col(track)
        rgbf = (rgb[0] / 256, rgb[1] / 256, rgb[2] / 256)
        y = -int(12 * (track - (self.num_samplers - 1) / 2))
        empty = self._tracks_empty
        # retrieving the state is super slow, so don't do it when transitioning
        if self.vm.transitioning:
            empty = True
        trigger_state = self.track_get_state(track, step) if not empty else 0
        size = 2
        x = 12 * (7.5 - step)
        x += self._group_gap * (1.5 - (step // 4))
        x = int(-x)
        group = step // 4
        bg = self.background_col
        if self._group_highlight_on[group] and not self.highlight_outline:
            bg = self.highlight_col
        if trigger_state == 3:
            self.ctx_draw_centered_rect(ctx, x, y, 8, 8, rgbf)
        elif trigger_state == 2:
            self.ctx_draw_centered_rect(ctx, x, y, 8, 8, bg)
            self.ctx_draw_centered_rect(ctx, x, y - 2, 8, 4, rgbf)
        elif trigger_state == 1:
            self.ctx_draw_centered_rect(ctx, x, y, 8, 8, bg)
            self.ctx_draw_centered_rect(ctx, x, y + 2, 8, 4, rgbf)
        elif trigger_state == 0:
            self.ctx_draw_centered_rect(ctx, x, y, 8, 8, bg)
            self.ctx_draw_centered_rect(ctx, x, y, 2, 2, rgbf)

    def ctx_draw_centered_rect(
        self, ctx: Context, posx: int, posy: int, sizex: int, sizey: int, col: Color
    ) -> None:
        ctx.rgb(*col)
        nosx = int(posx - (sizex / 2))
        nosy = int(posy - (sizey / 2))
        ctx.rectangle(nosx, nosy, int(sizex), int(sizey)).fill()

    def draw_bars(self, ctx: Context, data: None) -> None:
        ctx.move_to(0, 0)
        bar_len = 10 * (1 + self.num_samplers)
        bar_pos = 4 * 12 + self._group_gap
        self.ctx_draw_centered_rect(ctx, 0, 0, 1, bar_len, self.bar_col)
        self.ctx_draw_centered_rect(ctx, bar_pos, 0, 1, bar_len, self.bar_col)
        self.ctx_draw_centered_rect(ctx, -bar_pos, 0, 1, bar_len, self.bar_col)

    def draw_group_highlight(self, ctx: Context, data: int) -> None:
        i = data
        col = self.background_col
        if self._group_highlight_on[i]:
            col = self.highlight_col

        bar_len = 10 * (1 + self.num_samplers)
        bar_pos = 4 * 12 + self._group_gap

        sizex = 4 * 12
        sizey = bar_len
        posx = int(bar_pos * (i - 1.5))
        posy = 0

        if self.highlight_outline:
            self.ctx_draw_centered_rect(
                ctx, posx, int(sizey / 2) + 2, sizex - 2, 1, col
            )
            self.ctx_draw_centered_rect(
                ctx, posx, int(-sizey / 2) - 2, sizex - 2, 1, col
            )
            if i == 0:
                self.ctx_draw_centered_rect(ctx, -2 * bar_pos, 0, 1, bar_len, col)
            if i == 3:
                self.ctx_draw_centered_rect(ctx, 2 * bar_pos, 0, 1, bar_len, col)
        else:
            self.ctx_draw_centered_rect(ctx, posx, posy, sizex, sizey, col)
            for x in range(self.num_samplers):
                for y in range(4):
                    self.draw_track_step_marker(ctx, (x, y + 4 * i))

    def draw_bpm(self, ctx: Context, data: None) -> None:
        ctx.text_align = ctx.MIDDLE
        ctx.move_to(0, 0)
        self.ctx_draw_centered_rect(ctx, 0, -60, 200, 26, (0, 0, 0))
        bpm = self.seq.signals.bpm.value
        ctx.font = ctx.get_font_name(1)
        ctx.font_size = 20

        ctx.move_to(0, -60)
        ctx.rgb(255, 255, 255)
        ctx.text(str(bpm) + " bpm")

    def draw(self, ctx: Context) -> None:
        ctx.text_align = ctx.MIDDLE
        if not self.init_complete:
            ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()

            ctx.font = ctx.get_font_name(0)
            ctx.font_size = 24
            ctx.move_to(0, -40)
            ctx.rgb(0.8, 0.8, 0.8)
            if self.samples_loaded == self.samples_total:
                ctx.text("Loading complete")
                self.loading_text = ""
            else:
                ctx.text("Loading samples...")

            ctx.font_size = 16
            ctx.move_to(0, -20)
            ctx.text(self.loading_text)

            tutorial = (
                "how to:\n"
                "press one of the 4 left petals\n"
                "and one of the 4 right petals\n"
                "at the same time to toggle\n"
                "an event in the grid"
            )
            ctx.font_size = 16
            for x, line in enumerate(tutorial.split("\n")):
                if not x:
                    continue
                ctx.move_to(0, 5 + 19 * x)
                ctx.text(line)

            progress = self.samples_loaded / self.samples_total
            bar_len = 120 / self.samples_total
            for x in range(self.samples_loaded):
                rgb = self._track_col(x)
                rgbf = (rgb[0] / 256, rgb[1] / 256, rgb[2] / 256)
                ctx.rgb(*rgbf)
                ctx.rectangle(x * bar_len - 60, -8, bar_len, 10).fill()
            return

        if self.vm.transitioning:
            self._render_list += [(self.draw_background, None)]

        for i in range(4):
            if self._group_highlight_redraw[i]:
                self._group_highlight_redraw[i] = False
                self._render_list += [(self.draw_group_highlight, i)]

        for rendee in self._render_list:
            fun, param = rendee
            fun(ctx, param)
        self._render_list = []

        size = 4

        st = self.seq.signals.step.value

        stepx = -12 * (7.5 - (st % 16))
        stepx -= self._group_gap * (1.5 - ((st % 16) // 4))
        stepy = -13 - 5 * self.num_samplers

        trigger_state = self.track_get_state(self.track, st)
        dotsize = 1
        if trigger_state:
            dotsize = 4
        self.ctx_draw_centered_rect(ctx, 0, stepy, 240, 8, self.background_col)
        self.ctx_draw_centered_rect(ctx, stepx, stepy, dotsize, dotsize, (1, 1, 1))

        st_old = st
        st = self.steps - 1
        st_mod = st % 16

        repeater_col = (0.5, 0.5, 0.5)
        if st != 15:
            stepx = -12 * (7.5 - st_mod)
            stepx -= self._group_gap * (1.5 - (st_mod // 4))
            if st_mod == 3 or st_mod == 11:
                stepx += self._group_gap / 2
            elif st_mod == 7:
                stepx += (self._group_gap + 1) / 2

            if st == st_mod:
                stepx += 6
                self.ctx_draw_centered_rect(ctx, stepx, stepy, 1, 6, repeater_col)
            else:
                stepx += 5
                if st_old < 16:
                    self.ctx_draw_centered_rect(
                        ctx, stepx, stepy + 2.5, 1, 3, repeater_col
                    )
                    self.ctx_draw_centered_rect(
                        ctx, stepx, stepy - 2.5, 1, 3, repeater_col
                    )
                    stepx += 2
                    self.ctx_draw_centered_rect(
                        ctx, stepx, stepy + 2.5, 1, 3, repeater_col
                    )
                    self.ctx_draw_centered_rect(
                        ctx, stepx, stepy - 2.5, 1, 3, repeater_col
                    )
                else:
                    self.ctx_draw_centered_rect(ctx, stepx, stepy, 1, 6, repeater_col)
                    stepx += 2
                    self.ctx_draw_centered_rect(ctx, stepx, stepy, 1, 6, repeater_col)

    def draw_steps(self, ctx, data):
        self.ctx_draw_centered_rect(ctx, 0, 60, 200, 40, self.background_col)
        ctx.font = ctx.get_font_name(1)
        ctx.text_align = ctx.MIDDLE

        ctx.font_size = 20
        ctx.rgb(1, 1, 1)

        ctx.move_to(120, -120)
        ctx.text(str(data))

    def draw_track_name(self, ctx: Context, data: None) -> None:
        self.ctx_draw_centered_rect(ctx, 0, 60, 200, 40, self.background_col)
        ctx.font = ctx.get_font_name(1)
        ctx.text_align = ctx.MIDDLE

        ctx.font_size = 20
        ctx.rgb(1, 1, 1)

        ctx.move_to(45, 60)
        ctx.text(">")

        ctx.move_to(-45, 60)
        ctx.text(">")

        ctx.font_size = 15
        ctx.rgb(0.6, 0.6, 0.6)
        ctx.move_to(-45, 75)
        ctx.text("(hold)")

        ctx.font_size = 28

        track = self.track
        ctx.move_to(0, 60)
        col = [x / 255 for x in self._track_col(track)]
        ctx.rgb(*col)
        ctx.text(self.track_names[track])

        ctx.font_size = 15

        track = (track - 1) % self.num_samplers
        ctx.move_to(75, 60)
        col = [x / 255 for x in self._track_col(track)]
        ctx.rgb(*col)
        ctx.text(self.track_names[track])

        track = (track + 2) % self.num_samplers
        ctx.move_to(-75, 60)
        col = [x / 255 for x in self._track_col(track)]
        ctx.rgb(*col)
        ctx.text(self.track_names[track])

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        if not self.init_complete:
            if self.vm.transitioning:
                return
            try:
                self.samples_loaded, self.loading_text = next(self.load_iter)
            except StopIteration:
                self.init_complete = True
                self._render_list += [(self.draw_background, None)]
            return

        if self.input.buttons.app.left.pressed:
            if self.steps > 3:
                self.steps -= 1

        if self.input.buttons.app.right.pressed:
            if self.steps < 31:
                self.steps += 1

        if self.ct_prev is None:
            self.ct_prev = ins.captouch
            return

        st = self.seq.signals.step.value
        rgb = self._track_col(self.track)
        leds.set_all_rgb(0, 0, 0)
        self._highlight_petal(10 - (4 - (st // 4)), *rgb)
        self._highlight_petal(10 - (6 + (st % 4)), *rgb)
        leds.update()

        if self.bar != (st // 16):
            self.bar = st // 16
            # self._group_highlight_redraw = [True] * 4

        ct = ins.captouch
        for i in range(4):
            if ct.petals[6 + i].pressed:
                if not self._group_highlight_on[i]:
                    self._group_highlight_redraw[i] = True
                self._group_highlight_on[i] = True
                for j in range(4):
                    if ct.petals[4 - j].pressed and not (
                        self.ct_prev.petals[4 - j].pressed
                    ):
                        self._tracks_empty = False
                        self.track_incr_state(self.track, self.bar * 16 + i * 4 + j)

                        self._render_list += [
                            (self.draw_track_step_marker, (self.track, i * 4 + j))
                        ]
            else:
                if self._group_highlight_on[i]:
                    self._group_highlight_redraw[i] = True
                self._group_highlight_on[i] = False

        if not ct.petals[5].pressed and (self.ct_prev.petals[5].pressed):
            if self.track_back:
                self.track_back = False
            else:
                self.track = (self.track - 1) % self.num_samplers
                self._render_list += [(self.draw_track_name, None)]

        if ct.petals[0].pressed and not (self.ct_prev.petals[0].pressed):
            if self.stopped:
                self.seq.signals.bpm = self.bpm
                self._render_list += [(self.draw_bpm, None)]
                self.stopped = False
                self.blm.foreground = True
            elif self.tapping:
                t = self.tap["time_ms"] * 0.001
                n = self.tap["count"]
                l = self.tap["last"]
                self.tap["sum_y"] += t
                self.tap["sum_x"] += n
                self.tap["sum_xy"] += n * t
                self.tap["sum_xx"] += n * n
                self.tap["last"] = t

                T = (
                    self.tap["sum_xy"] / n
                    - self.tap["sum_x"] / n * self.tap["sum_y"] / n
                ) / (
                    self.tap["sum_xx"] / n
                    - self.tap["sum_x"] / n * self.tap["sum_x"] / n
                )
                if t - l < T / 1.2 or t - l > T * 1.2 or T <= 0.12 or T > 1.5:
                    self.tapping = False
                else:
                    bpm = int(60 / T)
                    self.seq.signals.bpm = bpm
                    self._render_list += [(self.draw_bpm, None)]
                    self.bpm = bpm

            if not self.tapping:
                self.tap["sum_y"] = 0
                self.tap["sum_x"] = 1
                self.tap["sum_xy"] = 0
                self.tap["sum_xx"] = 1
                self.tap["count"] = 1
                self.tap["last"] = 0
                self.tap["time_ms"] = 0
                self.tapping = True

            self.tap["count"] += 1

        if ct.petals[0].pressed:
            if self.tap_tempo_press_counter > 500:
                self.seq.signals.bpm = 0
                self.seq.signals.sync_in.start()
                self._render_list += [(self.draw_bpm, None)]
                self.stopped = True
                self.blm.foreground = False
            else:
                self.tap_tempo_press_counter += delta_ms
        else:
            self.tap_tempo_press_counter = 0

        if ct.petals[5].pressed:
            if (self.track_back_press_counter > 500) and not self.track_back:
                self.track = (self.track + 1) % self.num_samplers
                self._render_list += [(self.draw_track_name, None)]
                self.track_back = True
            else:
                self.track_back_press_counter += delta_ms
        else:
            self.track_back_press_counter = 0

        self.tap["time_ms"] += delta_ms
        self.ct_prev = ct

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        self.ct_prev = None

    def on_enter_done(self) -> None:
        super().on_enter_done()
        # schedule one more redraw so draw_track_step_marker draws the real state
        self._render_list += [(self.draw_background, None)]

    def on_exit_done(self):
        self._bpm_saved = self.bpm
        self._steps_saved = self.steps
        self._save_settings()
        self._seq_table_saved = self.seq.plugins.seq.table
        if self.tracks_are_empty() or self.stopped:
            self.blm.background_mute_override = False
            self.blm.clear()
            self.blm.free = True
            self.blm = None
            self.init_complete = False
            self.samples_loaded = 0
            self.load_iter = self.iterate_loading()
        else:
            self.blm.background_mute_override = True
        super().on_exit_done()


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_app(GayDrums)
