from ctx import Context

def stop() -> None:
    """
    Stops media playback, frees resources.
    """
    ...

def load(path: str) -> int:
    """
    Load path
    """
    ...

def draw(ctx: Context) -> None:
    """
    Draws current state of media object to provided ctx context.
    """
    ...

def think(ms: float) -> None:
    """
    Process ms amounts of media, queuing PCM data and preparing for draw()
    """
    ...
