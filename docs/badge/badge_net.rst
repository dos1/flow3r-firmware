.. _Badge Net:

Badge Net
=========

.. note::

   See also: :ref:`Micropython API docs<Badge Net API>`


Badge Net builds on top of :ref:`Badge Link` to provide Ethernet and IPv6
connectivity between badges, using 3.5mm jacks. Multiple badges can be connected
together and can all talk to eachother!

Each badge always runs a Badge Net interface that's visible as a standard
network interface to Micropython/ESP32.

When Badge Net connectivity is established, normal `sockets` calls can be used
to send and receive UDP packets, establish TCP connectivity, etc.

Protocol Stack
--------------

+-------------------------------------+---------------------+
| Layer                               | Info                |
+=====================================+=====================+
| **TCP/UDP**                         | You use this.       |
+-------------------------------------+---------------------+
| **IPv6**                            | You use this.       |
+-------------------------------------+---------------------+
| Ethernet                            | Badge Net!          |
+-------------------------------------+                     +
| SLIP (packetization)                |                     |
+-------------------------------------+                     +
| UART (115200 baud)                  |                     |
+-------------------------------------+---------------------+
| **Badge Link** (ESP32 digital pins) | You configure this. |
+-------------------------------------+---------------------+

Each badge runs an Ethernet switch (in software) which allows multiple badges to
be connected together.

Connecting Badges
-----------------

Currently, Badge Net needs to be manually configured for each jack. Because most
3.5mm audio cables are not-crossed-over, and because we use UART under the hood,
you will have to pay attention to the polarity of each configured jack.

Each jack has two pins: a tip and a ring. Each jack needs to be configured to
transmit on one and receive on another.

Each jack can be configured in one of four modes:

+-------------------+-----------------------------------------------------------+
| Mode              | Description                                               |
+===================+===========================================================+
| Disabled          | No Badge Net on this jack.                                |
+-------------------+-----------------------------------------------------------+
| Enabled, Auto     | Tip transmits on left jack, ring transmits on right jack. |
+-------------------+-----------------------------------------------------------+
| Enabled, Tip TX   | Tip transmits on this jack.                               |
+-------------------+-----------------------------------------------------------+
| Enabled, Ring  TX | Ring transmits on this jack.                              |
+-------------------+-----------------------------------------------------------+

The simplest option is to configure all participating jacks in 'Auto' mode, and
always connect the left jack of one badge to the right jack of another badge.

To actually establish connectivity, use :py:func:`badgenet.configure_jack` to
set a modde for a jack, then :py:mod:`badgelink` to enable Badge Link on the
same jack. **Configuring the jack with Badge Net is not enough, it must also be
enabled for Badge Link!**.

For example, on badge A:

.. code-block:: pycon

  >>> import badgenet
  >>> badgenet.configure_jack(badgenet.SIDE_LEFT, badgenet.MODE_ENABLE_AUTO)
  >>> import badgelink
  >>> badgelink.left.enable()

And on badge B:

.. code-block:: pycon

  >>> import badgenet
  >>> badgenet.configure_jack(badgenet.SIDE_RIGHT, badgenet.MODE_ENABLE_AUTO)
  >>> import badgelink
  >>> badgelink.right.enable()

And then connect badge A's left jack to badge B's right jack.

Sending Packets (broadcast)
---------------------------

The simplest option for communication is to use good old broadcast UDP.

Well, there's no real broadcast with IPv6 - but here we'll use a 'standard'
multicast group to get the equivalent functionality. Fell free to use different
`Multicast Groups
<https://en.wikipedia.org/wiki/IPv6_address#Multicast_addresses>`!

You'll also have to suffix the multicast group address with ``%bl1``, or
whatever the name of the badgelink interface is. This is an IPv6 scope, and it's
required when dealing with link-local addresses.

On badge A:

.. code-block:: pycon

  >>> addr = 'ff02::1%' + bagenet.get_interface().name()
  >>> import socket
  # IPv6, UDP
  >>> s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
  # Listen on multicast, UDP, port 1337
  >>> s.bind((addr, 1337))
  # Block until data was received
  >>> msg, addr = s.recvfrom(1024)

On badge B:

.. code-block:: pycon

  >>> addr = 'ff02::1%' + bagenet.get_interface().name()
  >>> import socket
  # IPv6, UDP
  >>> s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
  # Tell everyone something nice.
  >>> s.sendto('hi, good to see you!', (addr, 1337))

Sending Packets (unicast)
-------------------------

You can find out the link-local address of badge A, then listen on ``::`` and
connect or send data to this badge via unicast packets (not broadcasting the
data to everyone).

.. code-block:: pycon

  >>> badgenet.get_interface().ifconfig6()
  ['fe80::3685:18ff:fe90:888f']
  >>> s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
  >>> s.bind(('::', 1337))
  >>> msg, addr = s.recvfrom(1024)

Then, on another badge:

.. code-block:: pycon

  >>> addr = 'fe80::3685:18ff:fe90:888f%' + bagenet.get_interface().name()
  >>> import socket
  >>> s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
  >>> s.sendto('hi there, badge A', (addr, 1337))

Implementing badge discovery using multicasts is an exercise left to the reader. :)

In a similar fashion, TCP connections over unicast should just work.

Known Issues
------------

Badge Net is very experimental. Here are some known issues that we'd like to
address in future versions of the flow3r firmware:

 - No polarity autodetection.
 - No unified user settings.
 - No unified discovery mechanism.
 - Badge Net loops are undefined behaviour :)