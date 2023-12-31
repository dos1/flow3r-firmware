.. _Fonts:

Fonts
=====

The current selection of fonts is baked into the firmware for use with :ref:`Context<ctx API>`.

Available Fonts
---------------

Following fonts are currently available (previews in size 20):

.. |font0| image:: assets/0.png
.. |font1| image:: assets/1.png
.. |font2| image:: assets/2.png
.. |font3| image:: assets/3.png
.. |font4| image:: assets/4.png
.. |font5| image:: assets/5.png
.. |font6| image:: assets/6.png
.. |font8| image:: assets/8.png

+-------------+----------------------+---------+
| Font Number | Font Name            | Preview |
+=============+======================+=========+
| 0           | Arimo Regular        | |font0| |
+-------------+----------------------+---------+
| 1           | Arimo Bold           | |font1| |
+-------------+----------------------+---------+
| 2           | Arimo Italic         | |font2| |
+-------------+----------------------+---------+
| 3           | Arimo Bold Italic    | |font3| |
+-------------+----------------------+---------+
| 4           | Camp Font 1          | |font4| |
+-------------+----------------------+---------+
| 5           | Camp Font 2          | |font5| |
+-------------+----------------------+---------+
| 6           | Camp Font 3          | |font6| |
+-------------+----------------------+---------+
| 7           | Material Icons       |         |
+-------------+----------------------+---------+
| 8           | Comic Mono           | |font8| |
+-------------+----------------------+---------+

The Camp Fonts are based on Beon Regular, Saira Stencil One and Questrial Regular.

Material Icons contains Glyphs in the range of U+E000 - U+F23B.

See header files in ``components/ctx/fonts/`` for details.

Basic Usage
-----------
To switch fonts, simply set ``ctx.font`` and refer to the font by full name:

.. code-block:: python

    ctx.rgb(255, 255, 255)
    ctx.move_to(0, 0)
    ctx.font = "Camp Font 1"
    ctx.text("flow3r")

To insert one or more icons, use Python ``\u`` escape sequences.
You can look up the code points for icons on `https://fonts.google.com/icons <https://fonts.google.com/icons>`_.

.. code-block:: python

    ctx.save()
    ctx.rgb(255, 255, 255)
    ctx.move_to(0, 0)
    ctx.font = "Material Icons"
    ctx.text("\ue147 \ue1c2 \ue24e")
    ctx.restore()

Adding New Fonts
----------------
To add a new font to the firmware, it must first be converted into the ctx binary format.

ctx provides the tools for this and you can set them up them with the folling commands:

.. code-block:: bash

    git clone https://ctx.graphics/.git/
    cd ctx.graphics
    ./configure.sh
    make tools/ctx-fontgen

Now you can use ``ctx-fontgen`` to convert a font:

.. code-block:: bash

    ./tools/ctx-fontgen /path/to/ComicMono.ttf Comic_Mono latin1 > Comic-Mono.h

Note that the font name is read from the source file directly and not specified by any of the arguments.
You can find it at the end of the header file. In this case: ``#define ctx_font_Comic_Mono_name "Comic Mono"``.

The next step is to copy the header file over into ``components/ctx/fonts`` (and adding license headers, etc).

Once the file is in the fonts directory, it can be added to ``components/ctx/ctx_config.h``.
In there it needs an include and define with a new font number.

The new font is now available in the next firmware build, but the simulator needs a rebuild of the Wasm bundle.
See ``/sim/README.md`` for details.
