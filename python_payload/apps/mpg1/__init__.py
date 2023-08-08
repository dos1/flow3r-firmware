from st3m.application import Application, ApplicationContext
from st3m.input import InputState
import st3m.run
import media
from ctx import Context

class MediaMpgVideo(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        # self._filename = "/sd/toms_diner.mp3"
        # self._filename = "/sd/fanitullen.mp3"
        self._filename = "/sd/alien.mpg"
        # self._filename = "/sd/bapple.mpg"
        # self._filename = "/sd/video.mpg"
        # self._filename = "/sd/bunny.mpg"
        # self._filename = "/sd/bjork-160x120.mpg"

    def draw(self, ctx: Context) -> None:
        media.draw(ctx)

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        media.load(self._filename)

    def on_exit(self) -> None:
        media.stop()

if __name__ == '__main__':
    st3m.run.run_view(MediaMpgVideo(ApplicationContext()))
