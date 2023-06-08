from st3m.system import hardware,audio
from apps import flow3r

#TODO persistent settings
hardware.captouch_autocalib()
audio.set_volume_dB(0)

#Start default app
flow3r.app.run()