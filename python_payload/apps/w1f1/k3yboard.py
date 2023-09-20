# code from https://git.flow3r.garden/baldo/k3yboard
# LGPL-v3-only, by baldo, 2023

from ctx import Context

import st3m.run
from st3m import Responder, InputState
from st3m.application import Application, ApplicationContext
from st3m.goose import ABCBase, Enum
from st3m.ui.view import BaseView, ViewManager
from st3m.utils import tau


class Model(ABCBase):
    """
    Common base-class for models holding state that can be used for rendering views.
    """

    def __init__(self):
        pass


class TextInputModel(Model):
    """
    Model used for rendering the TextInputView. Holds the current input.

    The input is split into the part left and the part right of the cursor. The input candidate is a character that
    might be added to the input next and is displayed at the position of the cursor. This is used to give viusal
    feedback while toggling through multiple characters inside a input group (associated with a petal).
    """

    class CursorDirection(Enum):
        LEFT = -1
        RIGHT = 1

    def __init__(
        self, input_left: str = "", input_right: str = "", input_candidate: str = ""
    ) -> None:
        super().__init__()

        self._input_left = input_left
        self._input_right = input_right
        self.input_candidate = input_candidate

    @property
    def text(self) -> str:
        """
        The complete input string in its current state.
        """
        return self._input_left + self._input_right

    @property
    def input_left(self) -> str:
        return self._input_left

    @property
    def input_right(self) -> str:
        return self._input_right

    def move_cursor(self, direction: self.CursorDirection) -> None:
        """
        Moves the cursor one step in specified direction. Any pending input will be committed beforehand.
        """
        self.commit_input()

        if direction == self.CursorDirection.LEFT:
            self._input_right = self._input_left[-1:] + self._input_right
            self._input_left = self._input_left[:-1]

        elif direction == self.CursorDirection.RIGHT:
            self._input_left = self._input_left + self._input_right[0:1]
            self._input_right = self._input_right[1:]

    def add_input_character(self, char: str) -> None:
        """
        Adds an input character at the current cursor position.
        """
        self._input_left += char
        self.input_candidate = ""

    def delete_input_character(self) -> None:
        """
        Deletes the character left to the cursor (if any).

        If an input candidate is pending, it will be removed instead.
        """
        if self.input_candidate == "":
            self._input_left = self._input_left[:-1]

        self.input_candidate = ""

    def commit_input(self) -> None:
        """
        Adds the pending input candidate (if any) to input left of the cursor.
        """
        self.add_input_character(self.input_candidate)


class TextInputFieldView(Responder):
    """
    Displays the current text input and cursor of a keyboard.
    """

    def __init__(
        self, model: TextInputModel, cursor_blink_duration_ms: float = 500
    ) -> None:
        super().__init__()

        self._model = model

        self._cursor_blink_duration_ms = cursor_blink_duration_ms
        self._cursor_timer_ms = 0

    def draw_cursor(self, ctx: Context) -> None:
        if self._cursor_blink_duration_ms < self._cursor_timer_ms:
            return

        ctx.begin_path()
        ctx.rgb(0.0, 0.2, 1.0).rectangle(-1.0, -15.0, 2.0, 28.0).fill()
        ctx.close_path()

    def draw_text_input(self, ctx: Context) -> None:
        ctx.begin_path()

        cursor_offset = 1.5

        ctx.text_baseline = ctx.MIDDLE
        ctx.font_size = 24

        left_input_width = ctx.text_width(self._model.input_left)
        input_candidate_width = ctx.text_width(self._model.input_candidate)
        right_input_width = ctx.text_width(self._model.input_right)

        ctx.gray(0.2).rectangle(
            -1.0 - cursor_offset - input_candidate_width - left_input_width,
            ctx.font_size / 2.4,
            left_input_width
            + input_candidate_width
            + right_input_width
            + 2 * cursor_offset
            + 2 * 1.0,
            2,
        ).fill()

        ctx.text_align = ctx.END
        ctx.gray(1.0).move_to(-cursor_offset - input_candidate_width, 0).text(
            self._model.input_left
        )

        ctx.text_align = ctx.END
        ctx.rgb(0.0, 0.2, 1.0).move_to(-cursor_offset, 0).text(
            self._model.input_candidate
        )

        ctx.text_align = ctx.START
        ctx.gray(1.0).move_to(cursor_offset, 0).text(self._model.input_right)

        ctx.close_path()

    def draw(self, ctx: Context) -> None:
        ctx.begin_path()

        self.draw_text_input(ctx)
        self.draw_cursor(ctx)

        ctx.close_path()

    def think(self, ins: InputState, delta_ms: int) -> None:
        self._cursor_timer_ms = (self._cursor_timer_ms + delta_ms) % (
            2 * self._cursor_blink_duration_ms
        )


class InputControlsModel(Model):
    """
    Model used for rendering the InputControlsView.

    Holds the current active input and control groups (icons displayed in a ring around the edge of the screen).

    Input groups are groups of characters to toggle through to select the character to input.

    Control groups are groups of icons to control the behaviour of the keyboard.
    """

    def __init__(
        self,
        input_groups: list[list[str]],
        control_groups: list[list[str]],
        active_input_group: int = 0,
    ) -> None:
        super().__init__()

        self._input_groups = input_groups
        self._control_groups = control_groups
        self._active_input_control_group = active_input_group

    @property
    def active_input_control_group(self):
        return self._active_input_control_group

    def select_input_control_group(self, group: int) -> None:
        self._active_input_control_group = group

    @property
    def active_input_groups(self) -> list[str]:
        return self._input_groups[self._active_input_control_group]

    @property
    def active_control_groups(self) -> list[str]:
        return self._control_groups[self._active_input_control_group]


class InputControlsView(Responder):
    """
    Shows a ring of controls and input characters to choose from around the edge.
    """

    def __init__(self, model: InputControlsModel) -> None:
        super().__init__()

        self._model = model

    def draw_input_group(self, ctx: Context, group: int) -> None:
        inputs = self._model.active_input_groups[group]

        ctx.begin_path()

        bottom_input_group = 2 <= group <= 3

        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE

        ctx.font_size = 18.0
        ctx.gray(0.6)

        angle_offset = tau / 5.0 / 10
        angle = group * tau / 5.0 - angle_offset * (len(inputs) - 1) / 2.0

        for input in reversed(inputs) if bottom_input_group else inputs:
            ctx.save()
            ctx.rotate(angle)
            ctx.translate(0, -109)

            if bottom_input_group:
                ctx.rotate(tau / 2)

            ctx.move_to(0, 0)
            ctx.text(input)
            ctx.restore()

            angle += angle_offset

        ctx.close_path()

    def draw_control_group(self, ctx, group: int) -> None:
        controls = self._model.active_control_groups[group]

        ctx.begin_path()

        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE

        ctx.font_size = 18.0
        ctx.gray(0.6)

        ctx.font = "Material Icons"

        angle_offset = tau / 5.0 / 10
        angle = (group + 0.5) * tau / 5.0 - angle_offset * (len(controls) - 1) / 2.0

        for control in controls:
            ctx.save()
            ctx.rotate(angle)
            ctx.translate(0, -109)

            ctx.rotate(-angle)

            ctx.move_to(0, 0)
            ctx.text(control)
            ctx.restore()

            angle += angle_offset

        ctx.close_path()

    def draw(self, ctx: Context) -> None:
        ctx.begin_path()

        ctx.line_width = 4.0
        ctx.gray(0).arc(0, 0, 98, 0, tau, 0).stroke()

        ctx.line_width = 24.0
        ctx.gray(0.1).arc(0, 0, 110, 0, tau, 0).stroke()

        ctx.line_width = 2.0
        ctx.gray(0.3).arc(0, 0, 99, 0, tau, 0).stroke()

        for i in range(0, len(self._model.active_input_groups)):
            self.draw_input_group(ctx, i)

        for i in range(0, len(self._model.active_control_groups)):
            self.draw_control_group(ctx, i)

        ctx.close_path()

    def think(self, ins: InputState, delta_ms: int) -> None:
        pass


class KeyboardView(BaseView):
    class ControlPetal(Enum):
        SPECIAL_CHARACTERS = 1
        BACKSPACE = 3
        SPACE = 5
        CAPSLOCK = 7
        NUMLOCK = 9

    class InputControlGroup(Enum):
        LOWERCASE_LETTERS = 0
        UPPERCASE_LETTERS = 1
        NUMBERS = 2
        SPECIAL_CHARACTERS = 3

    def __init__(self, model: TextInputModel) -> None:
        super().__init__()

        self._last_input_group_press = -1
        self._last_input_group_character = -1
        self._time_since_last_input_group_press = 0
        self._input_group_timeout = 1000

        self._text_input_model = model
        self._text_input_view = TextInputFieldView(self._text_input_model)

        self._input_controls_model = InputControlsModel(
            [
                [
                    "fghij",
                    "klmno",
                    "uvwxyz",
                    "pqrst",
                    "abcde",
                ],
                [
                    "FGHIJ",
                    "KLMNO",
                    "UVWXYZ",
                    "PQRST",
                    "ABCDE",
                ],
                [
                    "34",
                    "56",
                    "90",
                    "78",
                    "12",
                ],
                [
                    ".,!?:;",
                    "'\"@#~^",
                    "%$&<>\\|",
                    "+-_*/=`",
                    "()[]{}",
                ],
            ],
            [
                [
                    "\ue9ef",  # Special characters
                    "\ue14a",  # Backspace
                    "\ue256",  # Space
                    "\ue5ce",  # Shift
                    "\ue400",  # Num
                ],
                [
                    "\ue9ef",  # Special characters
                    "\ue14a",  # Backspace
                    "\ue256",  # Space
                    "\ue5cf",  # Shift active
                    "\ue400",  # Num
                ],
                [
                    "\ue9ef",  # Special characters
                    "\ue14a",  # Backspace
                    "\ue256",  # Space
                    "\ue264",  # Text
                    "",
                ],
                [
                    "",
                    "\ue14a",  # Backspace
                    "\ue256",  # Space
                    "\ue264",  # Text
                    "\ue400",  # Num
                ],
            ],
        )
        self._input_controls_view = InputControlsView(self._input_controls_model)

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)

    def on_exit(self) -> None:
        self.reset_input_state()

    def draw(self, ctx: Context) -> None:
        ctx.begin_path()

        ctx.gray(0).rectangle(-120, -120, 240, 240).fill()

        self._text_input_view.draw(ctx)
        self._input_controls_view.draw(ctx)

        ctx.close_path()

    def reset_input_state(self) -> None:
        self._time_since_last_input_group_press = 0
        self._last_input_group_press = -1
        self._last_input_group_character = -1
        self._text_input_model.input_candidate = ""

    def add_input_character(self, char: str) -> None:
        self._text_input_model.commit_input()
        self._text_input_model.add_input_character(char)
        self.reset_input_state()

    def delete_input_character(self) -> None:
        self._text_input_model.commit_input()
        self._text_input_model.delete_input_character()
        self.reset_input_state()

    def commit_input(self) -> None:
        self._text_input_model.commit_input()
        self.reset_input_state()

    def select_input_group(self, input_group) -> None:
        self.commit_input()
        self._input_controls_model.select_input_control_group(input_group)
        self.reset_input_state()

    def handle_shoulder_buttons(self) -> None:
        if self.input.buttons.app.middle.pressed:
            self.commit_input()
            if self.vm:
                self.vm.pop()

        if self.input.buttons.app.left.pressed:
            self._text_input_model.move_cursor(TextInputModel.CursorDirection.LEFT)

        if self.input.buttons.app.right.pressed:
            self._text_input_model.move_cursor(TextInputModel.CursorDirection.RIGHT)

    def handle_control_inputs(self) -> None:
        if self.input.captouch.petals[
            self.ControlPetal.SPECIAL_CHARACTERS
        ].whole.pressed:
            self.select_input_group(self.InputControlGroup.SPECIAL_CHARACTERS)

        if self.input.captouch.petals[self.ControlPetal.BACKSPACE].whole.pressed:
            self.delete_input_character()

        if self.input.captouch.petals[self.ControlPetal.SPACE].whole.pressed:
            self.add_input_character(" ")

        if self.input.captouch.petals[self.ControlPetal.CAPSLOCK].whole.pressed:
            self.select_input_group(
                self.InputControlGroup.UPPERCASE_LETTERS
                if self._input_controls_model.active_input_control_group
                == self.InputControlGroup.LOWERCASE_LETTERS
                else self.InputControlGroup.LOWERCASE_LETTERS
            )

        if self.input.captouch.petals[self.ControlPetal.NUMLOCK].whole.pressed:
            self.select_input_group(self.InputControlGroup.NUMBERS)

    def handle_input_groups(self, delta_ms: int) -> bool:
        for i in range(0, 5):
            if self.input.captouch.petals[i * 2].whole.pressed:
                if (
                    self._last_input_group_press >= 0
                    and self._last_input_group_press != i
                ):
                    self.commit_input()

                self._last_input_group_press = i
                self._last_input_group_character = (
                    self._last_input_group_character + 1
                ) % len(self._input_controls_model.active_input_groups[i])

                self._time_since_last_input_group_press = 0
                self._text_input_model.input_candidate = (
                    self._input_controls_model.active_input_groups[i][
                        self._last_input_group_character
                    ]
                )

                return

        self._time_since_last_input_group_press += delta_ms

        if (
            self._last_input_group_press >= 0
            and self._time_since_last_input_group_press > self._input_group_timeout
        ):
            self.commit_input()

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        self._text_input_view.think(ins, delta_ms)
        self._input_controls_view.think(ins, delta_ms)

        self.handle_shoulder_buttons()
        self.handle_control_inputs()
        self.handle_input_groups(delta_ms)


class KeyboardDemoApp(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)

        self._model = TextInputModel("Hello world!")

    def draw(self, ctx: Context) -> None:
        ctx.gray(0).rectangle(-120, -120, 240, 240).fill()

        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE

        ctx.font_size = 24
        ctx.gray(1).move_to(0, -50).text("Keyboard Demo")

        ctx.font_size = 16
        ctx.gray(1).move_to(0, -20).text("Press left button to edit")

        ctx.font_size = 24
        ctx.gray(1).move_to(0, 20).text("Current input:")

        ctx.font_size = 16
        ctx.gray(1).move_to(0, 50).text(self._model.text)

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)  # Let Application do its thing

        if self.is_active() and self.input.buttons.app.middle.pressed:
            self.vm.push(KeyboardView(self._model))


if __name__ == "__main__":
    st3m.run.run_app(KeyboardDemoApp)
