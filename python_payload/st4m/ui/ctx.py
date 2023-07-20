from st4m.goose import ABCBase, abstractmethod, List


class Ctx(ABCBase):
    """
    Ctx is the rendering/rasterization API used by st4m.

    It's a WebCanvas-style vector API, with an implicit pen which can be moved
    and can draw lines, arcs, text, etc.

    In st4m, the Ctx object is backed by a drawlist generator. That is, any
    operation performed on the ctx object will cause an entry to be added to an
    in-memory draw list. Then, when the rasterizer is ready, it will rasterize
    said drawlist to pixels in a separate thread.

        A Ctx object is passed to all draw() calls to Responder. This object should
        only be used within the lifecycle of the draw method and must not be
        persisted.

    For more information, see: https://ctx.graphics/
    """

    __slots__ = (
        "font_size",
        "text_align",
        "text_baseline",
        "line_width",
        "global_alpha",
    )

    CENTER = "center"
    MIDDLE = "middle"

    @abstractmethod
    def __init__(self) -> None:
        self.font_size: float = 10
        self.text_align: str = "start"
        self.text_baseline: str = "alphabetic"
        self.line_width: float = 1.0
        self.global_alpha: float = 1.0

    @abstractmethod
    def begin_path(self) -> "Ctx":
        """
        Clears the current path if any.
        """
        pass

    @abstractmethod
    def save(self) -> "Ctx":
        """
        Stores the transform, clipping state, fill and stroke sources, font
        size, stroking and dashing options.
        """
        pass

    @abstractmethod
    def restore(self) -> "Ctx":
        """
        Restores the state previously saved with save, calls to save/restore
        should be balanced.
        """
        pass

    @abstractmethod
    def start_frame(self) -> "Ctx":
        """
        Prepare for rendering a new frame, clears internal drawlist and
        initializes the state.

        TODO(q3k): we probably shouldn't expose this
        """
        pass

    @abstractmethod
    def end_frame(self) -> "Ctx":
        """
        We're done rendering a frame, this does nothing on a context created for
        a framebuffer, where drawing commands are immediate.

        TODO(q3k): we probably shouldn't expose this
        """
        pass

    @abstractmethod
    def start_group(self) -> "Ctx":
        """
        Start a compositing group.
        """
        pass

    @abstractmethod
    def end_group(self) -> "Ctx":
        """
        End a compositing group, the global alpha, compositing mode and blend
        mode set before this call is used to apply the group.
        """
        pass

    @abstractmethod
    def clip(self) -> "Ctx":
        """
        Use the current path as a clipping mask, subsequent draw calls are
        limited by the path. The only way to increase the visible area is to
        first call save and then later restore to undo the clip.
        """
        pass

    @abstractmethod
    def rotate(self, x: float) -> "Ctx":
        """
        Add rotation to the user to device space transform.
        """
        pass

    @abstractmethod
    def scale(self, x: float, y: float) -> "Ctx":
        """
        Scales the user to device transform.
        """
        pass

    @abstractmethod
    def translate(self, x: float, y: float) -> "Ctx":
        """
        Adds translation to the user to device transform.
        """
        pass

    @abstractmethod
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
    ) -> "Ctx":
        """
        Adds a 3x3 matrix on top of the existing user to device space transform.

        TODO(q3k): we probably shouldn't expose this
        """
        pass

    @abstractmethod
    def line_to(self, x: float, y: float) -> "Ctx":
        """
        Draws a line segment from the position of the last
        {line,move,curve,quad}_to) to the given coordinates.
        """
        pass

    @abstractmethod
    def move_to(self, x: float, y: float) -> "Ctx":
        """
        Moves the virtual pen to the given coordinates without drawing anything.
        """
        pass

    @abstractmethod
    def curve_to(
        self, cx0: float, cy0: float, cx1: float, cy1: float, x: float, y: float
    ) -> "Ctx":
        """
        TOD(q3k): document
        """
        pass

    @abstractmethod
    def quad_to(self, cx: float, cy: float, x: float, y: float) -> "Ctx":
        """
        TOD(q3k): document
        """
        pass

    @abstractmethod
    def gray(self, a: float) -> "Ctx":
        """
        Set current draw color to a floating point grayscale value from 0 to 1.
        """
        pass

    @abstractmethod
    def rgb(self, r: float, g: float, b: float) -> "Ctx":
        """
        Set current draw color to an RGB color defined by component values from
        0 to 1.
        """
        pass

    @abstractmethod
    def rgba(self, r: float, g: float, b: float, a: float) -> "Ctx":
        """
        Set current draw color to an RGBA color defined by component values from
        0 to 1.
        """
        pass

    @abstractmethod
    def arc_to(
        self, x1: float, y1: float, x2: float, y2: float, radius: float
    ) -> "Ctx":
        """
        TOD(q3k): document
        """
        pass

    @abstractmethod
    def rel_line_to(self, x: float, y: float) -> "Ctx":
        """
        TOD(q3k): document
        """
        pass

    @abstractmethod
    def rel_move_to(self, x: float, y: float) -> "Ctx":
        """
        TOD(q3k): document
        """
        pass

    @abstractmethod
    def rel_curve_to(
        self, cx0: float, cy0: float, cx1: float, cy1: float, x: float, y: float
    ) -> "Ctx":
        """
        TOD(q3k): document
        """
        pass

    @abstractmethod
    def rel_quad_to(self, cx: float, cy: float, x: float, y: float) -> "Ctx":
        """
        TOD(q3k): document
        """
        pass

    @abstractmethod
    def rel_arc_to(
        self, x1: float, y1: float, x2: float, y2: float, radius: float
    ) -> "Ctx":
        """
        TOD(q3k): document
        """
        pass

    @abstractmethod
    def rectangle(self, x: float, y: float, w: float, h: float) -> "Ctx":
        """
        TOD(q3k): document
        """
        pass

    @abstractmethod
    def round_rectangle(
        self, x: float, y: float, w: float, h: float, r: float
    ) -> "Ctx":
        """
        TOD(q3k): document
        """
        pass

    @abstractmethod
    def arc(
        self,
        x: float,
        y: float,
        radius: float,
        angle1: float,
        angle2: float,
        direction: float,
    ) -> "Ctx":
        """
        TOD(q3k): document
        """
        pass

    @abstractmethod
    def close_path(self) -> "Ctx":
        """
        TOD(q3k): document
        """
        pass

    @abstractmethod
    def preserve(self) -> "Ctx":
        """
        TOD(q3k): document
        """
        pass

    @abstractmethod
    def fill(self) -> "Ctx":
        """
        TOD(q3k): document
        """
        pass

    @abstractmethod
    def stroke(self) -> "Ctx":
        """
        TOD(q3k): document
        """
        pass

    @abstractmethod
    def paint(self) -> "Ctx":
        """
        TOD(q3k): document
        """
        pass

    @abstractmethod
    def linear_gradient(self, x0: float, y0: float, x1: float, y1: float) -> "Ctx":
        """
        Change the source to a linear gradient from x0,y0 to x1,y1, by default
        an empty gradient from black to white exists, add stops with
        gradient_add_stop to specify a custom gradient.

        TODO(q3k): check gradient_add_stop
        """
        pass

    @abstractmethod
    def radial_gradient(
        self, x0: float, y0: float, r0: float, x1: float, y1: float, r1: float
    ) -> "Ctx":
        """
        Change the source to a radial gradient from a circle x0,y0 with radius0
        to an outer circle x1,y1 with radidus r1.

        NOTE: currently only the second circle's origin is used, but both
        radiuses are in use.
        """
        pass

    @abstractmethod
    def logo(self, x: float, y: float, dim: float) -> "Ctx":
        """
        TOD(q3k): document
        """
        pass

    @abstractmethod
    def text(self, text: str) -> "Ctx":
        """
        TOD(q3k): document
        """
        pass
