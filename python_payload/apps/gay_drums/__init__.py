import bl00mbox
import captouch
import leds

from st3m.application import Application, ApplicationContext
from st3m.input import InputState
from st3m.goose import Tuple, Iterator, Optional, Callable, List, Any, TYPE_CHECKING
from ctx import Context
from st3m.ui.view import View, ViewManager

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
        self.blm = bl00mbox.Channel("gay drums")
        self.num_samplers = 6
        self.seq = self.blm.new(bl00mbox.patches.sequencer, num_tracks=8, num_steps=32)
        self.bar = self.seq.signals.step.value // 16
        self.set_bar_mode(0)

        self.kick: Optional[bl00mbox.patches._Patch] = None
        self.hat: Optional[bl00mbox.patches._Patch] = None
        self.close: Optional[bl00mbox.patches._Patch] = None
        self.open: Optional[bl00mbox.patches._Patch] = None
        self.crash: Optional[bl00mbox.patches._Patch] = None
        self.snare: Optional[bl00mbox.patches._Patch] = None

        self.seq.signals.bpm.value = 80

        self.track_names = ["kick", "hihat", "snare", "crash", "nya", "woof"]
        self.ct_prev = captouch.read()
        self.track = 1
        self.blm.background_mute_override = True
        self.tap_tempo_press_counter = 0
        self.track_back_press_counter = 0
        self.delta_acc = 0
        self.stopped = False
        self.track_back = False
        self.bpm = self.seq.signals.bpm.value

        self.samples_loaded = 0
        self.samples_total = len(self.track_names)
        self.loading_text = ""
        self.init_complete = False
        self.load_iter = self.iterate_loading()
        self._render_list_2: List[Tuple[Rendee, Any]] = []
        self._render_list_1: List[Tuple[Rendee, Any]] = []

        self._group_highlight_on = [False] * 4
        self._group_highlight_redraw = [False] * 4
        self._redraw_background = 2
        self._group_gap = 5
        self.background_col: Color = (0, 0, 0)
        self.highlight_col: Color = (0.15, 0.15, 0.15)

    def set_bar_mode(self, mode: int) -> None:
        # TODO: figure out how to speed up re-render
        if mode == 0:
            self.seq.signals.step_start = 0
            self.seq.signals.step_end = 15
        if mode == 1:
            self.seq.signals.step_start = 16
            self.seq.signals.step_end = 31
        if mode == 2:
            self.seq.signals.step_start = 0
            self.seq.signals.step_end = 31

    def iterate_loading(self) -> Iterator[Tuple[int, str]]:
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

        # group bars
        bar_len = 10 * (1 + self.num_samplers)
        bar_pos = 4 * 12 + self._group_gap
        self.ctx_draw_centered_rect(ctx, 0, 0, 1, bar_len, (0.5, 0.5, 0.5))
        self.ctx_draw_centered_rect(ctx, bar_pos, 0, 1, bar_len, (0.5, 0.5, 0.5))
        self.ctx_draw_centered_rect(ctx, -bar_pos, 0, 1, bar_len, (0.5, 0.5, 0.5))

        ctx.font = ctx.get_font_name(1)

        ctx.font_size = 15
        ctx.rgb(0.6, 0.6, 0.6)

        ctx.move_to(0, -85)
        ctx.text("(hold) stop")

        ctx.move_to(0, -100)
        ctx.text("tap tempo")

        self.draw_track_name(ctx, None)
        self.draw_bpm(ctx, None)

        for track in range(self.num_samplers):
            for step in range(16):
                self.draw_track_step_marker(ctx, (track, step))

    def track_get_state(self, track: int, step: int) -> int:
        sequencer_track = track
        if track > 1:
            sequencer_track += 2
        if track == 1:
            if self.seq.trigger_state(1, step):
                return 1
            elif self.seq.trigger_state(2, step):
                return 2
            elif self.seq.trigger_state(3, step):
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
        if track > 1:
            sequencer_track += 2
        if track == 1:
            state = self.track_get_state(track, step)
            if state == 0:
                self.seq.trigger_start(1, step)
                self.seq.trigger_clear(2, step)
                self.seq.trigger_clear(3, step)
            if state == 1:
                self.seq.trigger_clear(1, step)
                self.seq.trigger_start(2, step)
                self.seq.trigger_clear(3, step)
            if state == 2:
                self.seq.trigger_clear(1, step)
                self.seq.trigger_clear(2, step)
                self.seq.trigger_start(3, step)
            if state == 3:
                self.seq.trigger_clear(1, step)
                self.seq.trigger_clear(2, step)
                self.seq.trigger_clear(3, step)
        else:
            state = self.seq.trigger_state(sequencer_track, step)
            if state == 0:
                new_state = 16000
            elif state == 32767:
                new_state = 0
            else:
                new_state = 32767
            self.seq.trigger_start(sequencer_track, step, new_state)

    def draw_track_step_marker(self, ctx: Context, data: Tuple[int, int]) -> None:
        track, step = data
        self._group_gap = 4
        rgb = self._track_col(track)
        rgbf = (rgb[0] / 256, rgb[1] / 256, rgb[2] / 256)
        y = -int(12 * (track - (self.num_samplers - 1) / 2))
        trigger_state = self.track_get_state(track, step)
        size = 2
        x = 12 * (7.5 - step)
        x += self._group_gap * (1.5 - (step // 4))
        x = int(-x)
        group = step // 4
        bg = self.background_col
        if self._group_highlight_on[group]:
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

    def draw_group_highlight(self, ctx: Context, data: int) -> None:
        i = data
        col = self.background_col
        if self._group_highlight_on[i]:
            col = self.highlight_col
        sizex = 48 + self._group_gap - 2
        sizey = 10 * (1 + self.num_samplers)
        posx = -int((12 * 4 + 1 + self._group_gap) * (1.5 - i))
        posy = 0
        self.ctx_draw_centered_rect(ctx, posx, posy, sizex, sizey, col)

        for x in range(self.num_samplers):
            for y in range(4):
                self.draw_track_step_marker(ctx, (x, y + 4 * i))

    def draw_bpm(self, ctx: Context, data: None) -> None:
        self.ctx_draw_centered_rect(ctx, 0, -65, 200, 22, (0, 0, 0))
        bpm = self.seq.signals.bpm.value
        ctx.font = ctx.get_font_name(1)
        ctx.font_size = 20

        ctx.move_to(0, -65)
        ctx.rgb(255, 255, 255)
        ctx.text(str(bpm) + " bpm")

    def draw(self, ctx: Context) -> None:
        if not self.init_complete:
            ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()

            ctx.font = ctx.get_font_name(0)
            ctx.text_align = ctx.MIDDLE
            ctx.font_size = 24
            ctx.move_to(0, -10)
            ctx.rgb(0.8, 0.8, 0.8)
            if self.samples_loaded == self.samples_total:
                ctx.text("Loading complete")
            else:
                ctx.text("Loading samples...")

            ctx.font_size = 16
            ctx.move_to(0, 10)
            ctx.text(self.loading_text)

            progress = self.samples_loaded / self.samples_total
            bar_len = 120 / self.samples_total
            for x in range(self.samples_loaded):
                rgb = self._track_col(x)
                rgbf = (rgb[0] / 256, rgb[1] / 256, rgb[2] / 256)
                ctx.rgb(*rgbf)
                ctx.rectangle(x * bar_len - 60, 30, bar_len, 10).fill()
            return

        for i in range(4):
            if self._group_highlight_redraw[i]:
                self._group_highlight_redraw[i] = False
                self._render_list_1 += [(self.draw_group_highlight, i)]

        for rendee in self._render_list_1:
            fun, param = rendee
            fun(ctx, param)
        for rendee in self._render_list_2:
            fun, param = rendee
            fun(ctx, param)
        self._render_list_2 = self._render_list_1.copy()
        self._render_list_1 = []

        size = 4

        st = self.seq.signals.step.value

        stepx = -12 * (7.5 - st)
        stepx -= self._group_gap * (1.5 - (st // 4))
        stepy = -12 - 5 * self.num_samplers

        trigger_state = self.track_get_state(self.track, st)
        dotsize = 1
        if trigger_state:
            dotsize = 4
        self.ctx_draw_centered_rect(ctx, 0, stepy, 200, 4, self.background_col)
        self.ctx_draw_centered_rect(ctx, stepx, stepy, dotsize, dotsize, (1, 1, 1))

    def draw_track_name(self, ctx: Context, data: None) -> None:
        self.ctx_draw_centered_rect(ctx, 0, 60, 200, 30, self.background_col)
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
            try:
                self.samples_loaded, self.loading_text = next(self.load_iter)
            except StopIteration:
                self.init_complete = True
                self._render_list_1 += [(self.draw_background, None)]
            return

        st = self.seq.signals.step.value
        rgb = self._track_col(self.track)
        leds.set_all_rgb(0, 0, 0)
        self._highlight_petal(10 - (4 - (st // 4)), *rgb)
        self._highlight_petal(10 - (6 + (st % 4)), *rgb)
        leds.update()

        if self.bar != (st // 16):
            self.bar = st // 16
            self._group_highlight_redraw = [True] * 4

        ct = captouch.read()
        for i in range(4):
            if ct.petals[6 + i].pressed:
                if not self._group_highlight_on[i]:
                    self._group_highlight_redraw[i] = True
                self._group_highlight_on[i] = True
                for j in range(4):
                    if ct.petals[4 - j].pressed and not (
                        self.ct_prev.petals[4 - j].pressed
                    ):
                        self.track_incr_state(self.track, self.bar * 16 + i * 4 + j)

                        self._render_list_1 += [
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
                self._render_list_1 += [(self.draw_track_name, None)]

        if ct.petals[0].pressed and not (self.ct_prev.petals[0].pressed):
            if self.stopped:
                self.seq.signals.bpm = self.bpm
                self._render_list_1 += [(self.draw_bpm, None)]
                self.blm.background_mute_override = True
                self.stopped = False
            elif self.delta_acc < 3000 and self.delta_acc > 10:
                bpm = int(60000 / self.delta_acc)
                if bpm > 40 and bpm < 500:
                    self.seq.signals.bpm = bpm
                    self._render_list_1 += [(self.draw_bpm, None)]
                    self.bpm = bpm
            self.delta_acc = 0

        if ct.petals[0].pressed:
            if self.tap_tempo_press_counter > 500:
                self.seq.signals.bpm = 0
                self._render_list_1 += [(self.draw_bpm, None)]
                self.stopped = True
                self.blm.background_mute_override = False
            else:
                self.tap_tempo_press_counter += delta_ms
        else:
            self.tap_tempo_press_counter = 0

        if ct.petals[5].pressed:
            if (self.track_back_press_counter > 500) and not self.track_back:
                self.track = (self.track + 1) % self.num_samplers
                self._render_list_1 += [(self.draw_track_name, None)]
                self.track_back = True
            else:
                self.track_back_press_counter += delta_ms
        else:
            self.track_back_press_counter = 0

        if self.delta_acc < 3000:
            self.delta_acc += delta_ms
        self.ct_prev = ct

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        self._render_list_1 += [(self.draw_background, None)]
        self._render_list_1 += [(self.draw_background, None)]  # nice
