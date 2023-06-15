.. py:module:: ctx

``ctx`` module
==============

.. py:class:: Context

   .. py:method:: line_to
   .. py:method:: move_to
   .. py:method:: curve_to
   .. py:method:: quad_to
   .. py:method:: rel_line_to
   .. py:method:: rel_move_to
   .. py:method:: rel_curve_to
   .. py:method:: rel_quad_to
   .. py:method:: rectangle
   .. py:method:: arc
   .. py:method:: arc_to
   .. py:method:: rel_arc_to
   .. py:method:: round_rectangle
   .. py:method:: begin_path
   .. py:method:: close_path
   .. py:method:: in_fill
   .. py:method:: fill
   .. py:method:: stroke
   .. py:method:: paint
   .. py:method:: clip
   .. py:method:: text
   .. py:method:: text_width
   .. py:method:: rotate
   .. py:method:: scale
   .. py:method:: translate
   .. py:method:: apply_transform
   .. py:method:: start_group
   .. py:method:: end_group
   .. py:method:: preserve
   .. py:method:: linear_gradient
   .. py:method:: radial_gradient
   .. py:method:: add_stop
   .. py:method:: line_dash
   .. py:method:: texture
   .. py:method:: save
   .. py:method:: restore
   .. py:method:: start_frame
   .. py:method:: end_frame
   .. py:method:: get_font_name

   .. py:method:: gray
   .. py:method:: rgb
   .. py:method:: rgba

   .. py:method:: tinyvg_draw
   .. py:method:: tinyvg_get_size

   .. py:method:: logo

   Attributes
   ----------

   .. py:attribute:: x
   .. py:attribute:: y
   .. py:attribute:: width
   .. py:attribute:: height
   .. py:attribute:: font
   .. py:attribute:: image_smoothing
   .. py:attribute:: compositing_mode
   .. py:attribute:: blend_mode
   .. py:attribute:: flags
   .. py:attribute:: line_cap
   .. py:attribute:: line_join
   .. py:attribute:: text_align
   .. py:attribute:: fill_rule
   .. py:attribute:: text_baseline
   .. py:attribute:: line_width
   .. py:attribute:: line_dash_offset
   .. py:attribute:: line_height
   .. py:attribute:: wrap_left
   .. py:attribute:: wrap_right
   .. py:attribute:: miter_limit
   .. py:attribute:: global_alpha
   .. py:attribute:: font_size

   Flag constants
   --------------

   .. py:data:: LOWFI
   .. py:data:: GRAY2
   .. py:data:: GRAY4
   .. py:data:: GRAY8
   .. py:data:: RGB332
   .. py:data:: HASH_CACHE
   .. py:data:: KEEP_DATA
   .. py:data:: INTRA_UPDATE
   .. py:data:: STAY_LOW

   Fill rule constants
   -------------------

   .. py:data:: WINDING
   .. py:data:: EVEN_ODD

   Join constants
   --------------

   .. py:data:: BEVEL
   .. py:data:: ROUND
   .. py:data:: MITER

   Cap constants
   -------------

   .. py:data:: NONE
   .. py:data:: ROUND
   .. py:data:: SQUARE

   Composite constants
   -------------------

   .. py:data:: SOURCE_OVER
   .. py:data:: COPY
   .. py:data:: SOURCE_IN
   .. py:data:: SOURCE_OUT
   .. py:data:: SOURCE_ATOP
   .. py:data:: CLEAR
   .. py:data:: DESTINATION_OVER
   .. py:data:: DESTINATION
   .. py:data:: DESTINATION_IN
   .. py:data:: DESTINATION_OUT
   .. py:data:: DESTINATION_ATOP
   .. py:data:: XOR

   Blend constants
   ---------------

   .. py:data:: NORMAL
   .. py:data:: MULTIPLY
   .. py:data:: SCREEN
   .. py:data:: OVERLAY
   .. py:data:: DARKEN
   .. py:data:: LIGHTEN
   .. py:data:: COLOR_DODGE
   .. py:data:: COLOR_BURN
   .. py:data:: HARD_LIGHT
   .. py:data:: SOFT_LIGHT
   .. py:data:: DIFFERENCE
   .. py:data:: EXCLUSION
   .. py:data:: HUE
   .. py:data:: SATURATION
   .. py:data:: COLOR
   .. py:data:: LUMINOSITY
   .. py:data:: DIVIDE
   .. py:data:: ADDITION
   .. py:data:: SUBTRACT

   Text baseline constants
   -----------------------

   .. py:data:: ALPHABETIC
   .. py:data:: TOP
   .. py:data:: HANGING
   .. py:data:: MIDDLE
   .. py:data:: IDEOGRAPHIC
   .. py:data:: BOTTOM

   Text alignment constants
   ------------------------

   .. py:data:: START
   .. py:data:: END
   .. py:data:: CENTER
   .. py:data:: LEFT
   .. py:data:: RIGHT

   Format constants
   ----------------

   .. py:data:: GRAY8
   .. py:data:: GRAYA8
   .. py:data:: RGB8
   .. py:data:: RGBA8
   .. py:data:: BGRA8
   .. py:data:: RGB565
   .. py:data:: RGB565_BYTESWAPPED
   .. py:data:: RGB332
   .. py:data:: GRAY1
   .. py:data:: GRAY2
   .. py:data:: GRAY4
   .. py:data:: YUV420

   Other constants
   ---------------

   .. py:data:: PRESS
   .. py:data:: MOTION
   .. py:data:: RELEASE
   .. py:data:: ENTER
   .. py:data:: LEAVE
   .. py:data:: TAP
   .. py:data:: TAP_AND_HOLD
   .. py:data:: DRAG_PRESS
   .. py:data:: DRAG_MOTION
   .. py:data:: DRAG_RELEASE
   .. py:data:: KEY_PRESS
   .. py:data:: KEY_DOWN
   .. py:data:: KEY_UP
   .. py:data:: SCROLL
   .. py:data:: MESSAGE
   .. py:data:: DROP
   .. py:data:: SET_CURSOR

Formats
-------

.. py:data::
.. py:data:: GRAY8
.. py:data:: GRAYA8
.. py:data:: RGB8
.. py:data:: RGBA8
.. py:data:: BGRA8
.. py:data:: RGB565
.. py:data:: RGB565_BYTESWAPPED
.. py:data:: RGB332
.. py:data:: GRAY1
.. py:data:: GRAY2
.. py:data:: GRAY4
.. py:data:: YUV420

Flags
-----

.. py:data:: LOWFI
.. py:data:: GRAY2
.. py:data:: GRAY4
.. py:data:: GRAY8
.. py:data:: RGB332
.. py:data:: HASH_CACHE
.. py:data:: KEEP_DATA
.. py:data:: INTRA_UPDATE
.. py:data:: STAY_LOW
