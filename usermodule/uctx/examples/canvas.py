#!/usr/bin/env uctx

import os
import sys
import ctx as modctx

_ctx = None

# this is the function exposed to the outside
def get_ctx(): 
  global _ctx
  if _ctx != None:  # we try to only run once
      return _ctx

  if sys.platform == 'linux':
    _ctx = unix_get_ctx ()
  elif sys.platform == 'javascript':
    _ctx = wasm_get_ctx ()
  else:
    print('you need to configure a display config for ctx in canvas.py')

    #_ctx = rp_pico_waveshare_240x240_joystick()
    _ctx = rp_pico_waveshare_480x320_res_touch()
    #_ctx = rp_pico_waveshare_240x320_res_touch()
    #_ctx = esp32_waveshare_320x240()

    #_ctx = dummy_get_ctx ()
  return _ctx

# The canvas module provides backend integration and configuration for
# ctx, by importing canvas.

scratch_size = 53 * 1024
    # for the cb backend used on microcontrollers, how much heap
    # to use for a scratch compositing buffer.
    # For a 480x320 display:
    #   14kb is enough for 2x2 GRAY2
    #                      5x5 LOWRES
    #   23kb is enough for 2x2 GRAY4 
    #                      3x3 RGB332
    #                      4x4 LOWRES
    #                      1x1 GRAY1  (not currently available)
    #   42kb is enough for 2x2 RGB332
    #                      1x1 GRAY2
    #                      3x3 LOWRES

canvas_flags = modctx.INTRA_UPDATE|modctx.LOWFI
    # the default flags the cb backend gets initialized with
    # possible values:
    #   INTRA_UPDATE : call the event handling between renders in hifi mode
    #   HASH_CACHE   : keep track of rendered content and only re-render changed areas
    #   LOWFI        : do preview rendering with RGB565
    #   RGB332       : trade color fidelity for higher res preview
    #   GRAY8        : full grayscale
    #   GRAY4        : 16 level gray
    #   GRAY2        : 4 level gray
    #   CBRLE        : constant bitrate RLE

# when non-zero override board default
canvas_width  = 0
canvas_height = 0

##### end of basic configuration ###########
############################################
############################################

pointer_down = False
pointer_x = 0
pointer_y = 0

# this is currently used by one of the backends
# adding gpio based input for more boards should
# follow the same pattern

keys={}
key_state={}
def get_pin_events(ctx):
    ret = False
    for name,pin in keys.items():
       if pin.value()== 0:
           if key_state[name] != 0:
               key_state[name]=0
               ctx.key_down(0, name, 0)
               ctx.key_press(0, name, 0)
               ret = True
       else:
          if key_state[name] != 1:
               ctx.key_up(0, name, 0)
               key_state[name]=1
               ret = True
    return ret


def rp_waveshare_240x240_joystick():
    import machine
    from machine import Pin,SPI,PWM
    global keys,key_state,canvas_width, canvas_height
    machine.freq(266_000_000)


    keys['a']= Pin(15,Pin.IN,Pin.PULL_UP)
    keys['b'] = Pin(17,Pin.IN,Pin.PULL_UP)
    keys['x'] = Pin(19 ,Pin.IN,Pin.PULL_UP)
    keys['y'] = Pin(21 ,Pin.IN,Pin.PULL_UP)
    
    keys['up'] = Pin(2,Pin.IN,Pin.PULL_UP)
    keys['down'] = Pin(18,Pin.IN,Pin.PULL_UP)
    keys['left'] = Pin(16,Pin.IN,Pin.PULL_UP)
    keys['right'] = Pin(20,Pin.IN,Pin.PULL_UP)
    keys['space'] = Pin(3,Pin.IN,Pin.PULL_UP)

    for key,value in keys.items():
       key_state[key]=1
        
    import lcd13
    import machine
    display=lcd13.LCD_1inch3()
    
    if canvas_width == 0:
        canvas_width = 240
    if canvas_height == 0:
        canvas_height = 240

    ret_ctx = modctx.Context(
      width=canvas_width,
      height=canvas_height,
      format=modctx.RGB565_BYTESWAPPED,
      # function to update a subregion of target framebuffer using prepared buf
      # with appropriate stride
      set_pixels=lambda x, y, w, h, buf: display.blit_buffer(buf, x, y, w, h),
      update=lambda:get_pin_events (ret_ctx),
      memory_budget=scratch_size,
      flags=canvas_flags)
    return ret_ctx
def update_events(lcd):
   global pointer_down, pointer_x, pointer_y
   touch=lcd.touch_get()
   if(touch):
      pointer_x=touch[0]
      pointer_y=touch[1]
      if not pointer_down:
         _ctx.pointer_press(pointer_x,pointer_y, 0, 0)
      else:
         _ctx.pointer_motion(pointer_x,pointer_y, 0, 0)
      pointer_down = True
      return True # abort rendering if in slow refresh
   else:
      if pointer_down:
         _ctx.pointer_release(pointer_x,pointer_y, 0, 0)
         pointer_down = False
         return True
      return False

def rp_pico_waveshare_480x320_res_touch():
    global canvas_width, canvas_height
    import ililcd
    lcd=ililcd.LCD()#480, 320, 15, 8, 9, 10, 11, 12, 13)
    if canvas_width == 0:
        canvas_width = 480
    if canvas_height == 0:
        canvas_height = 320

    return modctx.Context(
      width=canvas_width,
      height=canvas_height,
                             
      # the format specified here is the format of the lcd
      # not neccesarily the format used for rendering
      format=modctx.RGB565_BYTESWAPPED,

      # function to update a subregion of target framebuffer using prepared buf
      # with appropriate stride
      set_pixels=lambda x, y, w, h, buf: lcd.blit_buffer(buf, x, y, w, h),
                             
      # callback to be invoked in between batches of render+blit
      # if True is returned progressive updates are aborted
      # this is where touch events and button presses should be fed
      # to ctx
      update=lambda : update_events(lcd),
      memory_budget=scratch_size,
      flags=canvas_flags)

def dummy_get_ctx():
    global canvas_width, canvas_height
    if canvas_width == 0:
        canvas_width = 240
    if canvas_height == 0:
        canvas_height = 240

    return modctx.Context(
      width=canvas_width,
      height=canvas_height,
      format=modctx.RGB565_BYTESWAPPED,
      # function to update a subregion of target framebuffer using prepared buf
      # with appropriate stride
      set_pixels = lambda x, y, w, h, buf: True,
      update= lambda : False,
      memory_budget=scratch_size,
      flags=canvas_flags)

def esp32_waveshare_320x240():
    import st7789py
    import machine    
    machine.freq(240_000_000) # we want full tilt!
    spi = machine.SPI(2, baudrate=40000000, polarity=1)
    display = st7789py.ST7789(
        spi, 240, 320,
        reset=machine.Pin(17, machine.Pin.OUT),
        dc=machine.Pin(16, machine.Pin.OUT),
        cs=machine.Pin(5, machine.Pin.OUT),
        xstart=0,ystart=0
    )
    display.init()
    display.inversion_mode(False)

    if canvas_width == 0:
        canvas_width = 240
    if canvas_height == 0:
        canvas_height = 320

    return modctx.Context(
      width=canvas_width,
      height=canvas_height,
      format=modctx.RGB565_BYTESWAPPED,
      # function to update a subregion of target framebuffer using prepared buf
      # with appropriate stride
      set_pixels = lambda x, y, w, h, buf: display.blit_buffer(buf, x, y, w, h),
      update= lambda : False,
      memory_budget=scratch_size,
      flags=canvas_flags)
  
def rp_pico_waveshare_240x320_res_touch():
    global canvas_width, canvas_height
    import st7789py
    import time, machine
    machine.Pin(13,machine.Pin.OUT).low()
    spi = machine.SPI(1, baudrate=50000000, phase=1,polarity=1)
    display = st7789py.ST7789(spi, 240, 320,
        reset=machine.Pin(15, machine.Pin.OUT),
        backlight=machine.Pin(13, machine.Pin.OUT),
        sck=10,#machine.Pin(10, machine.Pin.OUT),        
        mosi=11,
        miso=12,
        dc=machine.Pin(8, machine.Pin.OUT),
        cs=machine.Pin(9, machine.Pin.OUT),xstart=0,ystart=0,tp_irq=17,tp_cs=18)
    display.init()
    if canvas_width == 0:
        canvas_width = 240
    if canvas_height == 0:
        canvas_height = 320
    
    return modctx.Context(
      width=canvas_width,
      height=canvas_height,
      format=modctx.RGB565_BYTESWAPPED,
      # function to update a subregion of target framebuffer using prepared buf
      # with appropriate stride
      set_pixels=lambda x, y, w, h, buf: display.blit_buffer(buf, x, y, w, h),
      update = lambda : update_events(display),
      memory_budget=scratch_size,
      flags=canvas_flags)

def reset_terminal():
    os.system('reset')

def unix_get_ctx():  # perhaps turn this into an attribute of the module?
  print("\e[H\e[2J")
  sys.atexit(reset_terminal)
  return modctx.Context()

def wasm_get_ctx():
  ctx = modctx.Context(memory_budget=scratch_size)
  ctx.flags=canvas_flags
  return ctx

def __getattr__(name):
    if name == 'ctx2d':
        return get_ctx()    
    elif name == 'ctx':
        return get_ctx()
    elif name == 'width':
        return get_ctx().width
    elif name == 'height':
        return get_ctx().height
    raise AttributeError("module '{__name__}' has no attribute '{name}'")

if __name__=='__main__':
    ctx = get_ctx()
    frames=60
    for frame in range(0,frames):
      ctx.start_frame()
      ctx.rgb(50/255,50/255,30/255).rectangle(0,0,ctx.width,ctx.height).fill()
      dim = ctx.height * frame / (frames-2.0)
      if frame >= frames-2:
        dim = ctx.height
      ctx.logo(ctx.width/2,ctx.height/2, dim)
      ctx.end_frame()



