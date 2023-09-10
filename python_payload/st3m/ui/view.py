from st3m.reactor import Responder
from st3m.goose import ABCBase, abstractmethod, Optional, List
from st3m.input import InputState, InputController
from ctx import Context
import machine
import utime


class View(Responder):
    """
    A View extends a reactor Responder with callbacks related to the Responder's
    lifecycle in terms of being foregrounded or backgrounded.

    These signals can be used to alter input processing, se the
    BaseView class.
    """

    def on_enter(self, vm: Optional["ViewManager"]) -> None:
        """
        Called when the View has just become active.
        """
        pass

    def on_exit(self) -> None:
        """
        Called when the View is about to become inactive.
        """
        pass

    def show_icons(self) -> bool:
        """
        View should return True if it accepts having system icons drawn on top
        if it.
        """
        return False


class BaseView(View):
    """
    A base class helper for implementing views which respond to inputs and who
    want to do their own, separate input processing.

    Derive this class, then use self.input to access the InputController.

    Remember to call super().__init__() in __init__() and super().think() in think()!
    """

    __slots__ = ("input", "vm")

    def __init__(self) -> None:
        self.input = InputController()
        self.vm: Optional["ViewManager"] = None

    def on_enter(self, vm: Optional["ViewManager"]) -> None:
        self.input._ignore_pressed()
        self.vm = vm

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.input.think(ins, delta_ms)


class ViewTransition(ABCBase):
    """
    A transition from one View/Responder to another.

    Can be implemented by the user to provide transition animations.
    """

    @abstractmethod
    def draw(
        self, ctx: Context, transition: float, incoming: Responder, outgoing: Responder
    ) -> None:
        """
        Called when the ViewManager performs a transition from the outgoing
        responder to the incoming responder. The implementer should draw both
        Responders when appropriate.

        The transition value is a float from 0 to 1 which represents the
        progress of the transition.
        """
        pass


class ViewTransitionBlend(ViewTransition):
    """
    Transition from one view to another by opacity blending.
    """

    def draw(
        self, ctx: Context, transition: float, incoming: Responder, outgoing: Responder
    ) -> None:
        outgoing.draw(ctx)

        ctx.save()
        ctx.global_alpha = transition
        incoming.draw(ctx)
        ctx.restore()


class ViewTransitionSwipeLeft(ViewTransition):
    """
    Swipe the outoing view to the left and replace it with the incoming view.
    """

    def draw(
        self, ctx: Context, transition: float, incoming: Responder, outgoing: Responder
    ) -> None:
        ctx.save()
        ctx.translate(int(transition * -240), 0)
        outgoing.draw(ctx)
        ctx.restore()

        ctx.save()
        ctx.translate(240 + int(transition * -240), 0)
        ctx.rectangle(-120, -120, 240, 240)
        ctx.clip()
        incoming.draw(ctx)
        ctx.restore()


class ViewTransitionSwipeRight(ViewTransition):
    """
    Swipe the outoing view to the right and replace it with the incoming view.
    """

    def draw(
        self, ctx: Context, transition: float, incoming: Responder, outgoing: Responder
    ) -> None:
        ctx.save()
        ctx.translate(int(transition * 240), 0)
        outgoing.draw(ctx)
        ctx.restore()

        ctx.save()
        ctx.translate(-240 + int(transition * 240), 0)
        ctx.rectangle(-120, -120, 240, 240)
        ctx.clip()
        incoming.draw(ctx)
        ctx.restore()


class ViewManager(Responder):
    """
    The ViewManager implements stateful routing between Views.

    It manages a history of Views, to which new Views can be pushed and then
    popped.
    """

    def __init__(self, vt: ViewTransition, debug: bool) -> None:
        """
        Create a new ViewManager with a default ViewTransition.
        """
        self._incoming: Optional[View] = None
        self._outgoing: Optional[View] = None

        self._debug = debug

        # Transition time.
        self._time_ms = 150

        self._default_vt = vt
        self._overriden_vt: Optional[ViewTransition] = None

        self._transitioning = False
        self._transition = 0.0
        self._history: List[View] = []
        self._input = InputController()

    def think(self, ins: InputState, delta_ms: int) -> None:
        self._input.think(ins, delta_ms)

        if self._input.buttons.os.middle.pressed:
            if not self._history and self._debug:
                utime.sleep(0.5)
                machine.reset()
            else:
                self.pop(ViewTransitionSwipeRight())

        if self._transitioning:
            self._transition += (delta_ms / 1000.0) * (1000 / self._time_ms)
            if self._transition >= 1.0:
                self._transition = 0
                self._transitioning = False

                self._outgoing = None

        if self._outgoing is not None:
            self._outgoing.think(ins, delta_ms)
        if self._incoming is not None:
            self._incoming.think(ins, delta_ms)

    def draw(self, ctx: Context) -> None:
        if self._transitioning:
            vt = self._default_vt
            if self._overriden_vt is not None:
                vt = self._overriden_vt

            if self._incoming is not None and self._outgoing is not None:
                ctx.save()
                vt.draw(ctx, self._transition, self._incoming, self._outgoing)
                ctx.restore()
                return
        if self._incoming is not None:
            ctx.save()
            self._incoming.draw(ctx)
            ctx.restore()

    def replace(self, r: View, overide_vt: Optional[ViewTransition] = None) -> None:
        """
        Replace the existing view with the given View, optionally using a given
        ViewTransition instead of the default.

        The new view will _not_ be added to history!
        """
        self._outgoing = self._incoming
        if self._outgoing is not None:
            self._outgoing.on_exit()
        self._incoming = r
        self._incoming.on_enter(self)
        self._overriden_vt = overide_vt
        if self._outgoing is None:
            return

        self._transitioning = True
        self._transition = 0.0

    def push(self, r: View, override_vt: Optional[ViewTransition] = None) -> None:
        """
        Push a View to the history stack and start transitioning to it. If set,
        override_vt will be used instead of the default ViewTransition
        animation.
        """
        if self._incoming is not None:
            self._history.append(self._incoming)

        self.replace(r, override_vt)

    def pop(self, override_vt: Optional[ViewTransition] = None, depth: int = 1) -> None:
        """
        Pop a view (or more views) from the history stack and start transitioning.
        If set, override_vt will be used instead of the default ViewTransition
        animation.
        """
        r = None
        for i in range(depth):
            if len(self._history) < 1:
                break
            r = self._history.pop()
        if r:
            self.replace(r, override_vt)

    def wants_icons(self) -> bool:
        """
        Returns true if the current active view wants icon to be drawn on top of it.
        """
        if self._incoming is not None:
            return self._incoming.show_icons()
        else:
            return True

    def is_active(self, view: View) -> bool:
        """
        Returns true if the passed view is currently the active one.
        """
        return self._incoming == view
