class MenuEntry:
    label: str = ""
    icon: str = " "

    def __init__(
        self,
        label: str,
        icon: str = " ",
    ) -> None:
        self.label = label
        self.icon = icon