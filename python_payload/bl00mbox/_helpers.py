# SPDX-License-Identifier: CC0-1.0
import time


def terminal_scope(
    signal,
    signal_min=-32767,
    signal_max=32767,
    delay_ms=20,
    width=80,
    fun=None,
    fun_ms=None,
):
    """give it a signal and show it on terminal uwu"""
    if signal_max <= signal_min:
        return
    ms_counter = 0
    fun_counter = 0
    if fun != None:
        fun()
    while True:
        if fun != None:
            if fun_ms != None:
                if fun_ms <= fun_counter:
                    fun()
                    fun_counter = fun_counter % fun_ms
        val = int((width * (signal.value - signal_min)) / (signal_max - signal_min))
        if val > width:
            val = width
        if val < 0:
            val = 0
        ret = f"{ms_counter:06d}"
        ret += " [" + "X" * val + "." * (width - val) + "]"
        percent = int(100 * val / width)
        ret += f" {percent:02d}%"
        print(ret)
        time.sleep_ms(delay_ms)
        ms_counter += delay_ms
        fun_counter += delay_ms


# terminal_scope(a.env.signals.output, 0, fun = a.start, fun_ms = 1000)


def sct_to_note_name(sct):
    sct = sct - 18367 + 100
    octave = ((sct + 9 * 200) // 2400) + 4
    tones = ["A", "Bb", "B", "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab"]
    tone = tones[(sct // 200) % 12]
    return tone + str(octave)


def note_name_to_sct(name):
    tones = ["A", "Bb", "B", "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab"]
    semitones = tones.index(name[0])
    if semitones > 2:
        semitones -= 12
    if name[1] == "b":
        octave = int(name[2:])
        semitones -= 1
    elif name[1] == "#":
        octave = int(name[2:])
        semitones += 1
    else:
        octave = int(name[1:])
    return 18367 + (octave - 4) * 2400 + (200 * semitones)


def sct_to_freq(sct):
    return 440 * 2 ** ((sct - 18367) / 2400)
