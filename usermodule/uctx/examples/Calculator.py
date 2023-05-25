#!/usr/bin/env uctx

import canvas
import micropython
import gc

class Calculator:
    def __init__(self):
        True
    
    def foo(self):
        print("fooed")


calc=Calculator()
calc.foo()
ctx=canvas.ctx
o=ctx

ctx.flags = ctx.HASH_CACHE | ctx.INTRA_UPDATE

pointer_x = 0
pointer_y = 0
pointer_down = False

pointer_was_down = False

widget_font_size_vh = 0.12
widget_bg=[70/255,70/255,90/255]
widget_fg=[1.0,1.0,1.0]
widget_marker=[1.0,0,0]
widget_fs=0.1

button_bg=widget_bg
button_fg=widget_fg
button_bg_active=widget_bg#[200,20,100]

def inside_rect(u,v,x,y,w,h):
    if u >= x and  u < x + w and v >= y and v < y + h:
      return True
    return False

def button(x, y, w, h, label):
    global pointer_down, pointer_was_down
    result = False
    ctx.font_size = ctx.height * widget_font_size_vh # why do we need this override
    lwidth = ctx.text_width (label)
    ctx.rectangle(x, y, w, h)
    ctx.rgb(*button_bg)
    if pointer_down and pointer_was_down == False and inside_rect(pointer_x,pointer_y,x,y,w,h):
      result = True
      ctx.rgb(*button_bg_active)    
    ctx.fill()
    ctx.rgb(*button_fg)
    ctx.move_to(x + w/2-lwidth/2, y + h/2 + ctx.font_size/3)
    ctx.text(label)
    return result

def toggle(x, y, w, h, label, value):
    global pointer_down, pointer_was_down
    result = value
    ctx.font_size = ctx.height * widget_font_size_vh # why do we need this override
    
    if pointer_down and pointer_was_down == False and inside_rect(pointer_x,pointer_y,x,y,w,h):
      result = not value
      ctx.rectangle(x, y, w, h).rgb(*button_bg_active).fill()

    ctx.rgb(*widget_fg)
    ctx.move_to(x + ctx.font_size*0.1, y + ctx.font_size*0.8)
    ctx.text(label)
    ctx.save()
    ctx.text_align=ctx.RIGHT
    ctx.move_to(x+w,y+ctx.font_size*0.8)
    if value:
        ctx.text("on")
    else:
        ctx.text("off")
    ctx.restore()

    return value


def coords_press(x, y):
    global pointer_x, pointer_y, pointer_down
    pointer_x = x
    pointer_y = y
    pointer_down = True

def coords_motion(x, y):
    global pointer_x, pointer_y
    pointer_x = x
    pointer_y = y

def coords_release(x, y):
    global pointer_x, pointer_y, pointer_down
    pointer_x = x
    pointer_y = y
    pointer_down = False

exit = False
need_clear = True
frame_no = 0
value = 0
accumulator = 0
op = ""
in_decimal = 0  # higher than zero means were in decimals at that digit
hex_mode = False

def compute_op():
    global need_clear,value, accumulator, op, in_decimals
    if op == '+':
      value = accumulator + value
      accumulator = 0
    if op == '-':
      value = accumulator - value
      accumulator = 0
    if op == 'x':
      value = accumulator * value
      accumulator = 0
    if op == '/':
      if value != 0.0:
         value = accumulator / value
      else:
         value = -0.0
      accumulator = 0
    op = ''
    need_clear = True

def calc_loop():
    global hex_mode,need_clear,value, accumulator, in_decimal, op, exit, pointer_was_down, pointer_down
    bw = ctx.width * 0.13
    bh = ctx.height * 0.19
    bsx = ctx.width * 0.14
    bsy = ctx.height * 0.2
    ctx.start_frame()
    ctx.translate(0,20)
    ctx.rectangle(0,0,ctx.width, ctx.height)#,ctx.height)
    o.listen(o.PRESS, lambda e:coords_press(e.x, e.y))
    o.listen(o.MOTION, lambda e:coords_motion(e.x, e.y))
    o.listen(o.RELEASE, lambda e:coords_release(e.x, e.y))
    
    ctx.gray(0).fill()

    ctx.font_size = ctx.height * widget_font_size_vh
    ctx.rgb(*widget_fg)
    
    ctx.text_align = ctx.RIGHT
    ctx.move_to(bsx*6,ctx.font_size*0.8)
    if hex_mode:
      value = int(value)
      ctx.text(hex(value))
    else:
      ctx.text(str(value))
    ctx.text_align = ctx.LEFT

    if accumulator != 0:
        ctx.move_to(0,ctx.font_size*0.8)
        ctx.text(str(accumulator) + " " + op)


    for digit in range(1,10):
      if button((((digit-1)%3)+1) * bsx, (3-int((digit-1)/3)) * bsy, bw, bh, str(digit)):
        if need_clear:
            value = 0
        need_clear = False
        if in_decimal>0:
            value = value + 1.0/pow(10,in_decimal)*digit
            in_decimal+=1
        else:
            value = value * 10 + digit
    if button(0 * bsx, 3 * bsy, bw, bh, "0"):
        if need_clear:
           value = 0
        need_clear = False

        if in_decimal>0:
            value = value + 0.0
        else:
            value = value * 10 + 0
        
    if button(0 * bsx, 2 * bsy, bw, bh, "."):
        in_decimal = 1
    if button(4 * bsx, 3 * bsy, bw * 1.5, bh, "="):
        compute_op()
        
    if button(5 * bsx, 1 * bsy, bw, bh, "/"):
        compute_op()
        accumulator = value
        value = 0
        op = "/"
        in_decimal = 0
    if button(6 * bsx, 2 * bsy, bw, bh, "sqrt"):
        value = pow(value, 0.5)
        op = ""
        need_clear = 1
        in_decimal = 0

    if hex_mode:
      if button(6 * bsx, 1 * bsy, bw, bh, "hex"):
        hex_mode = False
    else:
      if button(6 * bsx, 1 * bsy, bw, bh, "dec"):
        hex_mode = True

    if button(5.5 * bsx, 3 * bsy, 1.5 * bw, bh, "C"):
        value = 0
        accumulator = 0
        in_decimal = 0
    if button(4 * bsx, 2 * bsy, bw, bh, "+"):
        compute_op()
        accumulator = value
        value = 0
        op = "+"
        in_decimal = 0
    if button(5 * bsx, 2 * bsy, bw, bh, "x"):
        compute_op()        
        accumulator = value
        value = 0
        op = "x"
        in_decimal = 0
    if button(4 * bsx, 1 * bsy, bw, bh, "-"):
        compute_op()
        accumulator = value
        value = 0
        op = "-"
        in_decimal = 0
    if button(0, 1 * bsy, bw, bh, "q"):
        exit = True
    
    #ctx.rectangle(pointer_x-2, pointer_y-2, 4, 4).color([255,0,0]).fill()
    pointer_was_down = pointer_down # must happen after all the buttons
    #micropython.mem_info()
    ctx.end_frame()
    gc.collect()    
    
import micropython
    
while not exit:
    calc_loop()
    #micropython.mem_info(1)


