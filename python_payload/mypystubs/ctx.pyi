try:
    from typing import Protocol, Tuple
except ImportError:
    from typing_extensions import Protocol, Tuple  # type: ignore

class Context(Protocol):
    """
    Context is the rendering/rasterization API used by st3m.

    It's a WebCanvas-style vector API, with an implicit pen which can be moved
    and can draw lines, arcs, text, etc.

    In st3m, the Context object is backed by a drawlist generator. That is, any
    operation performed on the ctx object will cause an entry to be added to an
    in-memory draw list. Then, when the rasterizer is ready, it will rasterize
    said drawlist to pixels in a separate thread.

    A Context object is passed to all draw() calls to Responder. This object
    should only be used within the lifecycle of the draw method and must not be
    persisted.

    For more information, see: https://ctx.graphics/
    """

    image_smoothing: bool = True
    font_size: float = 10
    text_align: str = "start"
    text_baseline: str = "alphabetic"
    line_width: float = 1.0
    global_alpha: float = 1.0
    font: str = ""

    ALPHABETIC: str = "alphabetic"
    TOP: str = "top"
    HANGING: str = "hanging"
    MIDDLE: str = "middle"
    IDEOGRAPHIC: str = "ideographic"
    BOTTOM: str = "bottom"

    START: str = "start"
    END: str = "end"
    JUSTIFY: str = "justify"
    CENTER: str = "center"
    LEFT: str = "left"
    RIGHT: str = "right"

    def __init__(self) -> None: ...
    def text_width(self, text: str) -> float:
        """
        Calculates width of rendered text, without rendering it.
        """
        pass
    def get_font_name(self, ix: int) -> str:
        """
        Returns font name from internal font index. See ctx_config.h for
        defined fonts.

        TODO(q3k): expose these font indices into mpy
        """
        pass
    def begin_path(self) -> "Context":
        """
        Clears the current path if any.
        """
        pass
    def save(self) -> "Context":
        """
        Stores the transform, clipping state, fill and stroke sources, font
        size, stroking and dashing options.
        """
        pass
    def restore(self) -> "Context":
        """
        Restores the state previously saved with save, calls to save/restore
        should be balanced.
        """
        pass
    def start_frame(self) -> "Context":
        """
        Prepare for rendering a new frame, clears internal drawlist and
        initializes the state.

        TODO(q3k): we probably shouldn't expose this
        """
        pass
    def end_frame(self) -> "Context":
        """
        We're done rendering a frame, this does nothing on a context created for
        a framebuffer, where drawing commands are immediate.

        TODO(q3k): we probably shouldn't expose this
        """
        pass
    def start_group(self) -> "Context":
        """
        Start a compositing group.
        """
        pass
    def end_group(self) -> "Context":
        """
        End a compositing group, the global alpha, compositing mode and blend
        mode set before this call is used to apply the group.
        """
        pass
    def clip(self) -> "Context":
        """
        Use the current path as a clipping mask, subsequent draw calls are
        limited by the path. The only way to increase the visible area is to
        first call save and then later restore to undo the clip.
        """
        pass
    def rotate(self, x: float) -> "Context":
        """
        Add rotation to the user to device space transform.
        """
        pass
    def scale(self, x: float, y: float) -> "Context":
        """
        Scales the user to device transform.
        """
        pass
    def translate(self, x: float, y: float) -> "Context":
        """
        Adds translation to the user to device transform.
        """
        pass
    def apply_transform(
        self,
        a: float,
        b: float,
        c: float,
        d: float,
        e: float,
        f: float,
        g: float,
        h: float,
        i: float,
    ) -> "Context":
        """
        Adds a 3x3 matrix on top of the existing user to device space transform.

        TODO(q3k): we probably shouldn't expose this
        """
        pass
    def line_to(self, x: float, y: float) -> "Context":
        """
        Adds a line segment to the path, from the position of the virtual pen
        to the given coordinates, the coordinates are the new position of the
        virtual pen.
        """
        pass
    def move_to(self, x: float, y: float) -> "Context":
        """
        Moves the virtual pen to the given coordinates without drawing anything.
        """
        pass
    def curve_to(
        self, cx0: float, cy0: float, cx1: float, cy1: float, x: float, y: float
    ) -> "Context":
        """
        Add a cubic bezier segment to current path, with control handles at
        cx0, cy0 and cx1,cy1, the virtual pens new position is x, y.
        """
        pass
    def quad_to(self, cx: float, cy: float, x: float, y: float) -> "Context":
        """
        Add a quadratic bezier segment to current path with control point at
        cx,cy ending at x,y.
        """
        pass
    def gray(self, a: float) -> "Context":
        """
        Set current draw color to a floating point grayscale value from 0 to 1.
        """
        pass
    def rgb(self, r: float, g: float, b: float) -> "Context":
        """
        Set current draw color to an RGB color defined by component values from
        0 to 1.
        """
        pass
    def rgba(self, r: float, g: float, b: float, a: float) -> "Context":
        """
        Set current draw color to an RGBA color defined by component values from
        0 to 1.
        """
        pass
    def arc_to(
        self, x1: float, y1: float, x2: float, y2: float, radius: float
    ) -> "Context":
        """
        TOD(q3k): document
        """
        pass
    def rel_line_to(self, x: float, y: float) -> "Context":
        """
        TOD(q3k): document
        """
        pass
    def rel_move_to(self, x: float, y: float) -> "Context":
        """
        TOD(q3k): document
        """
        pass
    def rel_curve_to(
        self, cx0: float, cy0: float, cx1: float, cy1: float, x: float, y: float
    ) -> "Context":
        """
        TOD(q3k): document
        """
        pass
    def rel_quad_to(self, cx: float, cy: float, x: float, y: float) -> "Context":
        """
        TOD(q3k): document
        """
        pass
    def rel_arc_to(
        self, x1: float, y1: float, x2: float, y2: float, radius: float
    ) -> "Context":
        """
        TOD(q3k): document
        """
        pass
    def rectangle(self, x: float, y: float, w: float, h: float) -> "Context":
        """
        Trace the outline of a rectangle with upper left coordinates at x,y
        which is w wide and h high.
        """
        pass
    def round_rectangle(
        self, x: float, y: float, w: float, h: float, r: float
    ) -> "Context":
        """
        Trace the outline of a rectangle with upper left coordinates at x,y
        which is w wide and h high. With quarter circles with a radius of r
        for corners.
        """
        pass
    def arc(
        self,
        x: float,
        y: float,
        radius: float,
        angle1: float,
        angle2: float,
        direction: float,
    ) -> "Context":
        """
        TOD(q3k): document
        """
        pass
    def close_path(self) -> "Context":
        """
        Close the current open path with a curve back to where the current
        sequence of path segments started.
        """
        pass
    def preserve(self) -> "Context":
        """
        TOD(q3k): document
        """
        pass
    def fill(self) -> "Context":
        """
        TOD(q3k): document
        """
        pass
    def stroke(self) -> "Context":
        """
        TOD(q3k): document
        """
        pass
    def paint(self) -> "Context":
        """
        TOD(q3k): document
        """
        pass
    def add_stop(
        self, pos: float, color: Tuple[float, float, float], alpha: float
    ) -> "Context":
        """
        Adds a color stop for a linear or radial gradient. Pos is a position between 0.0 and 1.0.
        Should be called after linear_gradient or radial_gradient.
        """
        pass
    def linear_gradient(self, x0: float, y0: float, x1: float, y1: float) -> "Context":
        """
        Change the source to a linear gradient from x0,y0 to x1,y1, by default
        an empty gradient from black to white exists, add stops with
        add_stop to specify a custom gradient.

        This is a simple example rendering a rainbow gradient on the right side of the screen:

        >>> ctx.linear_gradient(0.18*120,0.5*120,0.95*120,0.5*120)
        >>> ctx.add_stop(0.0, [255,0,0], 1.0)
        >>> ctx.add_stop(0.2, [255,255,0], 1.0)
        >>> ctx.add_stop(0.4, [0,255,0], 1.0)
        >>> ctx.add_stop(0.6, [0,255,255], 1.0)
        >>> ctx.add_stop(0.8, [0,0,255], 1.0)
        >>> ctx.add_stop(1.0, [255,0,255], 1.0)
        >>> ctx.rectangle(-120, -120, 240, 240)
        >>> ctx.fill()
        """
        pass
    def radial_gradient(
        self, x0: float, y0: float, r0: float, x1: float, y1: float, r1: float
    ) -> "Context":
        """
        Change the source to a radial gradient from a circle x0,y0 with radius0
        to an outer circle x1,y1 with radidus r1.

        NOTE: currently only the second circle's origin is used, but both
        radiuses are in use.
        """
        pass
    def image(self, path: str, x: float, y: float, w: float, h: float) -> "Context":
        """
        Draw the image at path a in a rectangle with upper left coordinates at
        x,y which is w wide and h high. If w or h is -1 the other is set
        maintaining aspect ratio, if both are -1 the pixel dimensions of the
        image is used.
        """
        pass
    def logo(self, x: float, y: float, dim: float) -> "Context":
        """
        TOD(q3k): document
        """
        pass
    def text(self, text: str) -> "Context":
        """
        TOD(q3k): document
        """
        pass
    def scope(self) -> "Context":
        """
        Draw an audio 'oscilloscope'-style visualizer at -120,-120,120,120
        bounding box.

        Needs to be stroked/filled afterwards.
        """
        pass
