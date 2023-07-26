.. _bl00mbox:

bl00mbox
==========

bl00mbox is a modular audio engine designed for the flow3r badge. It it
suitable for live coding and is best explored in a REPL for the time being.

bl00mbox plugins are supplied in the radspa format.

.. note::
    This is a work in progress, things are somewhat broken.

Upcoming features
-----------------

(in no specific order)

1) Expose hardware such as captouch and IMU as pseudo-signals that buds can subscribe to. This frees the repl for parameter manipulation while the backend takes care of the playing surface. Medium urgency.

2) Be able to delete buds/connections. High urgency.

3) Sequencer. Low urgency.

4) Proper channel management (we're faking it rn, all is on Channel 0). Medium urgency.


Basic usage
-------------

In bl00mbox all sound sources live in a channel. This allows for easy 
application and patch management. Ideally the OS should provide each application
with a channel instance, but you can also spawn one directly:

.. code-block:: python

    import bl00mbox
    blm = bl00mbox.Channel()

To create sound, we can load a bud (basically a radspa plugin with some wrapper
around) from a radspa plugin descriptor. For this you have to find the ID number
of the plugin descriptor first. bl00mbox provides a plugin registry for this:

.. code-block:: python

    # print all plugins
    bl00mbox.PluginRegistry.print_plugin_list()
    # print details about specific plugin from its ID
    bl00mbox.PluginRegistry.print_plugin_info(420)
    # create a new bud from a plugin ID
    osc = blm.new_bud(420)
    env = blm.new_bud(42)

You can inspect properties of the new bud. Note how some signals are
inputs and others are outputs and that there are subtypes such as /trigger:

.. code-block:: python

    # print general info about bud
    osc
    # print info about a specific bud signal
    env.signals.trigger

If a signal is an input, you can assign a value to it. Some signal
types come with additional helper functions to make your life easier:

.. code-block:: python

    # assign raw value to an input signal
    env.signals.sustain.value = 0
    env.signals.decay.value = 2000
    # assign the same signal with a /pitch helper function to 440Hz (broken atm)
    osc.signals.pitch.freq = 460

You can stream an output signal to an input signal by assigning a signal to its
value. Actually, it works the other way around too! Multiple inputs can stream
from the same output, however each input streams only from one output.

.. note::
    A special case is the channel output mixer (an input signal) which only fakes
    being a bl00mbox signal and can accept multiple outputs.

.. code-block:: python

    # assign an output to an input...
    env.signals.input.value = osc.signals.output
    # ...or an input to an output!
    env.signals.output.value = blm.output

The trigger of the envelope has a special helper function that triggers a volume
response:

.. code-block:: python

    # set channel volume
    blm.volume = 2000
    # you should hear something when calling this
    env.signals.trigger.start()

Example 1: auto bassline
------------------------

.. code-block:: python

    import bl00mbox

    blm = bl00mbox.Channel()
    blm.volume = 10000
    osc1 = blm.new_bud(420)
    env1 = blm.new_bud(42)
    env1.signals.output.value = c.output
    env1.signals.input.value = osc1.signals.output

    osc2 = blm.new_bud(420)
    env2 = c.new_bud(42)
    env2.signals.input.value = osc2.signals.output

    amp1 = blm.new_bud(69)
    amp1.signals.input.value = env2.signals.output
    amp1.signals.output.value = osc1.signals.lin_fm

    env1.signals.sustain.value = 0
    env2.signals.sustain.value = 0
    env1.signals.attack.value = 10
    env2.signals.attack.value = 100
    env1.signals.decay.value = 800
    env2.signals.decay.value = 800

    osc1.signals.pitch.tone = -12
    osc2.signals.pitch.tone = -24

    osc3 = blm.new_bud(420)
    osc3.signals.waveform.value = 0
    osc3.signals.pitch.tone = -100
    osc3.signals.output.value = env1.signals.trigger
    osc3.signals.output.value = env2.signals.trigger

    osc4 = blm.new_bud(420)
    osc4.signals.waveform.value = 32767
    osc4.signals.pitch.tone = -124

    amp2 = blm.new_bud(69)
    amp2.signals.input.value = osc4.signals.output
    amp2.signals.bias.value = 18376 - 2400
    amp2.signals.gain.value = 300

    amp2.signals.output.value = osc1.signals.pitch

    amp3 = blm.new_bud(69)
    amp3.signals.input.value = amp2.signals.output
    amp3.signals.bias.value = - 2400
    amp3.signals.gain.value = 31000

    amp3.signals.output.value = osc2.signals.pitch
    osc2.signals.output.value = c.output

