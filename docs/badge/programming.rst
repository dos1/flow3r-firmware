.. _programming:

Programming
===========

*NOTE: this is about programming your own apps/instruments/toys while keeping the badge firmware stock. If you want to port Doom or work on the firmware, see the Firmware Development section.*

The main programming interface and language for the flow3rbadge is Python. More
exactly, it's `Micropython <https://micropython.org/>`_, which is a fairly
sizeable subset of Python that can run on microcontrollers.

Good news: if you've ever used Micropython on an ESP32, then you probably
already have all the tools required to get started. However, while the tools to
program the badge might be the same as for stock Micropython on ESP32, our APIs
are quite different.

If you haven't used Micrpython, don't worry. It's not that difficult other than
wrapping your head around file access.

The :ref:`st3m` framework is the main Python codebase you'll be writing against.
Instead of using standard Micropython libraries like ``machine`` or low level
display drivers, you'll be writing applications that implement st3m classes like
:py:class:`Responder` or :py:class:`Application`.

But, enough intro for now, let's get started.

Accessing the badge
-------------------

When the badge runs (for example, when you see the main menu), you can connect
it to a PC and it should appear as a serial device. On Linux systems, this
device will be usually called ``/dev/ttyACM0`` (sometimes ``/dev/ttyACM1``).

You can then use any terminal emulator program (like picocom, GNU screen, etc)
to access the badge's runtime logs. Even better, use a dedicated
micropython-specific program, as that will actually let you transfer files.
These are the tools we've tested and are known to work:

+---------------+-----------------------+
| Tool          | Platforms             |
+===============+=======================+
| mpremote_     | Linux, macOS, Windows |
+---------------+-----------------------+
| `Micro REPL`_ | Android               |
+---------------+-----------------------+

.. _mpremote: https://docs.micropython.org/en/latest/reference/mpremote.html
.. _`Micro REPL`: https://github.com/Ma7moud3ly/micro-repl

In the rest of these docs we'll use mpremote. But you should be able to follow
along with any of the aforementioned tools. If you are on Linux and your flow3r
came up as ``/dev/ttyACM1``, add an ``a1`` after ``mpremote``.

After connecting your badge and making sure it runs:

::

	$ mpremote
	Connected to MicroPython at /dev/ttyACM0
	Use Ctrl-] or Ctrl-x to exit this shell
	[... logs here... ]

The badge will continue to run.

.. warning::
   **Your flow3r is not showing up using Linux?**

   To let ``mpremote`` to work properly your user needs to have access rights to ttyACM.

   Quick fix: ``sudo chmod a+rw /dev/ttyACM[Your Device Id here]```

   More sustainable fix: Setup an udev rule to automatically allow the logged in user to access ttyUSB

	    1. To use this, add the following to /etc/udev/rules.d/60-extra-acl.rules: ``KERNEL=="ttyACM[0-9]*", TAG+="udev-acl", TAG+="uaccess"``
	    2. Reload ``udevadm control --reload-rules && udevadm trigger``
	

Now, if you press Ctrl-C, you will interrupt the
firmware and break into a Python REPL (read-eval-print-loop) prompt:

::

	Traceback (most recent call last):
	  File "/flash/sys/main.py", line 254, in <module>
	  [... snip ...]
	KeyboardInterrupt:
	MicroPython c48f94151-dirty on 1980-01-01; badge23 with ESP32S3
	Type "help()" for more information.
	>>>

The badge's display will now switch to 'In REPL' to indicate that software
execution has been interrupted and that the badge is waiting for a command over
REPL.

Congratulations! You can now use your badge as a calculator:

::

	>>> 5 + 5
	10

But that's not super interesting. Let's try to turn on some LEDs:

::

	>>> import leds
	>>> leds.set_rgb(0, 255, 0, 0)
	>>> leds.update()

The LED right next to the USB connector should light up red. You can continue
experimenting with different APIs (like :py:mod:`leds`, :py:mod:`audio`, etc).

Transferring files over REPL
----------------------------

You can also access the filesystem over the same Micropython serial port:

::

	$ mpremote
	MicroPython c48f94151-dirty on 1980-01-01; flow3r with ESP32S3
	Type "help()" for more information.
	>>> import os
	>>> os.listdir('/')
	['flash']
	>>> os.listdir('/flash/sys')
	['main.py', 'st3m', '.sys-installed']
	>>>

	$ mpremote ls :/flash/sys
	ls :/flash/sys
	           0 main.py
	           0 st3m
	           0 .sys-installed

.. _disk mode:

Disk Mode
---------

For larger file transfers (eg. images, sound samples, etc.) you can put the
badge into Disk Mode by selecting ``Settings -> Disk Mode`` in the badge's menu.

You can then select whether to mount the 10MiB internal flash or SD card (if
present) as a pendrive. The selected device will then appear as a pendrive on
your system, and will stay until it is ejected. The serial connection will
disconnect for the duration of the badge being in disk mode.

Disk Mode can also be enabled when the badge is in :ref:`Recovery mode`.

Writing Applications
--------------------

Once you feel some familiary with the REPL, you're ready to advance to the next
chapter: writing full-fledged applications that can draw graphics on the screen,
respond to input and play sound!

Basics
^^^^^^

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
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
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
^^^^^^^^^^^^^^^^^^^^^^^^^^

If we want to react to the user, we can use the :py:class:`InputState` which got
handed to us. In this example we look at the state of the app (by default left)
shoulder button. The values for buttons contained in the input state are one of
``InputButtonState.PRESSED_LEFT``, ``PRESSED_RIGHT``, ``PRESSED_DOWN``,
``NOT_PRESSED`` - same values as in the low-level
:py:mod:`sys_buttons`.

.. code-block:: python

    from st3m.reactor import Responder
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
            direction = ins.buttons.app

            if direction == ins.buttons.PRESSED_LEFT:
                self._x -= 1
            elif direction == ins.buttons.PRESSED_RIGHT:
                self._x += 1


    st3m.run.run_responder(Example())

Try it: when you run this code, you can move the red square using the app (by
default left) shoulder button.


Example 1c: Taking time into consideration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The previous example moved the square around, but could you tell how fast it moved across
the screen? What if you wanted it to move exactly 20 pixels per second to the left
and 20 pixels per second to the right?

The `think()` method has an additional parameter we can use for this: `delta_ms`. It
represents the time which has passed since the last call to `think()`.

.. code-block:: python

    from st3m.reactor import Responder
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
            direction = ins.buttons.app # -1 (left), 1 (right), or 2 (pressed)

            if direction == ins.buttons.PRESSED_LEFT:
                self._x -= 20 * delta_ms / 1000
            elif direction == ins.buttons.PRESSED_RIGHT:
                self._x += 40 * delta_ms / 1000


    st3m.run.run_responder(Example())

This becomes important if you need exact timings in your application,
as the Reactor makes no explicit guarantee about how often `think()` will
be called. Currently we are shooting for once every 20 milliseconds, but if something in the system
takes a bit longer to process something, this number can change from one call to the next.


Example 1d: Automatic input processing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Working on the bare state of the buttons and the captouch petals can be cumbersome and error prone.
the flow3r application framework gives you a bit of help in the form of the :py:class:`InputController`
which processes an input state and gives you higher level information about what is happening.

The `InputController` contains multiple :py:class:`Pressable` sub-objects, for
example the app/OS buttons are available as following attributes on the
`InputController`:

+-----------------------------------+--------------------------+
| Attribute on ``InputControlller`` | Meaning                  |
+===================================+==========================+
| ``.buttons.app.left``             | App button, pushed left  |
+-----------------------------------+--------------------------+
| ``.buttons.app.middle``           | App button, pushed down  |
+-----------------------------------+--------------------------+
| ``.buttons.app.right``            | App button, pushed right |
+-----------------------------------+--------------------------+
| ``.buttons.os.left``              | OS button, pushed left   |
+-----------------------------------+--------------------------+
| ``.buttons.os.middle``            | OS button, pushed down   |
+-----------------------------------+--------------------------+
| ``.buttons.os.right``             | OS button, pushed right  |
+-----------------------------------+--------------------------+

And each `Pressable` in turn contains the following attributes, all of which are
valid within the context of a single `think()` call:

+----------------------------+--------------------------------------------------------------------+
| Attribute on ``Pressable`` | Meaning                                                            |
+============================+====================================================================+
| ``.pressed``               | Button has just started being pressed, ie. it's a Half Press down. |
+----------------------------+--------------------------------------------------------------------+
| ``.down``                  | Button is being held down.                                         |
+----------------------------+--------------------------------------------------------------------+
| ``.released``              | Button has just stopped being pressed, ie. it's a Half Press up.   |
+----------------------------+--------------------------------------------------------------------+
| ``.up``                    | Button is not being held down.                                     |
+----------------------------+--------------------------------------------------------------------+

The following example shows how to properly react to single button presses without having to
think about what happens if the user presses the button for a long time. It uses the `InputController`
to detect single button presses and switches between showing a circle (by drawing a 360 deg arc) and
a square.


.. code-block:: python

    from st3m.reactor import Responder
    from st3m.input import InputController
    from st3m.utils import tau

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

            if self.input.buttons.app.middle.pressed:
                self._draw_rectangle = not self._draw_rectangle

            if self.input.buttons.app.left.pressed:
                self._x -= 20 * delta_ms / 1000
            elif self.input.buttons.app.right.pressed:
                self._x += 40 * delta_ms / 1000


    st3m.run.run_responder(Example())


Managing multiple views
^^^^^^^^^^^^^^^^^^^^^^^

If you want to write a more advanced application you probably also want to display more than
one screen (or view as we call them).
With just the Responder class this can become a bit tricky as it never knows when it is visible and
when it is not. It also doesn't directly allow you to launch a new screen.

To help you with that you can use a :py:class:`View` instead. It can tell you when
it becomes visible, when it is about to become inactive (invisible) and you can
also use it to bring a new screen or widget into the foreground or remove it
again from the screen.

Example 2a: Managing two views
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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

            # No need to handle returning back to Example on button press - the
            # flow3r's ViewManager takes care of that automatically.


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
            self.input._ignore_pressed()

        def think(self, ins: InputState, delta_ms: int) -> None:
            self.input.think(ins, delta_ms) # let the input controller to its magic

            if self.input.buttons.app.middle.pressed:
                self._vm.push(SecondScreen())

    st3m.run.run_view(Example())

Try it using `mpremote`. The right shoulder button switches between the two views. To avoid that
the still pressed button immediately closes `SecondScreen` we make us of a special method of the
`InputController` which hides the pressed button from the view until it is released again.

Example 2b: Easier view management
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The above code is so universal that we provide a special view which takes care
of this boilerplate: :py:class:`BaseView`. It integrated a local
`InputController` on ``self.input`` and a copy of the :py:class:`ViewManager`
which caused the View to enter on ``self.vm``.

Here is our previous example rewritten to make use of `BaseView`:

.. code-block:: python

    from st3m.ui.view import BaseView
    import st3m.run

    class SecondScreen(BaseView):
        def __init__(self) -> None:
            # Remember to call super().__init__() if you implement your own
            # constructor!
            super().__init__()

        def on_enter(self, vm: Optional[ViewManager]) -> None:
            # Remember to call super().on_enter() if you implement your own
            # on_enter!
            super().on_enter(vm)

        def draw(self, ctx: Context) -> None:
            # Paint the background black
            ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
            # Green square
            ctx.rgb(0, 255, 0).rectangle(-20, -20, 40, 40).fill()

    class Example(BaseView):
        def draw(self, ctx: Context) -> None:
            # Paint the background black
            ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
            # Red square
            ctx.rgb(255, 0, 0).rectangle(-20, -20, 40, 40).fill()

        def think(self, ins: InputState, delta_ms: int) -> None:
            super().think(ins, delta_ms) # Let BaseView do its thing

            if self.input.buttons.app.middle.pressed:
                self.vm.push(SecondScreen())

    st3m.run.run_view(Example())



Writing an application for the menu system
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

All fine and good, you were able to write an application that you can run with `mpremote`,
but certainly you also want to run it from flow3r's menu system.

Let's introduce the final class you should actually be using for application development:
:py:class:`Application`. It builds upon `BaseView` (so you still have access to
`self.input` and `self.vm`) but additionally is made aware of an
:py:class:`ApplicationContext` on startup and can be registered into a menu.

Here is our previous code changed to use `Application` for the base of its main view:

.. code-block:: python

    from st3m.application import Application, ApplicationContext
    from st3m.ui.view import BaseView, ViewManager
    from st3m.input import InputState
    from ctx import Context
    import st3m.run

    class SecondScreen(BaseView):
        def draw(self, ctx: Context) -> None:
            # Paint the background black
            ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
            # Green square
            ctx.rgb(0, 255, 0).rectangle(-20, -20, 40, 40).fill()

    class MyDemo(Application):
        def __init__(self, app_ctx: ApplicationContext) -> None:
            super().__init__(app_ctx)
            # Ignore the app_ctx for now.

        def draw(self, ctx: Context) -> None:
            # Paint the background black
            ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
            # Red square
            ctx.rgb(255, 0, 0).rectangle(-20, -20, 40, 40).fill()

        def think(self, ins: InputState, delta_ms: int) -> None:
            super().think(ins, delta_ms) # Let Application do its thing

            if self.input.buttons.app.middle.pressed:
                self.vm.push(SecondScreen())

    if __name__ == '__main__':
        # Continue to make runnable via mpremote run.
        st3m.run.run_view(MyDemo(ApplicationContext()))

Using `Application` also gives you access to the `ApplicationContext`, which for example
gives you a way to find out the base path of your app, in `app_ctx.bundle_path`. For a
sample app using this, see `schneider's nyan-cat fork
<https://git.flow3r.garden/chubbson/nyan-cat/-/tree/schneider/bundle-path>`_.

To add the application to the menu we are missing one more thing: a `flow3r.toml`
file which describes the application so flow3r knows where to put it in the menu system.
Together with the Python code this file forms a so called bundle
(see also :py:class:`BundleMetadata`).

::

    [app]
    name = "My Demo"
    menu = "Apps"

    [entry]
    class = "MyDemo"

    [metadata]
    author = "You :)"
    license = "pick one, LGPL/MIT maybe?"
    url = "https://git.flow3r.garden/you/mydemo"


Save this as `flow3r.toml` together with the Python code as `__init__.py` in a
folder (name doesn't matter) and put that folder into one of the possible
application directories (see below) using `Disk Mode`_. Restart the flow3r and
it should pick up your new application.

+--------+----------------------+---------------------+---------------------------------------+
| Medium | Path in Disk Mode    | Path on Badge       | Notes                                 |
+========+======================+=====================+=======================================+
| Flash  | ``sys/apps``         | ``/flash/sys/apps`` | “Default” apps.                       |
+--------+----------------------+---------------------+---------------------------------------+
| Flash  | ``apps``             | ``/flash/apps``     | Doesn't exist by default. Split       |
|        |                      |                     | from ``sys`` to allow for cleaner     |
|        |                      |                     | updates.                              |
+--------+----------------------+---------------------+---------------------------------------+
| SD     | ``apps``             | ``/sd/apps``        | Doesn't exist by default. Will be     |
|        |                      |                     | retained even across badge reflashes. |
+--------+----------------------+---------------------+---------------------------------------+

Distributing applications
-------------------------

We have an "App Store" where you can submit your applications: https://flow3r.garden/apps/

To add your application, follow the guide in this repository: https://git.flow3r.garden/flow3r/flow3r-apps

Using the simulator
-------------------

The flow3r badge firmware repository comes with a Python-based simulator which
allows you to run the Python part of :ref:`st3m` on your local computer, using
Python, Pygame and wasmer.

Currently the simulator supports the display, LEDs, the buttons and some static
input values from the accelerometer, gyroscope, temperature sensor and pressure
sensor.

It does **not** support any audio API, and in fact currently doesn't even stub
out the relevant API methods, so it will crash when attempting to run any Music
app. It also does not support positional captouch APIs.

To set the simulator up, clone the repository and prepare a Python virtual
environment with the required packages:

::

    $ git clone https://git.flow3r.garden/flow3r/flow3r-firmware
    $ cd flow3r-firmware
    $ python3 -m venv venv
    $ venv/bin/pip install pygame wasmer wasmer-compiler-cranelift

.. warning::

    The wasmer python module from PyPI `doesn't work with Python versions 3.10 or 3.11
    <https://github.com/wasmerio/wasmer-python/issues/539>`_.  You will get
    ``ImportError: Wasmer is not available on this system`` when trying to run
    the simulator.

    Instead, install our `rebuilt wasmer wheels <https://flow3r.garden/tmp/wasmer-py311/>`_ using

    ::

        venv/bin/pip install https://flow3r.garden/tmp/wasmer-py311/wasmer_compiler_cranelift-1.2.0-cp311-cp311-manylinux_2_34_x86_64.whl
        venv/bin/pip install https://flow3r.garden/tmp/wasmer-py311/wasmer-1.2.0-cp311-cp311-manylinux_2_34_x86_64.whl

*TODO: set up a pyproject/poetry/... file?*

You can then run the simulator:

::

    $ venv/bin/python sim/run.py

Grey areas near the petals and buttons can be pressed.

The simulators apps live in `python_payload/apps` copy you app folder in there
and it will appear in the simulators menu system.

*TODO: make simulator directly run a bundle on startup when requested*
