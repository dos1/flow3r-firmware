.. _bl00mbox:

bl00mbox
==========

bl00mbox is a modular audio engine designed for the flow3r badge. It it
suitable for live coding and is best explored in a REPL for the time being.

Upcoming features
-----------------

(in no specific order)

1) Expose hardware such as captouch and IMU as pseudo-signals that buds can subscribe to. This frees the repl for parameter manipulation while the backend takes care of the playing surface, neat for live coding.

2) Cross-channel connections

3) Stepped value naming

4) Better signal/bud representation in patches

Patches
-------------

In bl00mbox all sound sources live in a channel. This allows for easy 
application and patch management. Ideally the OS should provide each application
with a channel instance, but you can also spawn one directly:

.. code-block:: pycon

    >>> import bl00mbox
    # get a channel
    >>> blm = bl00mbox.Channel()
    # set channel volume
    >>> blm.volume = 5000

The easiest way to get sound is to use patches. These are "macroscopic" units
that directly connect to the output and provide a compact UI. Here's how to
create one:

.. code-block:: pycon

    # no enter here, press tab instead for autocomplete to see patches
    >>> bl00mbox.patches.
    # create a patch instance
    >>> tiny = blm.new(bl00mbox.patches.tinysynth_fm)
    # play it!
    >>> tiny.start()
    # try autocomplete here too!
    >>> tiny.
    # patches come with very individual parameters!
    >>> tiny.fm_waveform(tiny.SAW)
    >>> tiny.start()

Buds
----------

We can inspect the patch we created earlier:

.. code-block:: pycon

    >>> tiny
    [patch] tinysynth_fm
      [bud 32] osc_fm
        output [output]: 0 => input in [bud 34] ampliverter
        pitch [input/pitch]: 18367 / 0.0 semitones / 440.0Hz
        waveform [input]: -1
        lin_fm [input]: 0 <= output in [bud 35] osc_fm
      [bud 33] env_adsr
        output [output]: 0 => gain in [bud 34] ampliverter
        phase [output]: 0
        input [input]: 32767
        trigger [input/trigger]: 0
        attack [ms] [input]: 20
        decay [ms] [input]: 1000
        sustain [ms] [input]: 0
        release [input]: 100
        gate [input]: 0
      [bud 34] ampliverter
        output [output]: 0 ==> [channel mixer]
        input [input]: 0 <= output in [bud 32] osc_fm
        gain [input]: 0 <= output in [bud 33] env_adsr
        bias [input]: 0
      [bud 35] osc_fm
        output [output]: 0 => lin_fm in [bud 32] osc_fm
        pitch [input/pitch]: 21539 / 15.86 semitones / 1099.801Hz
        waveform [input]: 1
        lin_fm [input]: 0


The patch is actually composed of buds! Buds are wrappers that contain atomic plugins. Each
plugin is composed of signals that can be connected to other signals. Signals can have different
properties that are listed behind their name in square brackets. For starters, each signal is
either an input or output. Connections always happen between an input and an output. Outputs
can fan out to multiple inputs, but inputs can only receive data from a single output. If no
output is connected to an input, it has a static value.

.. note::
    A special case is the channel mixer (an [input] signal) which only fakes
    being a bl00mbox signal and can accept multiple outputs.

Let's play around with that a bit more and create some fresh unbothered buds:

.. code-block:: pycon

    # use autocomplete to see plugins
    >>> bl00mbox.plugins.
    # print details about specific plugin
    >>> bl00mbox.plugins.ampliverter
    # create a new bud
    >>> osc = blm.new(bl00mbox.plugins.osc_fm)
    >>> env = blm.new(bl00mbox.plugins.env_adsr)

You can inspect properties of the new buds just as with a patch - in fact, many patches simply print
all their contained buds and maybe some extra info (but that doesn't have to be the case and is up
to the patch designer).

.. note::
    As of now patch designers can hide buds within the internal structure however they like and
    you kind of have to know where to find stuff. We'll come up with a better solution soon!

.. code-block:: pycon

    # print general info about bud
    >>> osc
    [bud 36] osc_fm
      output [output]: 0
      pitch [input/pitch]: 18367 / 0.0 semitones / 440.0Hz
      waveform [input]: -16000
      lin_fm [input]: 0

    # print info about a specific bud signal
    >>> env.signals.trigger
    trigger [input/trigger]: 0

We can connect signals by using the "=" operator. The channel provides its own [input] signal for routing
audio to the audio outputs. Let's connect the oscillator to it:

.. code-block:: pycon

    # assign an output to an input...
    >>> env.signals.input = osc.signals.output
    # ...or an input to an output!
    >>> env.signals.output = blm.mixer

Earlier we saw that env.signals.trigger is of type [input/trigger]. The [trigger] type comes with a special
function to start an event:

.. code-block:: pycon

    # you should hear something when calling this!
    >>> env.signals.trigger.start()

If a signal is an input you can directly assign a value to it. Some signal types come with special setter
functions, for example [pitch] types support multiple abstract input concepts:

.. code-block:: pycon

    # assign raw value to an input signal
    >>> env.signals.sustain = 16000
    # assign a abstract value to a [pitch] with signal type specific setters
    >>> osc.signals.pitch.freq = 220
    >>> osc.signals.pitch.tone = "Gb4"

Raw signal values range generally from -32767..32767. Since sustain is nonzero now, the tone doesn't
automatically stop after calling .start()

.. code-block:: pycon

    # plays forever...
    >>> env.signals.trigger.start()
    # ...until you call this!
    >>> env.signals.trigger.stop()

Channels
--------

As mentioned earlier all plugins live inside of a channel. It is up to bl00mbox to decide
which channels to skip and which ones to render. In this instance bl00mbox has 32 channels,
and we can get them individually:

.. code-block:: pycon

    # returns specific channel
    >>> chan_one = bl00mbox.Channel(1)
    >>> chan_one
    [channel 1] (foreground)
      volume: 3000
      buds: 18
      [channel mixer] (1 connections)
        output in [bud 1] lowpass

As we can see this channel has quite a lot going on. Ideally each application should have
its own channel(s), so in order to get a free one we'll create a new one without argument:

.. code-block:: pycon

    # returns free or garbage channel
    >>> chan_free = bl00mbox.Channel()
    >>> chan_free
    [channel 3] (foreground)
      volume: 3000
      buds: 0
      [channel mixer] (0 connections)

In case there's no free channel yet you get channel 31, the garbage channel. It behaves like
any other channel has a high chance to be cleared by other applications, more on that later.

Channels accept volume from 0-32767. This can be used to mix different sounds together, however
there also is an auto-foregrounding that we need to be aware of before doing that. When we requested
a free channel, bl00mbox automatically moved it to foreground. Let's look at channel 1 again:

.. code-block:: pycon

    >>> chan_one
    [channel 1]
    ...

Note that the (foreground) marker has disappeared. This means no audio from channel 1 is rendered at
the moment, but it is still in memory and ready to be used at any time. We have several methods of
doing so:

.. code-block:: pycon

    # mark channel as foregrounded manually
    >>> chan_one.foreground = True
    >>> chan_one
    [channel 1] (foreground)
    ...
    >>> chan_free
    [channel 3]
    ...
    # override the background mute for a channel;
    # chan_free is always rendered now
    >>> chan_free.background_mute_override = True
    >>> chan_one
    [channel 1] (foreground)
    ...
    >>> chan_free
    [channel 3]
    # interact with channel to automatically pull it
    # into foreground
    >>> chan_free.new(bl00mbox.plugins.osc_fm)
    >>> chan_one
    [channel 1]
    ...
    >>> chan_free (foreground)
    [channel 3]

What constitutes a channel interaction for auto channel foregrounding is a bit in motion at this point
and generally unreliable. For applications it is ideal to mark the channel manually when using it. When
exiting, an application should free the channel with automatically clears all buds. A channel should
be no longer used after freeing:

.. code-block:: pycon

    # this clears all buds and sets the internal "free" marker to zero
    >>> chan_one.free = True
    # good practice to not accidentially use a free channel
    >>> chan_one = None 

Some other misc channel operations for live coding mostly:

.. code-block:: pycon
    
    # drop all plugins
    >>> chan_free.clear()
    # show all non-free channels
    >>> bl00mbox.Channels.print_overview()
    [channel 3] (foreground)
      volume: 3000
      buds: 0
      [channel mixer] (0 connections)

Example 1: Auto bassline
------------------------

.. code-block:: pycon

    >>> import bl00mbox

    >>> blm = bl00mbox.Channel()
    >>> blm.volume = 10000
    >>> osc1 = blm.new(bl00mbox.plugins.osc_fm)
    >>> env1 = blm.new(bl00mbox.plugins.env_adsr)
    >>> env1.signals.output = blm.mixer
    >>> env1.signals.input = osc1.signals.output

    >>> osc2 = blm.new(bl00mbox.plugins.osc_fm)
    >>> env2 = blm.new(bl00mbox.plugins.env_adsr)
    >>> env2.signals.input = osc2.signals.output

    >>> env2.signals.output = osc1.signals.lin_fm

    >>> env1.signals.sustain = 0
    >>> env2.signals.sustain = 0
    >>> env1.signals.attack = 10
    >>> env2.signals.attack = 100
    >>> env1.signals.decay = 800
    >>> env2.signals.decay = 800

    >>> osc1.signals.pitch.tone = -12
    >>> osc2.signals.pitch.tone = -24

    >>> osc3 = blm.new(bl00mbox.plugins.osc_fm)
    >>> osc3.signals.waveform = 0
    >>> osc3.signals.pitch.tone = -100
    >>> osc3.signals.output = env1.signals.trigger
    >>> osc3.signals.output = env2.signals.trigger

    >>> osc4 = blm.new(bl00mbox.plugins.osc_fm)
    >>> osc4.signals.waveform = 32767
    >>> osc4.signals.pitch.tone = -124

    >>> amp1 = blm.new(bl00mbox.plugins.ampliverter)
    >>> amp1.signals.input = osc4.signals.output
    >>> amp1.signals.bias = 18376 - 2400
    >>> amp1.signals.gain = 300

    >>> amp1.signals.output = osc1.signals.pitch

    >>> amp2 = blm.new(bl00mbox.plugins.ampliverter)
    >>> amp2.signals.input = amp1.signals.output
    >>> amp2.signals.bias = - 2400
    >>> amp2.signals.gain = 31000

    >>> amp2.signals.output = osc2.signals.pitch
    >>> osc2.signals.output = blm.mixer

