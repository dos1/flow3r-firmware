import leds


def copy_across_devices(src: str, dst: str):
    """
    copy a file across devices (flash->SD etc).
    does the whole file at once, should only be used on small files.
    """
    with open(src, "rb") as srcf:
        with open(dst, "wb") as dstf:
            dstf.write(srcf.read())


def set_direction_leds(direction, r, g, b):
    if direction == 0:
        leds.set_rgb(39, r, g, b)
    else:
        leds.set_rgb((direction * 4) - 1, r, g, b)
    leds.set_rgb(direction * 4, r, g, b)
    leds.set_rgb((direction * 4) + 1, r, g, b)


def mark_unknown_characters(text: str) -> str:
    glyph_index = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~ ¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿŁπ“”•…€™←↑→↓−≈▼♠♣♥♦ﬁﬂﬃﬄ"
    result_text = ""
    for char in text:
        result_text += char if char in glyph_index else "?"
    return result_text
