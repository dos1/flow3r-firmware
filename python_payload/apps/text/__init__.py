from st3m.application import Application
from st3m.ui.interactions import CapScrollController

import sys_display


class App(Application):
    PETAL_NO = 2

    def __init__(self, app_ctx):
        super().__init__(app_ctx)
        self.delta_ms = 0
        self.offset = 0
        self.scan = 0
        self.scroll_pos = 0.0
        self.scroll = CapScrollController()
        self.content = "fnord"
        self.font_size = 12
        self.line_numbers = True
        self.wrap_word = True
        self.bg_dirty = True
        # self.load_path("/flash/sys/apps/text/__init__.py")
        # self.load_path("/flash/sys/st3m/run.py")

        # if app_ctx.arguments:
        #  self.load_path(app_ctx.arguments[0])
        # else:
        self.load_path("/flash/sys/apps/text/LGPL")

    def on_enter(self, vm):
        super().on_enter(vm)
        mode = 16 + sys_display.osd + sys_display.low_latency
        sys_display.set_mode(mode)
        self.relayout()

    def layout_iter(self):
        if self.layout_i >= len(self.contents):
            if len(self.curline) > 0:
                self.lines.append(self.curline)
            self.curline = ""
            return

        if self.wrap_word:
            for b in self.contents[self.layout_i]:
                if b == " ":
                    if len(self.curline + " " + self.curword) > self.cols:
                        self.lines.append(self.curline)
                        self.curline = self.curword
                        self.line_no.append("")
                    else:
                        self.curline += " " + self.curword
                    self.curword = ""
                elif b == "\n":
                    if len(self.curline + " " + self.curword) > self.cols:
                        self.lines.append(self.curline)
                        self.line_no.append("")
                        self.lines.append(self.curword)
                    else:
                        self.lines.append(self.curline + " " + self.curword)
                    self.curword = ""
                    self.curline = ""
                    self.line += 1
                    self.line_no.append(str(self.line))
                else:
                    self.curword += b
        else:
            for b in self.contents[self.layout_i]:
                if len(self.curline) > self.cols:
                    self.line_no.append("")
                    self.lines.append(self.curline)
                    self.curline = b
                elif b == "\n":
                    self.lines.append(self.curline)
                    self.line += 1
                    self.line_no.append(str(self.line))
                    self.curline = ""
                else:
                    self.curline += b
        self.layout_i += 1

    def relayout(self):
        if self.line_numbers:
            self.x0 = self.font_size * 1.9
        else:
            self.x0 = self.font_size * 1
        self.cols = int((240 - self.x0) / self.font_size * 1.9)

        self.bg_dirty = True
        self.viewport_lines = int(240 / self.font_size + 8)
        self.viewport_height = self.font_size * self.viewport_lines
        self.scroll.position = (0.0, 0.0)
        self.scroll.momentum = (0.0, 0.0)
        self.drawn = []
        for i in range(self.viewport_lines):
            self.drawn.append(-1)
        self.layout_i = 0
        self.curline = ""
        self.curword = ""
        self.line = 1
        sys_display.fbconfig(240, self.viewport_height, 0, 0)
        self.lines = []
        self.line_no = []
        self.line_no.append(str(self.line))

        self.contents = ["".join(item) for item in zip(*[iter(self.content)] * 512)]
        self.layout_iter()

    def load_path(self, path):
        self.path = path
        try:
            with open(self.path, "r", encoding="utf-8") as f:
                self.content = f.read()
        except:
            self.content = "File Error"
        self.relayout()

    def think(self, ins, delta_ms):
        super().think(ins, delta_ms)
        self.scroll_pos += 50 * self.delta_ms / 1000.0
        self.scroll.update(self.input.captouch.petals[self.PETAL_NO].gesture, delta_ms)
        if self.input.buttons.app.left.pressed:
            new_size = self.font_size - 1
            if new_size >= 6:
                self.font_size = new_size
                self.relayout()
        if self.input.buttons.app.right.pressed:
            self.font_size += 1
            self.relayout()

    def drawline(self, ctx, no):
        slot = int(no - self.offset / self.font_size)

        offset = self.offset
        if slot >= self.viewport_lines:
            slot -= self.viewport_lines
            offset += self.viewport_height
        if self.drawn[slot] != no:
            self.drawn[slot] = no
            y = no * self.font_size - offset
            ctx.rectangle(0, y, 240, self.font_size)
            ctx.gray(0).fill()
            if no < 0 or no >= len(self.lines):
                no = -1
            if no >= 0:
                ctx.gray(1)

                ctx.move_to(self.x0, y + self.font_size * 0.8).text(self.lines[no])
                if self.line_numbers:
                    ctx.save()
                    ctx.text_align = ctx.RIGHT
                    ctx.gray(0.5)
                    ctx.move_to(self.x0 - 4, y + self.font_size * 0.8).text(
                        self.line_no[no]
                    )
                    ctx.restore()

    def draw(self, ctx):
        if self.vm.transitioning:
            return
        if self.bg_dirty:
            ctx.gray(0).rectangle(-120, -120, 240, 480).fill()
            self.bg_dirty = False
        _, self.scroll_pos = self.scroll.position
        self.scroll_pos = (self.scroll_pos * -2) - 100

        ctx.font = "Mono"
        ctx.font_size = self.font_size
        ctx.translate(-120, -120)
        self.offset = (
            int((self.scroll_pos + self.viewport_height * 64) / self.viewport_height)
        ) * self.viewport_height - self.viewport_height * 64
        self.scan = (
            int(self.scroll_pos + self.viewport_height * 64) % self.viewport_height
        )
        self.layout_iter()

        first_line = int((self.offset + self.scan) / self.font_size) - 1
        for i in range(240 / self.font_size + 2):
            self.drawline(ctx, first_line + i)

        sys_display.fbconfig(240, self.viewport_height, 0, self.scan)


if __name__ == "__main__":
    from st3m.run import run_app

    run_app(App)
