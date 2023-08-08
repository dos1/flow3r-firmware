from ctx import Context

def load(path: str) -> int:
    """
    Load path
    """
    ...

def iterate(ctx: Context) -> int:
    """
    Iterates video decoding, draws frame to ctx.
    Returns 0 if video is still playing
    """
    ...

def cleanup() -> int:
    """
    Clean up resources used by loaded video.
    """
    ...
