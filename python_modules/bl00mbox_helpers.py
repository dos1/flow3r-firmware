import time

def terminal_scope(signal, signal_min = -32767, signal_max = 32767, delay_ms = 20, width = 80, fun = None, fun_ms = None):
    '''give it a signal and show it on terminal uwu'''
    if signal_max <= signal_min:
        return
    ms_counter = 0
    fun_counter = 0
    if fun != None:
        fun();
    while True:
        if fun != None:
            if fun_ms != None:
                if fun_ms <= fun_counter:
                    fun()
                    fun_counter = fun_counter % fun_ms
        val = int((width*(signal.value-signal_min))/(signal_max-signal_min))
        if val > width:
            val = width
        if val < 0:
            val = 0
        ret = f"{ms_counter:06d}"
        ret += " [" + "X"*val + "."*(width-val) + "]"
        percent = int(100*val/width)
        ret += f" {percent:02d}%"
        print(ret)
        time.sleep_ms(delay_ms)
        ms_counter += delay_ms
        fun_counter += delay_ms

#terminal_scope(a.env.signals.output, 0, fun = a.start, fun_ms = 1000)
