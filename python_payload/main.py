import gc, time
from st3m import logging

log = logging.Log(__name__, level=logging.INFO)
log.info(f"starting main")
log.info(f"free memory: {gc.mem_free()}")

from st3m import control, application, ui, menu

log = logging.Log(__name__, level=logging.INFO)

ts_start = time.time()

from st3m import *

from apps import flow3r

log.info("import apps done")
log.info(f"free memory: {gc.mem_free()}")
ts_end = time.time()

log.info(f"boot took {ts_end-ts_start} seconds")

# TODO persistent settings
from st3m.system import hardware, audio

log.info("calibrating captouch, reset volume")
hardware.captouch_autocalib()
audio.set_volume_dB(0)

# Start default app
default_app = flow3r.app
log.info(f"running default app '{default_app.title}'")
default_app.run()
