import gc
import uos

vfs = uos.VfsPosix()
uos.mount(vfs, "/")

gc.collect()
