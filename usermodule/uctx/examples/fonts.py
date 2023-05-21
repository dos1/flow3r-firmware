#!/usr/bin/env uctx
import canvas
ctx = canvas.get_ctx()

ctx.flags=0

frames=400
for frame in range(0,frames):
        ctx.start_frame()
        ctx.rgb(50/255,50/255,30/255).rectangle(0,0,ctx.width,ctx.height).fill()
        dim = ctx.height * frame / (frames-2.0)
        ctx.rgb(1.0,1.0,1.0)
        ctx.font_size= ctx.height * 0.15
        ctx.save()
        ctx.translate(0,-frame/frames * ctx.height *2.5)
        ctx.move_to(0,ctx.height)
        ctx.rgba(1.0,0.0,0.0, .85)

        for font in ('Sans','Regular',
                     'Sans Bold','Bold',
                     "Courier Italic",
                     'Times',
                     'Mono',
                     'Arial Bold',
                     'Foobar Bold Italic',
                     'Helvetica Bold',
                     'Mono Bold',
                     'Mono Italic',
                     'Bold Italic', 
                     'Times Italic'):
          ctx.font=font
          ctx.text(font+'\n')

        ctx.rgba(1.0,1.0,1.0, .85)
        
        font_no = 0 
        
        while ctx.get_font_name(font_no) != None:
          font = ctx.get_font_name(font_no)
          font_no+=1
          ctx.font=font
          ctx.text(font+"\n")

          dim=ctx.height
        ctx.restore()
        ctx.end_frame()


