import pygame
from _sim import path_replace

_loaded = False


def stop():
    """
    Stops media playback, frees resources.
    """
    global _loaded
    _loaded = False
    pygame.mixer.music.stop()
    pygame.mixer.music.unload()


def load(path, paused=False):
    """
    Load path
    """
    global _loaded
    if path.startswith(("http://", "https://")):
        return
    pygame.mixer.music.load(path_replace(path))
    pygame.mixer.music.play()
    if paused:
        pygame.mixer.music.pause()
    _loaded = True


def play():
    if not _loaded:
        return
    pygame.mixer.music.unpause()


def pause():
    if not _loaded:
        return
    pygame.mixer.music.pause()


def is_playing():
    if not _loaded:
        return False
    return pygame.mixer.music.get_busy()


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


def set_volume(vol):
    if not _loaded:
        return
    pygame.mixer.music.set_volume(vol)


def get_volume():
    if not _loaded:
        return 1.0
    return pygame.mixer.music.get_volume()


def get_position():
    if not _loaded:
        return 0
    pos = pygame.mixer.music.get_pos() / 1000
    if pos < 0:
        pos = get_duration()
    return pos


def get_time():
    return get_position()


def seek(pos):
    if not _loaded:
        return
    pygame.mixer.music.play()
    pygame.mixer.music.rewind()


def get_duration():
    return 99999


def is_visual():
    return False


def has_video():
    return False


def has_audio():
    return _loaded
