def stop():
    """
    Stops media playback, frees resources.
    """
    pass

def load(path):
    """
    Load path
    """
    pass

def draw(ctx):
    """
    Draws current state of media object to provided ctx context.
    """
    pass

def think(delta_ms):
    """
    Process ms amounts of media, queuing PCM data and preparing for draw()
    """
    pass
