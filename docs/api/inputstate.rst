``input`` module
====================

InputState
----------

.. note::

    There is still old deprecated API in the InputState object and its recursive members.
    If it isn't listed here, your application might break in the future if you choose to
    use it. Note that class names/types may change in the future so that isinstance() is
    not considered part of the stable API. Existing class names may already deviate from
    the ones documented here. This is not considered a bug at this point in time.

InputState is the central source of all hardware inputs. In the future this will
be singleton-ified, but for now due to backwards compatibility reasons each view
spawns its own instance. It is possible to explore/play around with the object in
the REPL by creating it like this:

.. code-block:: pycon

    >>> from st3m import input
    # get an InputState object
    >>> ins = input.InputState()
    >>> import time
    >>> while(True):
    >>>     time.sleep_ms(300)
    # update to manually simulate the OS fetching fresh data
    >>>     ins.update(300)
    >>>     print(ins.meters_above_sea)

.. note::

    Don't actually create your own InputState in an application, use the one passed
    by think() and don't update it manually, let the OS do the job for you. We're providing
    the REPL alternative strictly for playing around/exploration, not for direct use in
    application code.

InputState is almost entirely is read-only, the little user malleable state it has
(i.e., only the setter for the MomentarySwitch timer) dissipates quickly. Persistent
config should be stored elsewhere. If a view wishes to implement higher-level signal
processing such as repeats it is responsible for keeping data and managing updates by
itself in a structure external to InputState.

.. py:class:: InputState

    .. py:property:: temperature
        :type: float

        Temperature in degree Celsius

    .. py:property:: pressure
        :type: float

        Pressure in Pascal

    .. py:property:: meters_above_sea
        :type: float

        Estimated elevation derived from ``.pressure``

    .. py:property:: battery_voltage
        :type: float/None

        Battery voltage in Volts, ``None`` if no battery is connected (this feature is still
        in the works, but do check for ``None`` when developing)

    .. py:property:: imu
        :type: IMUState

        Data from accelerometer/gyroscope

    .. py:property:: buttons
        :type: InputButtonState

        Data from the shoulder buttons

    .. py:property:: captouch
        :type: Captouch

        Data from the captouch surfaces

    .. py:method:: update(delta_t_ms) -> None
        
        /!\\ don't use in application code! Refreshes data for all inputs and
        recalculates edge events for all contained MomentarySwitch instances.
        ``delta_t_ms`` is the time passed since the last ``.update()`` call.

    .. py:method:: copy() -> InputState
    
        Returns a deep copy of the instance.

For button-like inputs such as captouch surfaces and the buttons the
MomentarySwitch class provides a unified API across InputState:

.. py:class:: MomentarySwitch

    .. py:property:: is_pressed
        :type: bool

        True if the button/surface is pressed, else False

    .. py:property:: press_event
        :type: bool

        True if the button/surface has just started being pressed, else False

    .. py:property:: release_event
        :type: bool

        True if the button/surface has just stopped being pressed, else False

    .. py:property:: pressed_since_ms
        :type: int/None

        If the button/surface is being pressed: time in millisecons since
        press_event, else: ``None``. Can be used as a setter but will only accept
        values while being pressed.

    .. py:method:: clear_press() -> None
        
        All APIs will pretend the button is not pressed and will report no events.
        This state persists until the button is actually not pressed anymore. Useful
        when switching context and no immediate reaction to the input that caused
        context switching is desired.

    .. py:method:: _update(int delta_t_ms, bool is_pressed) -> None

        Manually update state of the Pressable with the current de facto press
        state ``is_pressed`` and time passed since last update ``delta_t_ms``
        in milliseconds. press/release event edges are calculated and cleared per
        update. This means updating more often than reading might cause missed
        events.

    .. py:method:: copy() -> MomentarySwitch
    
        Returns a deep copy of the instance.

Example how to use the timer to implement a simple repeat:

.. code-block:: python

    def think(self, delta_ms, ins):
        ...
        a = ins.buttons.app.inward
        if a.press_event:
            do_thing()
        if a.is_pressed:
            if a.pressed_since_ms > 400:
                do_repeated_thing()
                # after first iteration: reduce delay to 150ms by not fully
                # resetting the timer. timer is fully reset at each press_event.
                a.pressed_since_ms -= 150

----------------

Shoulder Buttons
----------------

.. py:class:: InputButtonState

    .. py:property:: app_button_is_left
        :type: bool
        
        True if the user configured the application button to be on the
        left side.

    .. py:property:: app
        :type: TriSwitch

        TriSwitch object with data from the application button.

    .. py:property:: os
        :type: TriSwitch

        TriSwitch object with data from the os button. Note: This button is used
        by the operating system and should not be used by applications.

    .. py:method:: copy() -> InputButtonState
    
        Returns a deep copy of the instance.

.. py:class:: TriSwitch

    .. py:property:: left
        :type: MomentarySwitch

        MomentarySwitch object with data from the switch being pressed to the left.

    .. py:property:: right
        :type: MomentarySwitch

        MomentarySwitch object with data from the switch being pressed to the right.

    .. py:property:: middle
        :type: MomentarySwitch

        MomentarySwitch object with data from the switch being pressed down.

    .. py:property:: inward
        :type: MomentarySwitch

        If the switch is on the left side of the badge, mirror of ``.right``, else
        mirror of ``.left``. Both share the same timer/clear state.
    
    .. py:property:: outward
        :type: MomentarySwitch

        If the switch is on the left side of the badge, mirror of ``.left``, else
        mirror of ``.right``. Both share the same timer/clear state.

    .. py:method:: copy() -> TriSwitch
    
        Returns a deep copy of the instance.

.. note::

    About os and app button: The os button is reserved for operating system functions,
    i.e. exiting an application/view (middle) and controlling volume (left/right). An
    application may read from it, but it cannot override the operating system functionality
    at this point in time (future plans for this however do exist, they are just not fleshed
    out sufficiently.) The app button may be used by applications however they please, the
    operating system does not react to it.

    Which one is which depends on user preference; if you hold flow3r in one hand, it's easy
    to realize that one button can be activated by the holding hand with relative ease (right
    index finger on right button for example), the other one however is poorly placed for this.
    In order to account for different people preferring to hold the badge differently and
    desiring fast access to different functionality, the app and os buttons can be flipped in
    the settings. The additional ``.outward`` and ``.inward`` mirrors make it possible to
    also flip button directionality along with position depending on this user preference.
    Please account for it in your application design to keep flow3r accessible for everyone!

--------

Captouch
--------

.. py:class:: Captouch

    .. py:property:: petals
        :type: list(Petal)

        List of petals. Index 0 is the petal above the USB-C jack, then incrementing clockwise.
        The (even) top petals of the flow3r are labelled in silkscreen with roman numerals.
        Please note that "X" represents "0".

    .. py:property:: rotary_dial
        :type: CaptouchRotaryDial
    
        A rotary dial transformation for the captouch data. It stores a pointer to the petal
        data and calculates the transformation on demand.

    .. py:method:: copy() -> Captouch
    
        Returns a deep copy of the instance.

.. py:class:: Petal(MomentarySwitch)

    .. py:property:: is_top
        :type: bool

        Constant: True if the petal is on the top board, else False

    .. py:property:: at_angle
        :type: float
        
        Constant: The cw angle of the physical petal relative to the
        USB-C port in radians.

    .. py:property:: touch_area
        :type: float

        Estimate of how much area of the petal is being touched. Clamped
        between 0..1

    .. py:property:: touch_radius
        :type: float

        For a single touch per petal, reflects how far towards the outside
        of the petal the touch is happening. Clamped between 0..1, 0 is inside,
        1 is outside. Not properly implemented for petal 5 due to driver issues.
        (precision on the rough side at this point in time)

    .. py:property:: touch_angle_cw
        :type: float

        For a single touch per petal, reflects how far clockwise
        of the petal the touch is happening. Angle represented in radians
        from -tau/10..+tau/10. Can be summed with at_angle to get total
        touch angle. Only available for top petals, is 0 for bottom petals.
        (precision on the rough side at this point in time)

    .. py:property:: pads
        :type: PetalPads

        The individual pads of the petal are accessible in here as instances
        of PetalPad. For bottom petals, ``base`` and ``tip`` are available.
        For top petals, ``base``, ``cw`` and ``ccw`` are available.

    .. py:method:: copy() -> Petal
    
        Returns a deep copy of the instance.

.. py:class:: PetalPads

    .. py:property:: base
        :type: PetalPad

        Always available: The pad closest to the display. Returns garbage
        on petal 5 for driver reasons.

    .. py:property:: tip
        :type: PetalPad
        
        Only available for bottom petals: The pad furthest from the display.

    .. py:property:: cw
        :type: PetalPad
        
        Only available for top petals: The clockwise pad.

    .. py:property:: ccw
        :type: PetalPad
        
        Only available for top petals: The counterclockwise pad.

    .. py:method:: copy() -> PetalPads
    
        Returns a deep copy of the instance.

.. py:class:: PetalPad(MomentarySwitch)

    .. py:property:: touch_area
        :type: float

        Estimate of how much area of the pad is being touched. Clamped
        between 0..1

.. py:class:: CaptouchRotaryDial

    .. py:property:: touch_angle_cw
        :type: float/None

        If the data implies single touch on the entire top petal surface: Angle in
        radians at which the touch is estimated. Else ``None``.

------------------------

Inertial Measurement Unit
-------------------------

.. py:class:: IMUState

    .. py:property:: acc
        :type: Accelerometer

        (x, y, z) tuple of acceleration in m/s^2, including gravity,
        plus some bonus helper properties.

    .. py:property:: gyro
        :type: tuple

        (x, y, z) tuple of rotational speed in deg/s

    .. py:method:: copy() -> IMUState
    
        Returns a deep copy of the instance.

.. py:class:: Accelerometer(tuple)

    .. py:property:: radius
        :type: float

        Length of the acceleration vector in m/s^2, including gravity (expect 1.625m/s^2
        when flow3r is not in motion and on the moon (earth: 9.821m/s^2))

    .. py:property:: inclination
        :type: float

        How far the flow3r is tilted, in radians. 0 when flow3r is lying face-up,
        tau/4 when standing up, tau/2 when lying face down. Range: 0..tau/2

    .. py:property:: azimuth
        :type: float

        If flow3r is not lying flat: Angle between direction of gravity projected on
        the flow3r surface and petal 5. 0 if the USB-C jack is pointing up, increases
        when rotating flow3r clockwise. Range: -tau/2..tau/2

The sensing axes of the IMU are documented in
https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi270-ds000.pdf, page 144.
The x axis points towards the USB-C jack, the y axis points to the left side and the x axis to
the top surface of flow3r.

---------------

Old API Migration guide
-----------------------

.. note::

    In order to move InputState to the C backend the old API will be deprecated in the
    2.0 release. If you are using the old API in your applications please migrate soon.

The old API consisted of two seperate input objects, one passed to the ``Application.think``
method as the ``ins`` argument, one updated in the background as ``Application.input``.
Both used similar terms slightly inconsistently as shown below, be careful to pick the right one!

``ins`` simply contained an integer for each button that was then to be compared with a constant.
This can be directly migrated to MomentarySwitch. Captouch petals used a ``.pressed`` attribute,
however since the collision with the Pressable term this was deprecated and is replaced by ``.is_pressed``.
``.position`` and ``.pressure`` have been replaced by the new ``.touch_*``
methods with the linear conversions:

.. code-block:: python

    .position[0] = .touch_radius * 80000 - 35000
    .position[1] = .touch_angle * 35000 / (math.tau/10)
    .pressure = .touch_area * 35000

The ``ins`` instance passed to your Application.think method will be turned into a singleton
in the future. Right now the backend creates a new copy each round to pass to the application so
that the application can use a reference to this object to keep a log,  but in the future only one
InputState object will be kept by the backend and all its references will be updated as well. If
you want to log data, use the ``.copy()`` method provided by many objects in InputState to store
exactly what you need.

``Application.input`` will be deprecated entirely. Its functionality is now included in ``ins``.
It is primarily populated by Pressable objects whose functionality can be replicated with the
new MomentarySwitch objects as follows:

.. py:class:: Pressable

    .. py:property:: pressed -> bool

        Equivalent to ``MomentarySwitch.press_event``

    .. py:property:: released -> bool

        Equivalent to ``MomentarySwitch.release_event``

    .. py:property:: down -> bool

        Equivalent to ``.MomentarySwitch.pressed and not MomentarySwitch.press_event``

    .. py:property:: up -> bool
    
        Equivalent to not ``MomentarySwitch.pressed and not MomentarySwitch.release_event``

    .. py:property:: repeated -> bool

        No direct equivalence, use the example on the bottom of the MomentarySwitch section

