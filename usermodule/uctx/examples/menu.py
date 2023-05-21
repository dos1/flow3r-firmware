#!/usr/bin/env uctx

# If you are reading this on the web, this file is probably part of your
# persistent micropython filsystem on this URL (stored in your browser).
# as part of a rapid protoyping environment.
#
# main.py and canvas.py are copied afresh from the server on each reload, if
# canvas.py is missing all the preloaded contents are brought back. Navigate
# filesystem in dropdown, create files and folders by specifying paths to save.
#
# press ctrl+return to run code
#

import canvas # this contains the python configuration to create a context
import io,os,gc,time,sys

ctx=canvas.ctx

clientflags = ctx.flags # the flags used for launching clients

# override client flags
clientflags = ctx.HASH_CACHE | ctx.RGB332
# and set the flags specific for the launcher
ctx.flags |= ctx.HASH_CACHE | ctx.GRAY4

light_red=(255/255,80/255,80/255)
white=(1.0,1.0,1.0)
black=(0,0,0)
dark_gray=(80/255,80/255,80/255)
light_gray=(170/255,170/255,170/255)

wallpaper_bg    = black
document_bg     = white
document_fg     = black
toolbar_bg      = dark_gray
toolbar_fg      = white
button_bg       = dark_gray
button_fg       = white
dir_selected_bg = white
dir_selected_fg = black
dir_entry_fg    = white
scrollbar_fg    = dark_gray

button_height_vh = 0.33
button_width_vh  = 0.3
font_size_vh     = 0.11

if ctx.width < ctx.height:
  font_size_vh = 0.06
  button_width_vh = 0.2

cur = 0
def set_cur(event, no):
    global cur
    cur=no
    return 0

maxframe=10
def linear(start_val,end_val):
  return (frame/maxframe)*(end_val-start_val)+start_val

def mbutton(ctx, x, y, label, cb, user_data):
   ctx.save()
   ctx.font_size *= 0.75
   ctx.translate(x, y)
   ctx.begin_path().rectangle(0,0, ctx.height*button_width_vh * 0.95, ctx.height * button_height_vh * 0.95)
   ctx.listen_stop_propagate(ctx.PRESS, lambda e:cb(e, e.user_data), user_data)
   ctx.rgb(*button_bg).fill()
   ctx.rgb(*button_fg).move_to(ctx.font_size/3, ctx.height * button_height_vh / 2).text(label)
   ctx.restore()

def mbutton_thin(ctx, x, y, label, cb, user_data):
   ctx.save()
   ctx.font_size *= 0.8
   ctx.translate(x, y)
   ctx.begin_path().rectangle(0,0, ctx.height*button_width_vh * 0.95, ctx.font_size * 3)
   ctx.listen_stop_propagate(ctx.PRESS, lambda e:cb(e, e.user_data), user_data)
   ctx.begin_path().rectangle(0,0, ctx.height*button_width_vh * 0.95, ctx.font_size)

   ctx.rgb(*button_bg).fill()
   ctx.rgb(*button_fg).move_to(ctx.font_size/3, ctx.font_size*0.8).text(label)
   ctx.restore()

response = False

def respond(val):
    global response
    response = val

more_actions=False
def show_more_cb(event, userdata):
    global more_actions
    more_actions = True
    #event.stop_propagate=1

def hide_more_cb(userdata):
    global more_actions
    more_actions = False
    #event.stop_propagate=1


def remove_cb(event, path):
    global more_actions
    os.remove(path)
    more_actions = False
    #event.stop_propagate=1

offset=0

def drag_cb(event):
    global offset
    offset -=event.delta_y/(font_size_vh*ctx.height) #20.0

view_file=""
run_file=False
frame_no = 0

def run_cb(event, path):
    global run_file
    run_file = path

def view_cb(event, path):
    global view_file, offset
    offset = 0
    view_file = path
    #event.stop_propagate=1

def space_cb(event):
    run_cb(None, current_file)
    print(event.string)

def up_cb(event):
    global cur
    cur -= 1
    if cur <= 0:
        cur = 0

def down_cb(event):
    global cur
    cur += 1

def exit_cb(event):
    global main_exit
    main_exit = True

import micropython

def dir_view(ctx):
   global cur,frame_no,current_file,offset
   frame_no += 1

#   if cur > 4:
       

   #gc.collect()
   ctx.start_frame()
   #micropython.mem_info()
   ctx.add_key_binding("space", "", "", space_cb)
   ctx.add_key_binding("up", "", "", up_cb)
   ctx.add_key_binding("down", "", "", down_cb)
   ctx.add_key_binding("q", "", "", exit_cb)

   ctx.font_size=ctx.height*font_size_vh#32

   if (offset ) < cur - (ctx.height/ctx.font_size) * 0.75:
     offset = cur - (ctx.height/ctx.font_size)*0.42
   if (offset ) > cur - (ctx.height/ctx.font_size) * 0.2:
     offset = cur - (ctx.height/ctx.font_size)*0.42
   if offset < 0:
     offset = 0



   y = ctx.font_size - offset * ctx.font_size
   no = 0 
      
   ctx.rectangle(0,0,ctx.width,ctx.height).rgb(0.0,0.0,0.0).fill()
   #ctx.listen(ctx.MOTION, drag_cb)
   #ctx.begin_path()
   
   current_file = ""

   if hasattr(os, 'listdir'):
     file_list = os.listdir('/')
   else:
     file_list = []
     for f in os.ilistdir('/'):
         file_list.append(f[0])
   file_list.sort()

   for file in file_list:        
    if file[0] != '.':
      ctx.rectangle(ctx.height * button_width_vh * 1.2,y-ctx.font_size*0.75,
                  ctx.width - ctx.height * button_width_vh * 1.2, ctx.font_size)
      if no == cur:
        ctx.rgb(*dir_selected_bg)
        current_file = file
        ctx.fill()
        ctx.rgb(*dir_selected_fg)
      else:
        ctx.listen(ctx.PRESS, lambda e:set_cur(e, e.user_data), no)
        ctx.begin_path()
        ctx.rgb(*dir_entry_fg)
      ctx.move_to(ctx.height * button_width_vh * 1.2 + ctx.font_size * 0.1,y)
      
      ctx.text(file)
      
      y += ctx.font_size
      no += 1

   mbutton(ctx, 0, 0,
          "view", view_cb, current_file)
   mbutton(ctx, 0, ctx.height * button_height_vh,
          "run", run_cb, current_file)
   mbutton(ctx, 0, ctx.height - ctx.height * button_height_vh * 1,
          "...", show_more_cb, current_file)
   if more_actions:
     ctx.rectangle(60,0,ctx.width,ctx.height)
     ctx.listen_stop_propagate(ctx.PRESS, hide_more_cb)
     mbutton(ctx, ctx.width-ctx.height * button_width_vh, ctx.height - ctx.height * button_height_vh * 1, "remove", remove_cb, current_file)

   if frame_no > 1000:
     ctx.save()
     global_alpha=((frame_no-32)/50.0)
     if global_alpha > 1.0:
         global_alpha = 1.0
     elif global_alpha < 0:
         global_alpha = 0.0
     ctx.global_alpha = global_alpha
     ctx.logo(ctx.width - 32,ctx.height-32,64)
     ctx.restore()
   ctx.end_frame()
   
def scrollbar_cb(event):
    global offset
    factor = (event.y - ctx.font_size * 1.5) / (ctx.height-ctx.font_size*2);
    if factor < 0.0:
        factor = 0.0
    if factor > 1.0:
        factor = 1.0
    offset = factor * event.user_data

def prev_page_cb(event):
    global offset
    offset -= ((ctx.height / ctx.font_size) - 2)

def next_page_cb(event):
    global offset
    offset += ((ctx.height / ctx.font_size) - 2)

def file_view(ctx):
   global offset
   gc.collect()
   ctx.start_frame()
   ctx.save()

   ctx.font_size = ctx.height * font_size_vh

   #offset += 0.25
   ctx.rectangle(0,0,ctx.width,ctx.height)
   #ctx.listen(ctx.MOTION, drag_cb)
   ctx.rgb(*document_bg).fill()
   
   ctx.rgb(*document_fg)
   ctx.translate(0,(int(offset)-offset) * ctx.font_size)
   ctx.move_to(0, ctx.font_size*0.8 * 2)
   line_no = 0
   ctx.font="mono";
   y = ctx.font_size * 0.8 * 2
   with open(view_file,'r') as file:
     for line in file:
       if line_no > offset and y - ctx.font_size < ctx.height:
          ctx.move_to (0, int(y))
          for word in line.split():
            ctx.move_to(int(ctx.x), int(ctx.y))
            ctx.text(word + ' ')
          y+=ctx.font_size
            
       line_no += 1
   ctx.restore()

   ctx.save()
   ctx.rgb(*toolbar_bg)
   ctx.rectangle(0,0,ctx.width, ctx.font_size).clip()
   ctx.paint()
   ctx.text_align=ctx.RIGHT
   ctx.rgb(*toolbar_fg)
   ctx.move_to(ctx.width, ctx.font_size*0.8)
   ctx.text(view_file)
   ctx.restore()
   mbutton_thin(ctx, 0, ctx.height * button_height_vh * 0, "close", lambda e,d:view_cb(e,False), -3)

   ctx.rgb(*scrollbar_fg)
   
   ctx.move_to(ctx.width - ctx.font_size * 1.2, ctx.font_size + ctx.font_size)
   
   if True:#draw scrollbar
    ctx.line_to(ctx.width - ctx.font_size * 1.2, ctx.height - ctx.font_size)
    ctx.line_width=1
    ctx.stroke()
    ctx.arc(ctx.width - ctx.font_size * 1.2,
          ctx.font_size + ctx.font_size + (offset / line_no) * (ctx.height - ctx.font_size * 2),
          ctx.font_size*0.8, 0.0, 3.14152*2, 0).stroke()
    ctx.rectangle(ctx.width - ctx.font_size * 2, 0, ctx.font_size * 2, ctx.height)
    ctx.listen(ctx.PRESS|ctx.DRAG_MOTION, scrollbar_cb, line_no)
    ctx.begin_path()
    
   ctx.rectangle(0, ctx.font_size,
               ctx.width, (ctx.height - ctx.font_size)/2-1)
   ctx.listen(ctx.PRESS, prev_page_cb)
   ctx.begin_path()
   ctx.rectangle(0, ctx.font_size + (ctx.height - ctx.font_size)/2,
               ctx.width, (ctx.height - ctx.font_size)/2-1)
   ctx.listen(ctx.PRESS, next_page_cb)
   ctx.begin_path()
   ctx.end_frame()


main_exit = False
while not main_exit:
    if view_file:
        file_view(ctx)
    else:
        dir_view(ctx)
        if run_file != False:
            backupflags = ctx.flags
            # we remove any scratch format from flags
            ctx.flags = backupflags - (ctx.flags&(ctx.GRAY2|ctx.GRAY4|ctx.RGB332))
            # and add in low res
            ctx.flags = clientflags

            gc.collect()
            ctx.start_frame()    
            ctx.rgb(*wallpaper_bg).paint()
            ctx.end_frame()
            ctx.start_frame()    
            ctx.rgb(*wallpaper_bg).paint()
            ctx.font_size=ctx.height*font_size_vh#32
            ctx.rgb(*white).move_to(0,ctx.font_size).text(run_file)
            ctx.end_frame()
            gc.collect()


            
            try:
              exec(open(run_file).read())
            except Exception as e:
              string_res=io.StringIO(256)
              sys.print_exception(e, string_res)
              for frame in range(0,2):
               ctx.start_frame()
               ctx.rgb(*wallpaper_bg).paint()
               ctx.font_size=ctx.height*font_size_vh#32
               ctx.rgb(*white).move_to(0,ctx.font_size).text(run_file)
               #ctx.rgb(*[255,0,0]).move_to(0,ctx.font_size*2).text(str(e))
               ctx.rgb(*light_red).move_to(0,ctx.font_size*3).text(string_res.getvalue())

               ctx.end_frame()
              time.sleep(5)
     
            ctx.flags = backupflags
            run_file = False
            gc.collect()


