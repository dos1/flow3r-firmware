"""
Badge Net is a layer on top of Badge Link which provides Ethernet and IPv6
connectivity between badges.

A Badge Net interface is always present in the system and has an IPv6 link local
address assigned to it. To communicate with other badges, you will have to first
enable Badge Link on either of the two 3.5mm jacks (see :py:mod:`badgelink`) and
then configure the jacks for Badge Net as well.

Multiple badges can be connected together to build a single Ethernet / Badge Net
network. Just daisy chain them together!
"""
from typing import List, Tuple, Literal, Protocol

class BadgenetInterface(Protocol):
    """
    The sytem Badge Net interface. Alwasy exists, and this object is a thin
    wrapper around the singleton object in-memory.

    Get this by calling get_interface.
    """

    def ifconfig6() -> List[str]:
        """
        Returns list of IPv6 addresses. Currently these are always link-local
        (fe80:...) addresses.

        To connect to another badge's link-local address, remember to suffix the
        address with '%name' where 'name' is the result of the name function
        call. See main Badge Net docs for more information.
        """
        ...
    def name() -> str:
        """
        Returns interface name. Usually bl1. Used to suffix link-local addresses
        (in the form of an IPv6 zone/scope) when connecting to other badges.
        """
        ...

def get_interface() -> BadgenetInterface:
    """
    Returns the Badge Net interface.
    """
    ...

def get_switch_table() -> List[Tuple[str, str, int]]:
    """
    Returns dump of the MAC table fo the internal switch within the Badge Net
    interface. The switch is used to allow multiple badges to be connected
    together into a single Ethernet / Badge Net network.

    The returned value is a list of (mac address, destination port name,
    microseconds left until expired) tuples.
    """
    ...

class _Side(Protocol): ...
class _Mode(Protocol): ...

#: Left Jack.
SIDE_LEFT: _Side
#: Right Jack.
SIDE_RIGHT: _Side

#: Do not use Badge Net on this jack.
MODE_DISABLE: _Mode
#: Use the jack in 'auto' mode. Auto mode configures the left jack to transmit on its tip, and the right jack on its ring. This allows badges to be connected together with non-crossed 3.5mm cables, but only left-jack to right-jack. For left-to-left or left-to-right connectivity, non-AUTO modes must be used.
MODE_ENABLE_AUTO: _Mode
#: Use the jack, transmit on the jack tip. The other badge must be configured in TX_RING mode.
MODE_ENABLE_TX_TIP: _Mode
#: Use the jack, transmit on the jack ring. The other badge must be configured in TX_TIP mode.
MODE_ENABLE_TX_RING: _Mode

def configure_jack(side: _Side, mode: _Mode) -> None:
    """
    Configures Badge Net on a jack.

    The jack must also be set up using Badge Link! See Badge Link modes for more
    information.

    This can be called multiple times, and is a no-op if the jack is already
    configured.
    """
    ...
