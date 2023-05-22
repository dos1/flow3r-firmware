##demo python payload

/!\ PHOTOSENSITIVITY WARNING /!\

due to captouch malfunctions this payload can produce rapidly flashing lights. please refrain from using it if this could cause issues for you. this is a driver issue and is currently being addressed.

###available states:

MENU: the screen shows the text "SELECT :3" and a volume bar. the MENU can be entered at any point by pressing right shoulder button down. volume can be adjusted with right shoulder button L/R in any state. two bottom petals are lit up by LEDs in different colors. pressing each of them loads one of two available INSTRUMENTS:

INSTRUMENT 1, CHORD ORGAN: all LEDs are lit up in the same color. each top petal plays a different note. each bottom petal changes global LED color and changes the notes for each top petal. the note selection is intended to provide 5 different chords accessible with the bottom petals which can be manually appregiated with the top petals in a crude omnichord-esque fashion. the screen is entirely black.

INSTRUMENT 2, MELODY PLAYER: the 3 most down-pointing petal LEDs are lit in pink. these 3 petals select between 3 different octaves. depending on octave selection, the remaining petals are lit in blue (low octave), cyan (mid octave) and green (high octave). these "playing petals" provide tones in a major scale. it is a single oscillator system, so pressing more than one "playing petal" results in the oscillator jumping between pitches rapidly, providing a HIDDEN NOISE MODE. the screen is entirely black. author's note: this one didn't turn out well at all and could use a bunch more love. we find it valuable to provide people with a simple instrument to try to play their favorite melodies on, but this ain't it yet.

###current captouch issues

the current firmware is built around AD7147 captouch controllers and uses their simplified output, where the controller autocalibrates and guesses whether a pad is being touched or not. since the controller was designed to have several mm of material in between (as for a stovetop), it sometimes is wayyyy to sensitive.

the captouch controller can be forced into recalibration by holding the right shoulder button down while in the menu, however it does keep some internal state and sometimes turning the badge off and on again is the only way to get acceptable captouch performance.

in general, for best performance we recommend the following flow:
- hold the badge as you would hold it for playing (i.e., give it a reasonable initial calibration baseline), then plug in the USB cable on the other side to power up the badge
- wait for the menu to show
- play
- if issues occur or you switch hand position, do a manual recalibration and hope for the best
- if hope is lost powercycle

the new captouch driver takes raw data from the captouch controller and processes it on the esp32. it is currently being worked on in the raw_captouch branch, but sadly only for p4 badges.

also some individual pads on some revisions are not responding. the new driver is intended to fix that as well.
