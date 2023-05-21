#!/usr/bin/env uctx

import uimui
exit=False
while not exit:
  uimui.start_frame()
  uimui.label("foo")
  valid_red=(0,40,80,140,174,209,233,255)  # 176
  valid_green=(0,40,80,140,174,209,233,255)  # 176
  valid_blue=(0,80,140,255)    

  ctx = uimui.get_ctx()
  y = 0
  x = 0
  dim = ctx.width / (36)
  for green in valid_green:
     for red in valid_red:
         for blue in valid_blue:
              real_blue = blue | (green &31) # mix in low bits of green
              ctx.rectangle(x, y, dim, dim).rgb(red/255,green/255,real_blue/255).fill()
              x += dim
              if x + dim >= ctx.width:
                  x = 0
                  y += dim
  if uimui.button("X", x= 0, y =0, width = ctx.width/10):
    exit = True  
  uimui.end_frame()
