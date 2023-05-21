#!/usr/bin/env uctx

import ctx
import gc

pointer_x = 0
pointer_y = 0
down = False
was_down = False
text_edit_preview = ''

cursor_pos=[0,0]
widget_no = 0
grab_no = 0
text_edit_no = 0
text_edit_font_size = 0
text_edit_copy = None
text_edit_done = False

import canvas

o = canvas.ctx

try:
    font_size_vh
except NameError:
    if o.width < o.height:
      font_size_vh = 0.05
    else:
      font_size_vh = 0.06

widget_fs=0.1
widget_height_fs=2.0

#o.flags = o.HASH_CACHE | o.RGB332 | o.INTRA_UPDATE
o.flags = o.HASH_CACHE|o.RGB332
wallpaper_bg=(0,0,0)
widget_font_size_vh = font_size_vh
widget_bg=(80/255,80/255,80/255)
widget_fg=(1.0,1.0,1.0)
widget_marker=(163/255,100/255,180/255)
button_bg=widget_bg
button_fg=widget_fg
button_bg_active=(225/255,35/255,90/255)

widget_x=0
widget_y=0
widget_width=0
widget_height=0

edge_left = o.height * font_size_vh * 8
edge_right = o.width - edge_left

def add_widget(x, y, w, h):
    global widget_no
    widget_no += 1
    return widget_no
def get_ctx():
    return o
  
def inside_rect(u,v,x,y,w,h):
    if u >= x and  u < x + w and v >= y and v < y + h:
      return True
    return False

def slider_sized(x, y, w, h, label, value, min, max):
    global grab_no
    add_widget(x, y, w, h)
    o.font_size = o.height * widget_font_size_vh # why do we need this override
    o.rgb(*widget_bg).rectangle(x, y, w, h - 2).fill()
    o.rgb(*widget_marker).rectangle(x + (value-min)/(max-min)*w, y, 0.1*o.font_size, h).fill()     
    o.rgb(*widget_fg)
    o.move_to(x + o.font_size*0.1, y + o.font_size*0.8)
    o.text(label)
    o.save()
    o.text_align=o.RIGHT
    o.move_to(x+w,y+o.font_size*0.8)
    o.text(str(value))
    o.restore()
    
    if text_edit_no != 0:
      return value
    
    if down and text_edit_no==0:
      if inside_rect(pointer_x,pointer_y,x,y,w,h) or\
         grab_no == widget_no:
        if was_down == False:
          grab_no = widget_no
        if grab_no == widget_no:
          value = ((pointer_x - x) / w) * (max-min) + min
          if value < min:
            value = min
          if value > max:
            value = max
    else:
      grab_no = 0
    o.begin_path()
    return value

def label_sized(x, y, w, h, label):
    global down, was_down
    add_widget(x,y,w,h)
    o.font_size = o.height * widget_font_size_vh # why do we need this override
    o.rgb(*button_fg)
    o.move_to(x + o.font_size/8, y + h/2 + o.font_size/3).text(label)


def button_sized(x, y, w, h, label):
    global down, was_down

    add_widget(x,y,w,h)
    result = False
    o.font_size = o.height * widget_font_size_vh # why do we need this override
    lwidth = o.text_width (label)
    o.rectangle(x, y, w, h)
    o.rgb(*button_bg)
    if text_edit_no == 0 and down and was_down == False and inside_rect(pointer_x,pointer_y,x,y,w,h):
      result = True
      o.rgb(*button_bg_active)    
    o.fill()
    o.rgb(*button_fg)
    o.move_to(x + w/2-lwidth/2, y + h/2 + o.font_size/3).text(label)
    

    return result

def toggle_sized(x, y, w, h, label, value):
    global down, was_down
    add_widget(x,y,w,h)
    result = value

    o.font_size = o.height * widget_font_size_vh # why do we need this override
    
    if text_edit_no == 0 and down and was_down == False and inside_rect(pointer_x,pointer_y,x,y,w,h):
      result = not value
      o.rectangle(x, y, w, h).rgb(*button_bg_active).fill()

    o.rgb(*widget_fg)
    o.move_to(x + o.font_size*0.1, y + o.font_size*0.8)
    o.text(label)
    o.save()
    o.text_align=o.RIGHT
    o.move_to(x+w,y+o.font_size*0.8)
    if value:
        o.text("on")
    else:
        o.text("off")
    o.restore()
    
    if text_edit_no != 0:
       result = value
    
    return result


def entry_sized(x, y, w, h, label, value, default_value):
    global down, was_down, cursor_pos, text_edit_no, text_edit_font_size, text_edit_done, text_edit_copy
    widget_no=add_widget(x,y,w,h)
    result = value
    o.font_size = o.height * widget_font_size_vh # why do we need this override
    
    o.rgb(*widget_fg)
    o.move_to(x + o.font_size*0.1, y + o.font_size*0.8)
    o.text(label)
    
    o.save()
    #o.text_align=o.RIGHT
    #o.move_to(x+w,y+o.font_size*0.8)
    #if value:
    if text_edit_no == widget_no:
      o.text(text_edit_copy)
    else:
      o.text(value)
    
    if text_edit_no == 0 and down and was_down == False and inside_rect(pointer_x,pointer_y,x,y,w,h):
      #result = "baah"#not value
      #o.rectangle(x, y, w, h).rgb(*button_bg_active).fill()
      text_edit_copy = value
      text_edit_no = widget_no
      text_edit_font_size = o.font_size
      
    if text_edit_no == widget_no:
       cursor_pos=[o.x,o.y]
       o.move_to(cursor_pos[0],cursor_pos[1])

       o.rgb(1.0,1.0,1.0).text(text_edit_preview)
       o.rectangle(x, y, w, h).rgb(*button_bg_active).stroke()
       o.rectangle(cursor_pos[0],cursor_pos[1]-o.font_size,3, o.font_size * 1.1).fill()
       #o.move_to(cursor_pos[0],cursor_pos[1])
        
    if text_edit_done:
        text_edit_done = False # we only do this once
        text_edit_no = 0
        o.restore()

        return text_edit_copy
    o.restore()
    return None

def init_geom(x,y,width,height):
    absolute=True
    if x==None or y==None:
      x = edge_left #o.x
      y = o.y
      absolute=False
    if height==None:
      height = o.height * widget_font_size_vh * widget_height_fs
    if width==None:
      width = edge_right-edge_left #o.width

    # convert to integer and drop least signifcant
    # bit, this makes 2x2 preview renders avoid
    # rectangle AA
    x = int(x) & ~1
    y = int(y) & ~1
    width = int(width) & ~1
    height = int(height) & ~1
      
    return (x,y,width,height,absolute)


def slider(label, value, min, max, x=None, y=None, width=None, height=None):
    tup = init_geom(x,y,width,height)
    x=tup[0];y=tup[1];width=tup[2];height=tup[3];absolute=tup[4]

    retval = slider_sized (x, y, width, height, label, value, min, max)

    if not absolute:
      o.move_to (x, y + height)

    return retval


def toggle(label, value, x=None, y=None, width=None, height=None):
    tup = init_geom(x,y,width,height)
    x=tup[0];y=tup[1];width=tup[2];height=tup[3];absolute=tup[4]

    retval = toggle_sized (x, y, width, height, label, value)

    if not absolute:
      o.move_to (x, y + height)

    return retval


def button(string, x=None, y=None, width=None, height=None):
    tup = init_geom(x,y,width,height)
    x=tup[0];y=tup[1];width=tup[2];height=tup[3];absolute=tup[4]
    
    retval = button_sized (x, y, width, height * 0.98, string)
    if not absolute:
      o.move_to (x, y + height)
    return retval

def label(string, x=None, y=None, width=None, height=None):
    tup = init_geom(x,y,width,height)
    x=tup[0];y=tup[1];width=tup[2];height=tup[3];absolute=tup[4]

    label_sized (x, y, width, height * 0.98, string)
    if not absolute:
      o.move_to (x, y + height)
    
def entry(label,string,default_string,x=None,y=None,width=None,height=None):
    tup = init_geom(x,y,width,height)
    x=tup[0];y=tup[1];width=tup[2];height=tup[3];absolute=tup[4]
    string = entry_sized (x, y, width, height * 0.98, label, string, default_string)
    if not absolute:
      o.move_to (x, y + height)
    return string

def coords_press(x, y):
    global pointer_x, pointer_y, down
    pointer_x = x
    pointer_y = y
    down = True

def coords_motion(x, y):
    global pointer_x, pointer_y
    pointer_x = x
    pointer_y = y

def coords_release(x, y):
    global pointer_x, pointer_y, down
    pointer_x = x
    pointer_y = y
    down = False

exit = False

KEYs = (
        (
            ('q','w','e','r','t','y','u','i','o','p'),
            ('a','s','d','f','g','h','j','k','l','⏎'),
            ('z','x','c','v','b','n','m',',','.'),
            ('ABC',' ',' ',' ',' ',' ',' ',' ',' ','⌫')
        ),
        (
            ('Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',),
            ('A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', '⏎'),
            ('Z', 'X', 'C', 'V', 'B', 'N', 'M', ':', '?'),
            ('123', ' ',' ', ' ', ' ', ' ', ' ', '!',' ','⌫')
        ),
        (
            ('1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '|'),
            ('@', '#', '$', '%', '^', '&', '*', '(', ')', '⏎'),
            ('+','-','=','_','"','\'','[',']'),
            ('æøå', ' ', '~', '/', '\\', '{','}','`',' ','⌫')
        ),
        (
            ('pu','é', 'ñ', 'ß', 'ü', 'Ü', 'æ', 'ø', 'å', '|'),
            ('pd', '€', '®', '©', 'Æ', 'Ø', 'Å',' ', ' ', '⏎'),
            ('ctr' ,'alt', ' ',' ↑', ' ', 'ö','Ö','Å'),
            ('abc', ' ','←','↓','→', ' ',' ',' ',' ','⌫')
        )

        
        # add a layout with cursor keys and modifiers, return space tab escape
        # home, end insert delta page up page down .. and even some F keys?
    )

osk_layout=0

def start_frame():
    global widget_no
    widget_no = 0
    o.start_frame()
    o.rgb(*wallpaper_bg).paint()
    o.rectangle(0,0,o.width, o.height)#,o.height)
    o.listen(o.PRESS, lambda e:coords_press(e.x, e.y))
    o.listen(o.MOTION, lambda e:coords_motion(e.x, e.y))
    o.listen(o.RELEASE, lambda e:coords_release(e.x, e.y))
    o.fill()

import math

keyboard_bg=(70/255,70/255,90/255)
keyboard_fg=(1.0,1.0,1.0)


def keyboard(x,y,width,height):
    global text_edit_copy, cursor_pos, text_edit_preview, osk_layout, text_edit_done
    o.rectangle(x,y,width,height)
    #o.listen_stop_propagate(o.PRESS|o.MOTION, lambda e:update_coords(e.x, e.y, True))
    #o.listen_stop_propagate(o.RELEASE, lambda e:update_coords(e.x, e.y, False))
    o.rgb(*keyboard_bg).fill()
    cell_width = width /11.0;
    cell_height = height / 4.0;
    o.font_size = cell_height;
    row_no=0
    o.save()
    o.text_align=o.CENTER
    o.rgb(*keyboard_fg)
    
    nearest = False
    best_dist = 1000
    
    for row in KEYS[osk_layout]:
        col_no = 0
        for key in row:
          keyx = x + (col_no+0.5 + row_no/4.0) * cell_width
          keyy = y + row_no * cell_height + cell_height * 0.5

          dist = math.sqrt((pointer_x-keyx)*(pointer_x-keyx)+ (pointer_y-keyy)*(pointer_y-keyy))
          if dist < best_dist:
              best_dist = dist
              nearest = key
          o.move_to(keyx, keyy + cell_height * 0.3)
          label = str(key)
          label = {'\b':'bs','\n':'nl'}.get(label,label)
          o.text(label)
          col_no += 1
        row_no += 1
    o.restore()
    if inside_rect(pointer_x,pointer_y,x,y,width,height):
      if down:
        text_edit_preview = nearest
      else:
        text_edit_preview = ''
      if not down and was_down:
        
        o.font_size = text_edit_font_size
        if nearest == '⏎':
          text_edit_done = True
        elif nearest == '⌫':
          if len(text_edit_copy)>0:
            cursor_pos[0] -= o.text_width (text_edit_copy[-1])
            text_edit_copy = text_edit_copy[0:-1]
        elif nearest == 'ABC':
            osk_layout=1
        elif nearest == '123':
            osk_layout=2
        elif nearest == '<>{':
            osk_layout=3
        elif nearest == 'æøå':
            osk_layout=3
        elif nearest == 'abc':
            osk_layout=0
        else:
          text_edit_copy += nearest
          #cursor_pos[0] += o.text_width (nearest)
          text_edit_preview = ''
    else:
       text_edit_preview = ''

def end_frame():
    global was_down, down, text_edit_no
    
    if text_edit_no > 0:
        keyboard(0,o.height/2,o.width,o.height/2)
    
    
    was_down = down # must happen after all the buttons    
    gc.collect()
    o.end_frame()
    




    
def nearest_in_list(needle,list):
    best_score=400000
    best=needle
    for i in list:
        score = abs(i-needle)
        if score < best_score:
            best_score = score
            best=i
    return best

def choice(label, chosen, choices):
    return chosen

if __name__=='__main__':
  exit = 0
  constrain_332 = False
  string = ""
  red = widget_bg[0]
  green = widget_bg[1]
  blue = widget_bg[2]
  chosen = "foo"
  
  start_frame()
  end_frame()
  gc.collect()
 
  while exit < 4: # a hack to let the ui re-settle, makes
                  # switching apps slower though
    start_frame()
    edited_string = entry ("entry: ", string, "default")
    if edited_string:
        string = edited_string

    chosen = choice ("among", chosen, ("foo","bar","baz"))
        
    label (chosen)
        
    constrain_332 = toggle ("RGB332", constrain_332)
    red  = int(slider("red", red, 0, 255))
    green  = int(slider("green", green, 0, 255))
    blue = int(slider("blue", blue, 0, 255))

    o.rectangle (o.x+o.width/2,o.y - o.font_size * 4,o.width * 0.15, o.height * 0.15)
    o.rgb(red/255,green/255,blue/255).fill()
    o.rectangle (o.x+o.width/2,o.y - o.font_size * 4,o.width * 0.15, o.height * 0.15)
    o.rgb(1.0,1.0,1.0).stroke()

    if constrain_332:
      valid_red=[0,40,80,140,174,209,233,255]  # 176
      valid_green=[0,40,80,140,174,209,233,255]  # 176
      valid_blue=[0,80,170,255]    
      red = nearest_in_list(red, valid_red)
      green = nearest_in_list(green, valid_green)
      blue = nearest_in_list(blue, valid_blue)

    if button ("done"):
        exit = True

    if button("x", x=40, y=40, width=50, height=50):
        exit = True

    if False:
      widget_bg[0] = red
      widget_bg[1] = green
      widget_bg[2] = blue
        
    
    end_frame()
    gc.collect()
    if exit > 0:
        exit += 1

