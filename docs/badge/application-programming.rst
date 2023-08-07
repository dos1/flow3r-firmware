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
