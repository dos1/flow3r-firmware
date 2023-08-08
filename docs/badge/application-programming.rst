.. _application_programming:

Application Programming
=======================

*NOTE: this section shows higher level application programming concepts. Please first consult the
:ref:`programming` section on how to get started.*


Basics
------

Implementing a responsive user interface on a resource constrained device which
at the same time should also output glitch free audio is not the easiest task in
the world. The flow3r application programming environment tries make it a bit
easier for you.

There are two major components to the running an app on the flower: the
:py:class:`Reactor` and at least one or more :py:class:`Responder` s.
The Reactor is a component which comes with the flow3r and takes care of all
the heavy lifting for you. It decides when it is time to draw something on the
display and it also gathers the data from a whole bunch of inputs like captouch
or the buttons for you to work with.

A responder is a software component which can get called by the Reactor and is
responsible to react to the input data and when asked draw something to the screen.

Example 1a: Display something
-------------------------------
Let's have a look at a very simple example involving a responder:

.. code-block:: python

    from st3m.reactor import Responder
    import st3m.run

    class Example(Responder):
        def __init__(self) -> None:
            pass

        def draw(self, ctx: Context) -> None:
            # Paint the background black
            ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()

            # Paint a red square in the middle of the display
            ctx.rgb(255, 0, 0).rectangle(-20, -20, 40, 40).fill()

        def think(self, ins: InputState, delta_ms: int) -> None:
            pass


    st3m.run.run_responder(Example())

You can save this example as a Python file (e.g. example.py) and run it using
``mpremote run example.py``. It should display a red square in the middle of
the display and do nothing else.

You might already be able to guess the meaning of the three things that a responder
has to implement:

+---------------+------------------------------------------------------------+
| Function      | Meaning                                                    |
+===============+============================================================+
| `__init__()`  | Called once before any of the other methods is run.        |
+---------------+------------------------------------------------------------+
| `draw()`      | Called each time the display should be drawn.              |
+---------------+------------------------------------------------------------+
| `think()`     | Called regularly with the latest input and sensor readings |
+---------------+------------------------------------------------------------+

It's important to note that none of these methods is allowed take a significant
amount of time if you want the user interface of the flow3r to feel snappy. You
also need to make sure that each time `draw()` is called, everything you want
to show is drawn again. Otherwise you will experience strange flickering or other
artifacts on the screen.


Example 1b: React to input
--------------------------

If we want to react to the user, we can use the :py:class:`InputState` which got
handed to us. In this example we look at the state of the right shoulder button.
The values contained in the input state are the same as used by the
:py:mod:`hardware` module.

.. code-block:: python

    from st3m.reactor import Responder
    from hardware import BUTTON_PRESSED_LEFT, BUTTON_PRESSED_RIGHT, BUTTON_PRESSED_DOWN
    import st3m.run

    class Example(Responder):
        def __init__(self) -> None:
            self._x = -20

        def draw(self, ctx: Context) -> None:
            # Paint the background black
            ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()

            # Paint a red square in the middle of the display
            ctx.rgb(255, 0, 0).rectangle(self._x, -20, 40, 40).fill()

        def think(self, ins: InputState, delta_ms: int) -> None:
            direction = ins.left_button

            if direction == BUTTON_PRESSED_LEFT:
                self._x -= 1
            elif direction == BUTTON_PRESSED_RIGHT:
                self._x += 1


    st3m.run.run_responder(Example())

Try it: when you run this code, you can move the red square using the left shoulder button.


Example 1c: Taking time into consideration
------------------------------------------

The previous example moved the square around, but could you tell how fast it moved across
the screen? What if you wanted it to move exactly 20 pixels per second to the left
and 20 pixels per second to the right?

The `think()` method has an additional parameter we can use for this: `delta_ms`. It
represents the time which has passed since the last call to `think()`.

.. code-block:: python

    from st3m.reactor import Responder
    from hardware import BUTTON_PRESSED_LEFT, BUTTON_PRESSED_RIGHT, BUTTON_PRESSED_DOWN
    import st3m.run

    class Example(Responder):
        def __init__(self) -> None:
            self._x = -20.

        def draw(self, ctx: Context) -> None:
            # Paint the background black
            ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()

            # Paint a red square in the middle of the display
            ctx.rgb(255, 0, 0).rectangle(self._x, -20, 40, 40).fill()

        def think(self, ins: InputState, delta_ms: int) -> None:
            direction = ins.left_button # -1 (left), 1 (right), or 2 (pressed)

            if direction == BUTTON_PRESSED_LEFT:
                self._x -= 20 * delta_ms / 1000
            elif direction == BUTTON_PRESSED_RIGHT:
                self._x += 40 * delta_ms / 1000


    st3m.run.run_responder(Example())

This becomes important if you need exact timings in your application,
as the Reactor makes no explicit guarantee about how often `think()` will
be called. Currently we are shooting for once every 20 milliseconds, but if something in the system
takes a bit longer to process something, this number can change from one call to the next.


Example 1d: Automatic input processing
--------------------------------------

Working on the bare state of the buttons and the captouch petals can be cumbersome and error prone.
the flow3r application framework gives you a bit of help in the form of the :py:class:`InputController`
which processes an input state and gives you higher level information about what is happening.

The following example shows how to properly react to single button presses without having to
think about what happens if the user presses the button for a long time. It uses the `InputController`
to detect single button presses and switches between showing a circle (by drawing a 360 deg arc) and
a square.


.. code-block:: python

    from st3m.reactor import Responder
    from st3m.input import InputController
    from st3m.utils import tau

    from hardware import BUTTON_PRESSED_LEFT, BUTTON_PRESSED_RIGHT, BUTTON_PRESSED_DOWN
    import st3m.run

    class Example(Responder):
        def __init__(self) -> None:
            self.input = InputController()
            self._x = -20.
            self._draw_rectangle = True

        def draw(self, ctx: Context) -> None:
            # Paint the background black
            ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()

            # Paint a red square in the middle of the display
            if self._draw_rectangle:
                ctx.rgb(255, 0, 0).rectangle(self._x, -20, 40, 40).fill()
            else:
                ctx.rgb(255, 0, 0).arc(self._x, -20, 40, 0, tau, 0).fill()

        def think(self, ins: InputState, delta_ms: int) -> None:
            self.input.think(ins, delta_ms) # let the input controller to its magic

            direction = ins.left_button # -1 (left), 1 (right), or 2 (pressed)

            if self.input.right_shoulder.middle.pressed:
                self._draw_rectangle = not self._draw_rectangle

            if direction == BUTTON_PRESSED_LEFT:
                self._x -= 20 * delta_ms / 1000
            elif direction == BUTTON_PRESSED_RIGHT:
                self._x += 40 * delta_ms / 1000


    st3m.run.run_responder(Example())


Managing multiple views
----------------------------------------

If you want to write a more advanced application you probably also want to display more than
one screen (or view as we call them).
With just the Responder class this can become a bit tricky as it never knows when it is visible and
when it is not. It also doesn't directly allow you to launch a new screen.

To help you with that you can use a :py:class:`View` instead. It can tell you when
it becomes visible and you can also use it to bring a new screen or widget into the foreground or remove
it again from the screen.

Example 2a: Managing two views
--------------------------------

In this example we use a basic `View` to switch between to different screens using a button. One screen
shows a red square, the other one a green square. You can of course put any kind of complex processing
into the two different views. We make use of an `InputController` again to handle the button presses.


.. code-block:: python

    from st3m.input import InputController
    from st3m.ui.view import View
    import st3m.run

    class SecondScreen(View):
        def __init__(self) -> None:
            self.input = InputController()
            self._vm = None

        def on_enter(self, vm: Optional[ViewManager]) -> None:
            self._vm = vm

            # Ignore the button which brought us here until it is released
            self.input._ignore_pressed()

        def draw(self, ctx: Context) -> None:
            # Paint the background black
            ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
            # Green square
            ctx.rgb(0, 255, 0).rectangle(-20, -20, 40, 40).fill()

        def think(self, ins: InputState, delta_ms: int) -> None:
            self.input.think(ins, delta_ms) # let the input controller to its magic

            if self.input.right_shoulder.middle.pressed:
                self._vm.pop()


    class Example(View):
        def __init__(self) -> None:
            self.input = InputController()
            self._vm = None

        def draw(self, ctx: Context) -> None:
            # Paint the background black
            ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
            # Red square
            ctx.rgb(255, 0, 0).rectangle(-20, -20, 40, 40).fill()


        def on_enter(self, vm: Optional[ViewManager]) -> None:
            self._vm = vm

        def think(self, ins: InputState, delta_ms: int) -> None:
            self.input.think(ins, delta_ms) # let the input controller to its magic

            if self.input.right_shoulder.middle.pressed:
                self._vm.push(SecondScreen())

    st3m.run.run_view(Example())

Try it using `mpremote`. The right shoulder button switches between the two views. To avoid that
the still pressed button immediately closes `SecondScreen` we make us of a special method of the
`InputController` which hides the pressed button from the view until it is released again.

Example 2b: Easier view management
----------------------------------

The idea that a button (physical or captouch) is used to enter / exit a view is so universal that
there is a special view which helps you with that: :py:class:`ViewWithInputState`. It integrates an
`InputController` and handles the ignoring of extra presses:

.. code-block:: python

    from st3m.ui.view import ViewWithInputState
    import st3m.run

    class SecondScreen(ViewWithInputState):
        def __init__(self) -> None:
            super().__init__()

        def on_enter(self, vm: Optional[ViewManager]) -> None:
            super().on_enter(vm) # Let ViewWithInputState do its thing

        def draw(self, ctx: Context) -> None:
            # Paint the background black
            ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
            # Green square
            ctx.rgb(0, 255, 0).rectangle(-20, -20, 40, 40).fill()

        def think(self, ins: InputState, delta_ms: int) -> None:
            super().think(ins, delta_ms) # Let ViewWithInputState do its thing

            if self.input.right_shoulder.middle.pressed:
                self.vm.pop()


    class Example(ViewWithInputState):
        def __init__(self) -> None:
            super().__init__()

        def draw(self, ctx: Context) -> None:
            # Paint the background black
            ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
            # Red square
            ctx.rgb(255, 0, 0).rectangle(-20, -20, 40, 40).fill()


        def on_enter(self, vm: Optional[ViewManager]) -> None:
            super().on_enter(vm) # Let ViewWithInputState do its thing

        def think(self, ins: InputState, delta_ms: int) -> None:
            super().think(ins, delta_ms) # Let ViewWithInputState do its thing

            if self.input.right_shoulder.middle.pressed:
                self.vm.push(SecondScreen())

    st3m.run.run_view(Example())



Writing an application for the menu system
------------------------------------------

All fine and good, you were able to write an application that you can run with `mpremote`,
but certainly you also want to run it from flow3r's menu system.

Let's introduce the final class you should actually be using for application development:
:py:class:`Application` (yeah, right).

.. code-block:: python

    from st3m.application import Application
    import st3m.run

    class SecondScreen(ViewWithInputState):
        def __init__(self) -> None:
            super().__init__()

        def on_enter(self, vm: Optional[ViewManager]) -> None:
            super().on_enter(vm) # Let ViewWithInputState do its thing

        def draw(self, ctx: Context) -> None:
            # Paint the background black
            ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
            # Green square
            ctx.rgb(0, 255, 0).rectangle(-20, -20, 40, 40).fill()

        def think(self, ins: InputState, delta_ms: int) -> None:
            super().think(ins, delta_ms) # Let ViewWithInputState do its thing

            if self.input.right_shoulder.middle.pressed:
                self.vm.pop()


    class MyDemo(Application):
        def __init__(self) -> None:
            super().__init__(name="My demo")

        def draw(self, ctx: Context) -> None:
            # Paint the background black
            ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
            # Red square
            ctx.rgb(255, 0, 0).rectangle(-20, -20, 40, 40).fill()

        def on_enter(self, vm: Optional[ViewManager]) -> None:
            super().on_enter(vm) # Let Application do its thing

        def think(self, ins: InputState, delta_ms: int) -> None:
            super().think(ins, delta_ms) # Let Application do its thing

            if self.input.right_shoulder.middle.pressed:
                self._view_manager.push(SecondScreen())

    st3m.run.run_view(Example())

The `Application` class gives you the following extras:

 - Pressing the down the left shoulder button exits the app
 - You get a `_view_manager` member to manager your views
 - It can be picked up by the main menu system


To add the application to the menu we are missing one more thing: a `flow3r.toml`
file which describes the application so flow3r knows where to put it in the menu system.
Together with the Python code this file forms a so called bundle
(see also :py:class:`BundleMetadata`).

-- code-block::

    [app]
    name = "My Demo"
    menu = "Apps"

    [entry]
    class = "MyDemo"

    [metadata]
    author = "You :)"
    license = "pick one, LGPL/MIT maybe?"
    url = "https://git.flow3r.garden/you/mydemo"


Save this as `flow3r.toml` together with the Python code as `__init__.py` in a folder (name doesn't matter)
and put that folder into the `apps` folder on your flow3r (if there is no `apps` folder visible,
there might be an `apps` folder in the `sys` folder). Restart the flow3r and it should pick up your
new application.


Using the simulator
-------------------

The simulator deserves its own page in the docs. For now have a look at the "Firmware Development" page
to see how to set it up.

The simulators apps live in `python_payload/apps` copy you app folder in there and it will appear in
the simulators menu system. Currently the simulator supports the display, LEDs, the buttons and some
static input values from the accelerometer, gyroscope, temperature sensor and pressure sensor.

No audio output on the simulator yet. Want to step up and get it in?

