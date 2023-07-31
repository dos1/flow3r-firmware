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


Using patches
-------------

In bl00mbox all sound sources live in a channel. This allows for easy 
application and patch management. Ideally the OS should provide each application
with a channel instance, but you can also spawn one directly:

.. code-block:: python

    import bl00mbox
    blm = bl00mbox.Channel()
    blm.volume = 5000

The easiest way to get sound is to use patches. These are "macroscopic" units
that directly connect to the output and provide a compact UI. Here's how to
create one:

.. code-block:: python

    # no enter, just tab for autocomplete to see patches
    bl00mbox.patches.
    # create a patch instance
    tiny = blm.new(bl00mbox.patches.tinysynth_fm)
    # play it!
    tiny.start()
    # try autocomplete here too!
    tiny.
    # patches come with very individual parameters!
    tiny.fm_waveform(tiny.SAW)
    tiny.start()

.. note::
    Tone! While your first instinct might be to use a .freq() property to tune
    an oscillator, there might be an easier way: .tone() interprets its input
    as semitones away from A440. In practice this means that by using .tone(int)
    you can easily get the usual equidistant 12-tone thing. An Db direclty below
    A440 for example can be expressed as .tone(-8), or shifted an octave higher
    as .tone(-8+12).

Using buds
----------

To create sound, we can load a bud (basically a radspa plugin with some wrapper
around) from a radspa plugin descriptor. For this you have to find the ID number
of the plugin descriptor first. bl00mbox provides a plugin registry for this:

.. code-block:: python

    # use autocomplete to see plugins
    bl00mbox.plugins
    # print details about specific plugin
    bl00mbox.plugins.ampliverter
    # create a new bud
    osc = blm.new(bl00mbox.plugins.trad_osc)
    env = blm.new(bl00mbox.plugins.trad_envelope)

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
    env.signals.sustain = 0
    env.signals.decay = 2000
    # assign the same signal with a /pitch helper function to 440Hz (broken atm)
    osc.signals.pitch.freq = 460

You can stream an output signal to an input signal by assigning a signal to its
value. Actually, it works the other way around too! Multiple inputs can stream
from the same output, however each input streams only from one output.

.. note::
    A special case is the channel mixer (an input signal) which only fakes
    being a bl00mbox signal and can accept multiple outputs.

.. code-block:: python

    # assign an output to an input...
    env.signals.input = osc.signals.output
    # ...or an input to an output!
    env.signals.output = blm.mixer

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
    osc1 = blm.new(bl00mbox.plugins.trad_osc)
    env1 = blm.new(bl00mbox.plugins.trad_envelope)
    env1.signals.output = blm.mixer
    env1.signals.input = osc1.signals.output

    osc2 = blm.new(bl00mbox.plugins.trad_osc)
    env2 = blm.new(bl00mbox.plugins.trad_envelope)
    env2.signals.input = osc2.signals.output

    env2.signals.output = osc1.signals.lin_fm

    env1.signals.sustain = 0
    env2.signals.sustain = 0
    env1.signals.attack = 10
    env2.signals.attack = 100
    env1.signals.decay = 800
    env2.signals.decay = 800

    osc1.signals.pitch.tone = -12
    osc2.signals.pitch.tone = -24

    osc3 = blm.new(bl00mbox.plugins.trad_osc)
    osc3.signals.waveform = 0
    osc3.signals.pitch.tone = -100
    osc3.signals.output = env1.signals.trigger
    osc3.signals.output = env2.signals.trigger

    osc4 = blm.new(bl00mbox.plugins.trad_osc)
    osc4.signals.waveform = 32767
    osc4.signals.pitch.tone = -124

    amp1 = blm.new(bl00mbox.plugins.ampliverter)
    amp1.signals.input = osc4.signals.output
    amp1.signals.bias = 18376 - 2400
    amp1.signals.gain = 300

    amp1.signals.output = osc1.signals.pitch

    amp2 = blm.new(bl00mbox.plugins.ampliverter)
    amp2.signals.input = amp1.signals.output
    amp2.signals.bias = - 2400
    amp2.signals.gain = 31000

    amp2.signals.output = osc2.signals.pitch
    osc2.signals.output = blm.mixer

