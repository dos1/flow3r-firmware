"""
Badgelink API.

Badgelink allows for multiple flow3r badges to connect together over the TRRS
jacks and exchange data. It also allows for flow3r badges to access the outside
world over the same connector, even if that world is outside of the realm of
usual audio frequencies. Think: eurorack control voltages, MIDI, ...

Currently the API is very spartan, and only exposes Jack objects for the left
and right TRRS jack. These Jack objects in return have standard micropython
ESP32 Pin objects on which you can instantiate other micropython ESP32 objects,
like UART, PWM, etc.

We hope to have a high-level API that allows actually establishing connectivity
between two badges very soon :).

Sample usage:

>>> import badge_link
>>> badge_link.right
Jack(right)
>>> badge_link.right.active()
False
>>> badge_link.right.enable()
True
>>> badge_link.right.active()
True
>>> badge_link.right.tip
Pin(4)
>>> rt = badge_link.right.tip
>>> rt.init(mode=machine.Pin.OUT)
>>> rt.on()
"""

try:
    from typing import Protocol
except ImportError:
    from typing_extensions import Protocol

class Pin(Protocol):
    """
    TODO(q3k): properly define in machine.pyi
    """

    pass

class JackPin(Protocol):
    @property
    def tip(self) -> Pin:
        """
        Return the GPIO Pin object for this jack pin.

        This pin can be manipulated to perform I/O on the jack once it is in
        badgelink mode.
        """
        ...
    def enable(self) -> bool:
        """
        Enable badgelink on this jack, on both ring and tip connectors.

        Enabling badgelink means that the given jack will not be used to pass
        through audio (ie. line out / headphones out on the left jack, and line
        in on the right jack). Instead, that jack will be connected to the
        ESP32's pins, and normal ESP32 I/O operations can be performed on said
        pins.

        True is returned if badgelink was succesfully enabled, false otherwies.
        Badgelink will fail to initialize on the left jack if nothing is
        currently plugged in (as a safety measure).
        """
        ...
    def disable(self) -> None:
        """
        Disable badgelink on this jack, returning it into audio mode.
        """
        ...
    def active(self) -> bool:
        """
        Returns true if this pin is currently enabled for badgelink, false if
        it's in standard audio mode.
        """
        ...

class Jack(Protocol):
    def enable(self) -> bool:
        """
        Enable badgelink on this jack, on both ring and tip connectors.

        Enabling badgelink means that the given jack will not be used to pass
        through audio (ie. line out / headphones out on the left jack, and line
        in on the right jack). Instead, that jack will be connected to the
        ESP32's pins, and normal ESP32 I/O operations can be performed on said
        pins.

        True is returned if badgelink was succesfully enabled, false otherwies.
        Badgelink will fail to initialize on the left jack if nothing is
        currently plugged in (as a safety measure).

        This enables both pins (tip and ring) of this jack. They can also be
        individually enabled using the .pin.enable/.pin.disable methods.
        """
        ...
    def disable(self) -> None:
        """
        Disable badgelink on this jack, returning it into audio mode.

        This disables both pins (tip and ring) of this jack. They can also be
        individually disabled using the .pin.enable/.pin.disable methods.
        """
        ...
    def active(self) -> bool:
        """
        Returns true if this jack is currently fully enabled for badgelink,
        false if it's in standard audio mode.

        If only one of the pins is individually enabled, this method returns
        false.
        """
        ...
    @property
    def tip(self) -> JackPin:
        """
        Return the JackPin object for the tip of this jack.

        This pin can be manipulated to perform I/O on the jack once it is in
        badgelink mode. It can also be used to individdually enable/disable
        badge link on this pin only.
        """
        ...
    @property
    def ring(self) -> JackPin:
        """
        Return the JackPin object for the ring of this jack.

        This pin can be manipulated to perform I/O on the jack once it is in
        badgelink mode. It can also be used to individdually enable/disable
        badge link on this pin only.
        """
        ...

left: Jack
right: Jack
