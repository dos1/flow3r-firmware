from st3m.reactor import Responder
from st3m.goose import ABCBase, abstractmethod, Optional, List, Enum
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

    def on_exit(self) -> bool:
        """
        Called when the View is about to become inactive.
        If it returns True, think calls will continue to happen
        until on_exit_done.
        """
        return False

    def on_enter_done(self) -> None:
        """
        Called after a transition into the view has finished.
        """
        pass

    def on_exit_done(self) -> None:
        """
        Called after a transition out of the view has finished.
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
        if self.input:
            self.input._ignore_pressed()
        self.vm = vm

    def think(self, ins: InputState, delta_ms: int) -> None:
        if self.input:
            self.input.think(ins, delta_ms)

    def is_active(self) -> bool:
        if not self.vm:
            return False
        return self.vm.is_active(self)


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


class ViewTransitionNone(ViewTransition):
    """
    Immediate transition with no animation.
    """

    def draw(
        self, ctx: Context, transition: float, incoming: Responder, outgoing: Responder
    ) -> None:
        incoming.draw(ctx)


class ViewTransitionBlend(ViewTransition):
    """
    Transition from one view to another by opacity blending.
    """

    def draw(
        self, ctx: Context, transition: float, incoming: Responder, outgoing: Responder
    ) -> None:
        ctx.save()
        outgoing.draw(ctx)
        ctx.restore()

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


class ViewTransitionDirection(Enum):
    NONE = 1
    FORWARD = 2
    BACKWARD = 3


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

        self._pending: Optional[View] = None
        self._pending_vt: Optional[ViewTransition] = None
        self._pending_direction: Optional[ViewTransitionDirection] = None

        self._debug = debug

        # Transition time.
        self._time_ms = 150

        self._default_vt = vt
        self._overriden_vt: Optional[ViewTransition] = None

        self._transitioning = False
        self._transition = 0.0
        self._history: List[View] = []
        self._input = InputController()

        self._first_think = False
        self._fully_drawn = False

        self._outgoing_wants_to_think = False

    def _end_transition(self) -> None:
        if not self._transitioning:
            return

        self._transitioning = False

        if self._outgoing is not None:
            self._outgoing.on_exit_done()
            self._outgoing = None
        if self._incoming is not None:
            self._incoming.on_enter_done()

    def _perform_pending(self):
        if not self._pending:
            return
        self._end_transition()
        self._transitioning = True
        self._transition = 0.0
        self._first_think = True
        self._fully_drawn = False
        self._direction = self._pending_direction
        self._overriden_vt = self._pending_vt
        self._outgoing = self._incoming
        self._incoming = self._pending
        self._pending = None
        if self._outgoing is not None:
            self._outgoing_wants_to_think = self._outgoing.on_exit()
        self._incoming.on_enter(self)
        if self._outgoing is None:
            self._end_transition()

    def think(self, ins: InputState, delta_ms: int) -> None:
        self._input.think(ins, delta_ms)

        if self._transitioning:
            if not self._first_think:
                self._transition += (delta_ms / 1000.0) * (1000 / self._time_ms)
            else:
                self._first_think = False

            if self._transition >= 1.0:
                self._transition = 1.0
                if self._fully_drawn:
                    self._end_transition()

        if self._input.buttons.os.middle.pressed:
            if not self._history and self._debug:
                utime.sleep(0.5)
                machine.reset()
            else:
                self.pop(ViewTransitionSwipeRight())

        if self._outgoing is not None and self._outgoing_wants_to_think:
            self._outgoing.think(ins, delta_ms)
        if self._incoming is not None:
            self._incoming.think(ins, delta_ms)

        self._perform_pending()

    def draw(self, ctx: Context) -> None:
        if self._transitioning:
            if self._transition == 0.0:
                ctx.save()
                self._outgoing.draw(ctx)
                ctx.restore()
                return

            if self._transition >= 1.0:
                self._fully_drawn = True
                ctx.save()
                self._incoming.draw(ctx)
                ctx.restore()
                return

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

    def replace(
        self,
        r: View,
        override_vt: Optional[ViewTransition] = None,
        direction: ViewTransitionDirection = ViewTransitionDirection.NONE,
    ) -> None:
        """
        Replace the existing view with the given View, optionally using a given
        ViewTransition instead of the default.

        The new view will _not_ be added to history!
        """
        self._pending = r
        self._pending_vt = override_vt
        self._pending_direction = direction

    def push(self, r: View, override_vt: Optional[ViewTransition] = None) -> None:
        """
        Push a View to the history stack and start transitioning to it. If set,
        override_vt will be used instead of the default ViewTransition
        animation.
        """
        if self._pending is not None:
            self._history.append(self._pending)
        elif self._incoming is not None:
            self._history.append(self._incoming)

        self.replace(r, override_vt, ViewTransitionDirection.FORWARD)

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
            self.replace(r, override_vt, ViewTransitionDirection.BACKWARD)

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
        return (self._incoming == view and not self._pending) or self._pending == view

    @property
    def transitioning(self) -> bool:
        """
        Returns true if a transition is in progress.
        """
        return self._transitioning or self._pending

    @property
    def direction(self) -> ViewTransitionDirection:
        """
        Returns the direction in which the currently active view has became one:
          - ViewTransitionDirection.NONE if it has replaced another view
          - ViewTransitionDirection.FORWARD if it was pushed into the stack
          - ViewTransitionDirection.BACKWARD if another view was popped
        """
        return self._direction
