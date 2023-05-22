### user input

```
button_get(button_number)
```
RETURNS state of a shoulder button (0: not pressed, -1: left, 1: right, 2: down)

`button_number` (int, range 0-1)
: 0 is right shoulder button, 1 is left shoulder button

---

```
captouch_get(petal_number)
```
RETURNS 0 if captouch petal is not touched, 1 if it is touched.

note that captouch is somewhat buggy atm.

`petal_number` (int, range 0-9)
: which petal. 0 is the top petal above the USB-C port, increments ccw.

---
```
captouch_autocalib()
```
recalibrates captouch "not touched" reference

### LEDs

```
led_set_hsv(index, hue, sat, value)
```
prepares a single LED to be set to a color with the next `leds_update()` call.

`index` (int, range 0-39)
: indicates which LED is set. 0 is above the USB-C port, increments ccw. there are 8 LEDs per top petal.

`hue` (float, range 0.0-359.0)
: hue

`val` (float, range 0.0-1.0)
: value

`sat` (float, range 0.0-1.0)
: saturation

---

```
led_set_rgb(index, hue, sat, value)
```
prepares a single LED to be set to a color with the next `leds_update()` call.

`index` (int, range 0-39)
: indicates which LED is set. 0 is above the USB-C port, increments ccw. there are 8 LEDs per top petal.
`r` (int, range 0-255)
: red
`g` (int, range 0-255)
: green
`b` (int, range 0-255)
: blue

---

```
leds_update()
```
writes LED color configuration to the actual LEDs.

### display (DEPRECATED SOON)

```
display_set_pixel(x, y, col)
```
write to a single pixel in framebuffer to be set to a color with the next `display_update()` call

`x` (int, range 0-239)
: sets x coordinate (0: right)
`y` (int, range 0-239)
: sets y coordinate (0: down)
`col` (int, range 0-65535)
: color as an rgb565 number

---

```
display_get_pixel(x, y, col)
```
RETURNS color of a single pixel from the framebuffer as an rgb565 number

`x` (int, range 0-239)
: sets x coordinate (0: right)
`y` (int, range 0-239)
: sets y coordinate (0: down)

---

```
display_fill(col)
```
writes to all pixels in framebuffer to be set to a color with the next `display_update()` call

`col` (int, range 0-65535)
: color as an rgb565 number

---

```
display_update()
```
writes framebuffer to display
